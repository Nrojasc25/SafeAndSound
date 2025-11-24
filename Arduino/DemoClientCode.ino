#pragma GCC optimize ("Os")    // Optimize for size
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h>
#include <WiFi.h>
#include "driver/ledc.h"
#include <Firebase_ESP_Client.h>
#include "secrets.h"  // define WIFI_SSID, WIFI_PASSWORD, API_KEY, DATABASE_URL, USER_EMAIL, USER_PASSWORD, USER_ID

// ==== Pin map ====
const int LED_R = 6;    // D3
const int LED_G = 5;    // D4
const int LED_B = 4;    // D5
const int VIB_PIN = 21; // D6

// PWM settings
const int freq = 5000;
const int resolution = 8;

// ==== BLE UUIDs ====
static BLEUUID serviceUUID("7b58dad6-aa86-4330-9390-c2edf92f50ef");
static BLEUUID charUUID   ("21c88cf2-1bba-4d5b-a70a-aa555d43921b");

// ==== Globals ====
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteChar = nullptr;
BLEAdvertisedDevice* targetDevice = nullptr;
bool connected = false;
bool shouldConnect = false;
unsigned long lastConnectionAttempt = 0;
const unsigned long CONNECTION_RETRY_DELAY = 5000; // 5s

// ==== Firebase ====
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ==== Sound Settings ====
struct Setting { char type[16]; char color[9]; char vibration[9]; };
#define MAX_SETTINGS 10
Setting settingsList[MAX_SETTINGS];
size_t settingsCount = 0;

// ==== Helpers ====
// ====================================================================================
// RGB LED PWM Setup
// ====================================================================================
void setupRGBPWM() {
 ledcAttach(LED_R, freq, resolution);
  ledcAttach(LED_G, freq, resolution);
  ledcAttach(LED_B, freq, resolution);

  // OFF state for common anode
  ledcWrite(LED_R, 255);
  ledcWrite(LED_G, 255);
  ledcWrite(LED_B, 255);
}

// ====================================================================================
// Convert Firebase Hex → RGB PWM Output
// ====================================================================================
void setLedColor(String hexColor) {
  hexColor.toLowerCase();

  // Strip alpha channel if ARGB (e.g., ff2196f3)
  if (hexColor.length() == 8)
    hexColor = hexColor.substring(2);

  if (hexColor.length() != 6) {
    Serial.println("Invalid color, using red");
    ledcWrite(LED_R, 0);
    ledcWrite(LED_G, 255);
    ledcWrite(LED_B, 255);
    return;
  }

  int r = strtol(hexColor.substring(0,2).c_str(), NULL, 16);
  int g = strtol(hexColor.substring(2,4).c_str(), NULL, 16);
  int b = strtol(hexColor.substring(4,6).c_str(), NULL, 16);

  // COMMON ANODE → INVERTED PWM
  r = 255 - r;
  g = 255 - g;
  b = 255 - b;

  ledcWrite(LED_R, r);
  ledcWrite(LED_G, g);
  ledcWrite(LED_B, b);

  Serial.printf("Set RGB (inverted PWM): %d %d %d\n", r, g, b);
}


void vibratePattern(String pattern) {
  pattern.toLowerCase();
  if (pattern == "long") {
    digitalWrite(VIB_PIN, HIGH); delay(600); digitalWrite(VIB_PIN, LOW);
  } else if (pattern == "short") {
    for (int i=0;i<3;i++){ digitalWrite(VIB_PIN,HIGH);delay(150);digitalWrite(VIB_PIN,LOW);delay(250);}
  } else if (pattern == "repeat") {
    for (int i=0;i<5;i++){ digitalWrite(VIB_PIN,HIGH);delay(100);digitalWrite(VIB_PIN,LOW);delay(100);}
  } else {
    digitalWrite(VIB_PIN,HIGH); delay(100); digitalWrite(VIB_PIN,LOW);
  }
}

Setting findSettingByType(const char* t) {
  for(size_t i=0;i<settingsCount;i++)
    if(strcasecmp(settingsList[i].type,t)==0) return settingsList[i];
  Setting none={"Unknown","ffffff","short"};
  return none;
}

// ==== Firebase Setup ====
void setupFirebase() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while(WiFi.status()!=WL_CONNECTED){ Serial.print("."); delay(500);}
  Serial.println("\nWiFi connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized");
}

