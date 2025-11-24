#pragma GCC optimize ("Os")    // Optimize for size

#include "secrets.h"
#include <driver/i2s.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <SafeSound_AudioClassifier_inferencing.h>

// ------------------- I2S Configuration -------------------
#define I2S_WS   3
#define I2S_SD   4
#define I2S_SCK  2
#define MIC_LR_PIN 5   // any GPIO
#define I2S_PORT I2S_NUM_0

#define CHUNK_SAMPLES 256

// ------------------- BLE Configuration -------------------
#define SERVICE_UUID        "7b58dad6-aa86-4330-9390-c2edf92f50ef"
#define CHARACTERISTIC_UUID "21c88cf2-1bba-4d5b-a70a-aa555d43921b"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// ------------------- Audio Settings -------------------
unsigned long lastAlert = 0;
const unsigned long ALERT_COOLDOWN = 2000;


// ------------------- BLE Callbacks -------------------
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("BLE client connected");
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) override {
    Serial.println("BLE client disconnected. Restarting advertising...");
    deviceConnected = false;
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("Advertising restarted.");
  }
};

// ------------------- I2S Setup -------------------
void i2s_install() {
  Serial.println("Installing I2S driver...");
  const i2s_config_t cfg = {
    .mode=i2s_mode_t(I2S_MODE_MASTER|I2S_MODE_RX),
    .sample_rate=16000,
    .bits_per_sample=I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format=I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format=I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags=ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count=4,
    .dma_buf_len=CHUNK_SAMPLES,
    .use_apll=false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  esp_err_t err = i2s_driver_install(I2S_PORT,&cfg,0,NULL);
  if (err == ESP_OK) Serial.println("I2S driver installed successfully");
  else Serial.printf("I2S driver install failed. Code: %d\n", err);
}

void i2s_setpin() {
  Serial.println("Configuring I2S pins...");
  const i2s_pin_config_t pin={
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = -1,
    .data_in_num  = I2S_SD
  };   
  esp_err_t err = i2s_set_pin(I2S_PORT,&pin);
  if (err == ESP_OK) Serial.println("I2S pins configured");
  else Serial.printf("I2S pin setup failed. Code: %d\n", err);
}

void sendBLEMessage(const char* soundType){
  if(deviceConnected){
    pCharacteristic->setValue(soundType);
    pCharacteristic->notify();
    Serial.printf("BLE message sent: %s\n", soundType);
  } else {
    Serial.println("No client connected, message not sent.");
  }
}

// -----------------------------------------------------
// Edge Impulse inference helper
// -----------------------------------------------------
bool runEIInference() {
  static float audioBuffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
  static size_t sampleIndex = 0;
  static String lastLabel = "";
  static uint8_t silentChunks = 0;
  static uint8_t repeatCount = 0;
  const uint8_t MIN_REPEATS = 2;


  // Temporary chunk buffer for I2S reads
  int32_t chunk[CHUNK_SAMPLES];
  size_t bytesRead = 0;

  // ---- Read a chunk of audio from I2S ----
  esp_err_t res = i2s_read(I2S_PORT, chunk, CHUNK_SAMPLES * sizeof(int32_t), &bytesRead, portMAX_DELAY);
  if (res != ESP_OK || bytesRead == 0) {
    return false;
  }

  size_t samplesRead = bytesRead / 4;

  // ---- Compute max amplitude to detect silence ----
  int32_t maxVal = 0;
  for (int i = 0; i < samplesRead; i++) {
    int32_t s = chunk[i] >> 14;  // Normalize 32-bit INMP441 output
    int32_t absS = abs(s);
    if (absS > maxVal) maxVal = absS;
  }
  Serial.println(maxVal);
  
  if (maxVal < 500) {
    silentChunks++;
    if (silentChunks > 200) {
      sampleIndex = 0;         
    }
    return false;
  } else {
    silentChunks = 0;          // reset when sound appears
  }


  // ---- Amplify and store audio into EI buffer ----
  for (size_t i = 0; i < samplesRead && sampleIndex < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
    int32_t s = chunk[i] >> 14;
    audioBuffer[sampleIndex++] = (float)s * 15.0f;   // boost gain
  }

  // If we haven't filled a full window yet, nothing to classify
  if (sampleIndex < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    return false;
  }

  // We have a full window → build signal and run classifier
  signal_t signal;
  int err = numpy::signal_from_buffer(audioBuffer, EI_CLASSIFIER_RAW_SAMPLE_COUNT, &signal);
  if (err != 0) {
    Serial.println("Failed to create signal from buffer.");
    sampleIndex = 0; // reset for next window
    return false;
  }

  ei_impulse_result_t resultEI;
  EI_IMPULSE_ERROR eiErr = run_classifier(&signal, &resultEI, false);
  if (eiErr != EI_IMPULSE_OK) {
    Serial.printf("run_classifier failed: %d\n", eiErr);
    sampleIndex = 0;
    return false;
  }

  // ---- Print predictions ----
  Serial.println("Predictions:");
  float topScore = 0.0f;
  const char* bestLabel = "UNCERTAIN";

  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    float score = resultEI.classification[ix].value;
    Serial.printf("  %s: %.2f\n", resultEI.classification[ix].label, score);

    if (score > topScore) {
      topScore = score;
      bestLabel = resultEI.classification[ix].label;
    }
  }

  // Reset buffer for next window
  sampleIndex = 0;

  // ---- Confidence threshold ----
  if (topScore < 0.60f) { 
    Serial.println("Low confidence — ignoring");
    return true;
  }

  if (lastLabel == String(bestLabel)) {
    repeatCount++;
  } else {
    lastLabel = String(bestLabel);
    repeatCount = 1;
  }

  if (repeatCount < MIN_REPEATS) {
    Serial.println("Not enough repeats yet — waiting");
    return true;
  }
  
  // ---- Cooldown ----
  if (millis() - lastAlert < ALERT_COOLDOWN) {
    return true;
  }

  // ---- Send best label over BLE ----
  sendBLEMessage(bestLabel);
  lastAlert = millis();

  return true;
}


// ------------------- Setup -------------------
void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Safe&Sound Mic BLE device...");
  
  pinMode(MIC_LR_PIN, OUTPUT);
  digitalWrite(MIC_LR_PIN, LOW); // force LEFT channel
  // Audio init
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  i2s_set_clk(I2S_PORT, 16000, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
  Serial.println("I2S started successfully.");

  // BLE init
  Serial.println("Initializing BLE...");
  BLEDevice::init("SafeSound_TX");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  //pAdvertising->setScanResponse(true);
  //pAdvertising->setMinPreferred(0x06);
  //pAdvertising->setMinPreferred(0x12);
  //pAdvertising->setAdvertisementType(ADV_TYPE_IND);
  pServer->getAdvertising()->start();
  Serial.println("BLE advertising started. Waiting for connection...");
  Serial.println("Setup complete. Entering main loop.");
}

// ------------------- Loop -------------------
void loop() {
  runEIInference();
  delay(10);
}