#pragma GCC optimize ("Os")    // Optimize for size

#include "secrets.h"
#include <driver/i2s.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// ------------------- I2S Configuration -------------------
#define I2S_WS   25
#define I2S_SD   32
#define I2S_SCK  33
#define I2S_PORT I2S_NUM_0

#define NUM_SAMPLES 64

// ------------------- BLE Configuration -------------------
#define SERVICE_UUID        "7b58dad6-aa86-4330-9390-c2edf92f50ef"
#define CHARACTERISTIC_UUID "21c88cf2-1bba-4d5b-a70a-aa555d43921b"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// ------------------- WiFi + Firebase -------------------

FirebaseData fbdo;
FirebaseData fbdoPush;
FirebaseAuth auth;
FirebaseConfig config;

// ------------------- Audio Settings -------------------
unsigned long lastAlert = 0;
const unsigned long ALERT_COOLDOWN = 2000;

const char* labels[] = {"Fire Alarm","Siren","Microwave","Timer"};
uint8_t simIndex = 0;

// ------------------- User Settings -------------------
struct Setting { char type[16]; char color[9]; char vibration[9]; };
#define MAX_SETTINGS 5
Setting settingsList[MAX_SETTINGS];
size_t settingsCount = 0;

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
    .sample_rate=44100,
    .bits_per_sample=i2s_bits_per_sample_t(16),
    .channel_format=I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format=i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags=0,
    .dma_buf_count=8,
    .dma_buf_len=64,
    .use_apll=false
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

// ------------------- Firebase Functions -------------------
void setupFirebase() {
  Serial.println("Initializing Firebase...");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email=USER_EMAIL;
  auth.user.password=USER_PASSWORD;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized and WiFi reconnect enabled");
}

void loadUserSettings() {
  Serial.println("Loading user settings from Firebase...");
  settingsCount = 0;
  for(int i=0;i<MAX_SETTINGS;i++){
    char path[80]; snprintf(path,sizeof(path),"/users/%s/settings/%d",USER_ID,i);
    Serial.printf("Fetching: %s\n", path);
    if(Firebase.RTDB.getJSON(&fbdo,path)){
      String js=fbdo.jsonString();
      if(js.length()>0){
        FirebaseJson json; json.setJsonData(js);
        FirebaseJsonData val;
        if(json.get(val,"type")) strncpy(settingsList[settingsCount].type,val.stringValue.c_str(),15);
        else strcpy(settingsList[settingsCount].type,"Unknown");
        if(json.get(val,"color")) strncpy(settingsList[settingsCount].color,val.stringValue.c_str(),8);
        else strcpy(settingsList[settingsCount].color,"ffffff");
        if(json.get(val,"vibrationPattern")) strncpy(settingsList[settingsCount].vibration,val.stringValue.c_str(),8);
        else strcpy(settingsList[settingsCount].vibration,"s");
        Serial.printf("Loaded setting %d: type=%s, color=%s, vibration=%s\n",
                      settingsCount, settingsList[settingsCount].type,
                      settingsList[settingsCount].color, settingsList[settingsCount].vibration);
        settingsCount++;
      } else {
        Serial.println("Empty JSON received, stopping load.");
        break;
      }
    } else {
      Serial.printf("Firebase read failed: %s\n", fbdo.errorReason().c_str());
      break;
    }
    delay(100);
  }
  Serial.printf("Loaded %d user settings.\n", settingsCount);
}

void onSettingsUpdate(FirebaseStream data) {
  String path = data.dataPath();
  String event = data.eventType();

  // Ignore keep-alive or root-level refresh events
  if (path == "/" || path.length() == 0 || !(event == "put" || event == "patch")) {
    return;
  }

  static unsigned long lastReload = 0;
  if (millis() - lastReload < 2000) return;
  lastReload = millis();

  Serial.printf("Firebase child changed at: %s\n", path.c_str());
  loadUserSettings();
}


Setting findSettingByType(const char* t) {
  for(size_t i=0;i<settingsCount;i++)
    if(strcasecmp(settingsList[i].type,t)==0) return settingsList[i];
  Setting none={"Unknown","ffffff","s"};
  Serial.printf("No setting found for type '%s'. Using default.\n", t);
  return none;
}

void sendBLEMessage(const Setting &s){
  if(deviceConnected){
    String msg = String(s.color) + "|" + String(s.vibration);
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();
    Serial.printf("BLE message sent: %s\n", msg.c_str());
  } else {
    Serial.println("No client connected, message not sent.");
  }
}

void pushAlertLog(const char* soundType){
  char path[64]; snprintf(path,sizeof(path),"/users/%s/alerts",USER_ID);
  FirebaseJson json;
  json.set("sound",soundType);
  json.set("timestamp",(int)millis());
  json.set("device","ESP32_Detector");
  Serial.printf("Pushing alert to Firebase: %s\n", soundType);
  if(Firebase.RTDB.pushJSON(&fbdoPush,path,&json))
    Serial.printf("Alert pushed: %s\n", soundType);
  else
    Serial.printf("Failed to push alert: %s\n", fbdoPush.errorReason().c_str());
}

// ------------------- Setup -------------------
void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Safe&Sound Mic BLE device...");

  // Audio init
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
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
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->setAdvertisementType(ADV_TYPE_IND);
  pServer->getAdvertising()->start();
  Serial.println("BLE advertising started. Waiting for connection...");

  // WiFi + Firebase
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempt = 0;
  while(WiFi.status()!=WL_CONNECTED){ 
    Serial.print(".");
    delay(500);
    attempt++;
    if (attempt % 10 == 0) Serial.printf(" (%d sec elapsed)\n", attempt/2);
  }
  Serial.println();
  Serial.println("WiFi connected: " + WiFi.localIP().toString());
  setupFirebase();
  loadUserSettings();
  if (!Firebase.RTDB.beginStream(&fbdo, "/users/P4OTxL5xMVSeljSaqTN3PINrEh12/settings")) {
    Serial.printf("Stream start failed: %s\n", fbdo.errorReason().c_str());
  } else {
    Firebase.RTDB.setStreamCallback(&fbdo, onSettingsUpdate, nullptr);
    Serial.println("Firebase stream listener started.");
  }
  Serial.println("Setup complete. Entering main loop.");
}

// ------------------- Loop -------------------
void loop(){
  int16_t sample[NUM_SAMPLES]; size_t bytes_read; int64_t sum=0;
  esp_err_t result = i2s_read(I2S_PORT,&sample,sizeof(sample),&bytes_read,portMAX_DELAY);
    
  if (result == ESP_OK) {
    int16_t samples_read = bytes_read / 8;
    if (samples_read > 0) {
      float mean = 0;
      for (int16_t i = 0; i < samples_read; ++i) {
        mean += (sample[i]);
      }
      mean /= samples_read;

      const char* detectedType = nullptr;

      if (mean > 12000) {
        detectedType = "Fire Alarm";
      } 
      else if (mean > 8000) {
        detectedType = "Siren";
      } 
      else if (mean > 4000) {
        detectedType = "Timer";
      } 
      else if (mean > 2000) {
        detectedType = "Microwave";
      }

      if (detectedType && millis() - lastAlert > ALERT_COOLDOWN) {
        Serial.printf("Sound Detected: %s (%.0f)\n", detectedType, mean);
        Setting match = findSettingByType(detectedType);
        sendBLEMessage(match);
        pushAlertLog(detectedType);
        lastAlert = millis();
      }
    }
  }
  delay(20);
}