void loadSettingsFromFirebase() {
  settingsCount = 0;
  for(int i=0;i<MAX_SETTINGS;i++){
    char path[80]; snprintf(path,sizeof(path),"/users/%s/settings/%d",USER_ID,i);
    if(Firebase.RTDB.getJSON(&fbdo,path)){
      String js = fbdo.jsonString();
      if(js.length()>0){
        FirebaseJson json; json.setJsonData(js);
        FirebaseJsonData val;
        if(json.get(val,"type")) strncpy(settingsList[settingsCount].type,val.stringValue.c_str(),15);
        else strcpy(settingsList[settingsCount].type,"Unknown");
        if(json.get(val,"color")) strncpy(settingsList[settingsCount].color,val.stringValue.c_str(),8);
        else strcpy(settingsList[settingsCount].color,"ffffff");
        if(json.get(val,"vibrationPattern")) strncpy(settingsList[settingsCount].vibration,val.stringValue.c_str(),8);
        else strcpy(settingsList[settingsCount].vibration,"short");
        settingsCount++;
      } else break;
    } else break;
    delay(50);
  }
  Serial.printf("Loaded %d settings from Firebase\n", settingsCount);
}

// ==== BLE Notification Callback ====
void notifyCallback(BLERemoteCharacteristic* rc, uint8_t* data, size_t len, bool isNotify) {
  String sound((char*)data,len);
  sound.trim();
  Serial.println("Notification received: " + sound);

  Setting s = findSettingByType(sound.c_str());
  Serial.printf("Action -> Color: %s | Vibration: %s\n", s.color, s.vibration);

  setLedColor(String(s.color));
  vibratePattern(String(s.vibration));
  delay(500);
  // RGB stays lit briefly, then off:
  ledcWrite(LED_R, 255);
  ledcWrite(LED_G, 255);
  ledcWrite(LED_B, 255);
}

// ==== BLE Client Callbacks ====
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient){ Serial.println("Connected to BLE server"); }
  void onDisconnect(BLEClient* pclient){
    connected = false;
    Serial.println("Disconnected, restarting scan...");
    BLEDevice::getScan()->start(0, nullptr, false);
  }
};

// ==== Connect to Detector ====
void connectToDetector(BLEAdvertisedDevice advertisedDevice) {
  BLEDevice::getScan()->stop();
  delay(500);
  BLEAddress serverAddress = advertisedDevice.getAddress();
  if(pClient==nullptr){
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }
  if(!pClient->connect(serverAddress)){
    Serial.println("Connection failed!");
    pClient->disconnect();
    lastConnectionAttempt = millis();
    BLEDevice::getScan()->start(0, nullptr, false);
    return;
  }

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if(!pRemoteService){ Serial.println("Service not found"); pClient->disconnect(); return;}
  pRemoteChar = pRemoteService->getCharacteristic(charUUID);
  if(pRemoteChar && pRemoteChar->canNotify()){
    pRemoteChar->registerForNotify(notifyCallback);
    Serial.println("Subscribed to notifications");
  }
  connected = true;
}

// ==== Scan Callback ====
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice){
    if(connected || shouldConnect) return;
    if(advertisedDevice.haveServiceUUID() &&
       advertisedDevice.isAdvertisingService(serviceUUID)){
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
      shouldConnect = true;
      BLEDevice::getScan()->stop();
    }
  }
};

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(VIB_PIN, OUTPUT);
  digitalWrite(VIB_PIN, LOW);

  setupRGBPWM();

  Serial.println("Receiver BLE Client starting...");
  BLEDevice::init("SafeSound_Receiver");

  setupFirebase();
  loadSettingsFromFirebase();

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->start(0, nullptr, false);
}

// ==== Loop ====
void loop() {
  if (shouldConnect && !connected && targetDevice != nullptr) {
    if (millis() - lastConnectionAttempt > CONNECTION_RETRY_DELAY) {
      connectToDetector(*targetDevice);
      delete targetDevice; targetDevice = nullptr; shouldConnect = false;
    }
  }

  if (connected && !pClient->isConnected()) {
    Serial.println("Connection lost, restarting scan...");
    connected = false;
    BLEDevice::getScan()->start(0, nullptr, false);
  }

  delay(2000);
}
