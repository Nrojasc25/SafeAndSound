#pragma GCC optimize ("Os")

#include <BLEDevice.h>
#include <BLERemoteCharacteristic.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "driver/ledc.h"
#include "secrets.h"

// =============================
// DEBUG
// =============================
#define DEBUG 0
#if DEBUG
  #define DPRINT(x) Serial.print(x)
  #define DPRINTLN(x) Serial.println(x)
#else
  #define DPRINT(x)
  #define DPRINTLN(x)
#endif

// =============================
// HARDWARE
// =============================
const int LED_R = 25;
const int LED_G = 26;
const int LED_B = 27;
const int VIB_PIN = 14;

// =============================
// BLE
// =============================
static BLEUUID serviceUUID("7b58dad6-aa86-4330-9390-c2edf92f50ef");
static BLEUUID charUUID("21c88cf2-1bba-4d5b-a70a-aa555d43921b");

BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteChar = nullptr;
bool connected = false;

volatile bool alertPending = false;
char alertSound[32] = {0};

// =============================
// SETTINGS
// =============================
struct Setting {
  char type[16];
  char color[9];
  char vibration[9];
};

#define MAX_SETTINGS 6
Setting settingsList[MAX_SETTINGS];
size_t settingsCount = 0;
unsigned long lastSettingsRefresh = 0;
#define SETTINGS_REFRESH_INTERVAL 30000

// =============================
// FIREBASE
// =============================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// =============================
// RGB LED
// =============================
void setupRGBPWM() {
  const int freq = 5000;      // PWM frequency
  const int resolution = 8;   // 8-bit duty

  ledcAttach(LED_R, freq, resolution);
  ledcAttach(LED_G, freq, resolution);
  ledcAttach(LED_B, freq, resolution);

  // OFF state for common anode
  ledcWrite(LED_R, 255);
  ledcWrite(LED_G, 255);
  ledcWrite(LED_B, 255);
}

void setLedColor(const char* hex) {
  int len = strlen(hex);
  if (len == 8) hex += 2;
  if (strlen(hex) != 6) {
    ledcWrite(0, 0);
    ledcWrite(1, 255);
    ledcWrite(2, 255);
    return;
  }

  char buf[3] = {0};
  buf[0] = hex[0]; buf[1] = hex[1];
  int r = 255 - strtol(buf, NULL, 16);
  buf[0] = hex[2]; buf[1] = hex[3];
  int g = 255 - strtol(buf, NULL, 16);
  buf[0] = hex[4]; buf[1] = hex[5];
  int b = 255 - strtol(buf, NULL, 16);

  ledcWrite(0, r);
  ledcWrite(1, g);
  ledcWrite(2, b);
}

// =============================
// VIBRATION
// =============================
void vibratePattern(const char* p) {
  if (strcasecmp(p, "long") == 0) {
    digitalWrite(VIB_PIN, HIGH); delay(600);
  } 
  else if (strcasecmp(p, "short") == 0) {
    for (int i=0;i<3;i++){
      digitalWrite(VIB_PIN, HIGH); delay(150);
      digitalWrite(VIB_PIN, LOW); delay(250);
    }
  }
  else if (strcasecmp(p, "repeat") == 0) {
    for (int i=0;i<5;i++){
      digitalWrite(VIB_PIN, HIGH); delay(100);
      digitalWrite(VIB_PIN, LOW); delay(100);
    }
  }
  else {
    digitalWrite(VIB_PIN, HIGH); delay(120);
  }
  digitalWrite(VIB_PIN, LOW);
}

// =============================
// FIND SETTING
// =============================
Setting* findSettingByType(const char* t) {
  for (size_t i=0; i<settingsCount; i++)
    if (strcasecmp(settingsList[i].type, t) == 0)
      return &settingsList[i];
  return nullptr;
}

// =============================
// WIFI
// =============================
void setupWiFi() {
  DPRINTLN("\n========================================");
  DPRINTLN("CONNECTING TO WIFI");
  DPRINTLN("========================================");
  DPRINT("SSID: "); DPRINTLN(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int timeout = 20;
  while (WiFi.status() != WL_CONNECTED && timeout-- > 0) {
    delay(500);
    DPRINT(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    DPRINTLN("\n✓ WiFi CONNECTED");
    DPRINT("  IP: "); DPRINTLN(WiFi.localIP());
  } else {
    DPRINTLN("\n✗ WiFi FAILED");
  }
  DPRINTLN("========================================\n");
}

// =============================
// FIREBASE SETUP
// =============================
void setupFirebase() {
  DPRINTLN("Setting up Firebase...");
  
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  DPRINTLN("✓ Firebase initialized");
}

// =============================
// LOAD SETTINGS
// =============================
void loadSettingsFromFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    DPRINTLN("✗ Cannot load settings: No WiFi");
    return;
  }
  
  DPRINTLN("\n========================================");
  DPRINTLN("LOADING SETTINGS FROM FIREBASE");
  DPRINTLN("========================================");
  
  settingsCount = 0;
  
  for (int i = 0; i < MAX_SETTINGS; i++) {
    char path[80];
    snprintf(path, sizeof(path), "/users/%s/settings/%d", USER_ID, i);
    
    // Add timeout protection
    unsigned long startTime = millis();
    bool success = Firebase.RTDB.getJSON(&fbdo, path);
    
    // Timeout after 5 seconds
    if (millis() - startTime > 5000) {
      DPRINTLN("✗ Firebase timeout");
      break;
    }
    
    if (success) {
      String jsonStr = fbdo.jsonString();
      if (jsonStr.length() > 5) {
        FirebaseJson json;
        json.setJsonData(jsonStr);
        FirebaseJsonData val;
        
        memset(&settingsList[settingsCount], 0, sizeof(Setting));
        strcpy(settingsList[settingsCount].vibration, "short");
        
        if (json.get(val, "type")) {
          strncpy(settingsList[settingsCount].type, val.stringValue.c_str(), 
                  sizeof(settingsList[settingsCount].type) - 1);
        }
        
        if (json.get(val, "color")) {
          strncpy(settingsList[settingsCount].color, val.stringValue.c_str(), 
                  sizeof(settingsList[settingsCount].color) - 1);
        }
        
        if (json.get(val, "vibrationPattern")) {
          strncpy(settingsList[settingsCount].vibration, val.stringValue.c_str(), 
                  sizeof(settingsList[settingsCount].vibration) - 1);
        }
        
        DPRINT("  ✓ Loaded: "); DPRINT(settingsList[settingsCount].type);
        DPRINT(" | "); DPRINT(settingsList[settingsCount].color);
        DPRINT(" | "); DPRINTLN(settingsList[settingsCount].vibration);
        
        settingsCount++;
      } else {
        break;
      }
    } else {
      break;
    }
    
    delay(50);
    yield(); // Feed the watchdog
  }

  lastSettingsRefresh = millis();
  DPRINTLN("========================================");
  DPRINT("✓ TOTAL SETTINGS LOADED: "); DPRINTLN(settingsCount);
  DPRINTLN("========================================\n");
}

// =============================
// PUSH ALERT LOG
// =============================
void pushAlertLog(const char* soundType, const char* colorValue) {
  if (WiFi.status() != WL_CONNECTED) {
    DPRINTLN("  ✗ Cannot log: No WiFi");
    return;
  }
  
  DPRINTLN("  Logging alert to Firebase...");
  
  char path[64];
  snprintf(path, sizeof(path), "/users/%s/alerts", USER_ID);

  FirebaseJson json;
  json.set("sound", soundType);
  json.set("timestamp", (int)millis());
  json.set("device", "ESP32_Receiver");
  json.set("color", colorValue);

  unsigned long startTime = millis();
  if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
    DPRINTLN("  ✓ Alert logged");
  } else {
    // Timeout protection
    if (millis() - startTime > 5000) {
      DPRINTLN("  ✗ Alert log timeout");
    } else {
      DPRINT("  ✗ Alert log failed: "); DPRINTLN(fbdo.errorReason());
    }
  }
}

// =============================
// BLE NOTIFY CALLBACK
// =============================
void notifyCallback(BLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  // keep this callback tiny and non-blocking
  if (len == 0) return;

  // limit length to fit buffer and include NUL
  if (len > (sizeof(alertSound) - 1)) len = sizeof(alertSound) - 1;

  // copy into shared buffer atomically
  noInterrupts();
  memcpy(alertSound, data, len);
  alertSound[len] = '\0';
  alertPending = true;
  interrupts();
}

// =============================
// BLE CALLBACKS
// =============================
class MyClientCallback : public BLEClientCallbacks {
  void onDisconnect(BLEClient*) override {
    connected = false;
    DPRINTLN("BLE disconnected");
  }
};

// =============================
// CONNECT BLE
// =============================
void connectToDetector() {
  static BLEAddress addr("80:65:99:2a:5c:b6");

  if (!pClient) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }

  if (!pClient->connect(addr)) {
    connected = false;
    DPRINTLN("BLE connect failed");
    return;
  }

  BLERemoteService* s = pClient->getService(serviceUUID);
  if (!s) {
    DPRINTLN("No BLE service");
    return;
  }

  pRemoteChar = s->getCharacteristic(charUUID);
  if (pRemoteChar && pRemoteChar->canNotify())
    pRemoteChar->registerForNotify(notifyCallback);

  connected = true;
  DPRINTLN("✓ BLE connected");
}

// =============================
// SETUP
// =============================
void setup() {
  #if DEBUG
    Serial.begin(115200);
    delay(2000);
  #endif
  
  DPRINTLN("\n╔════════════════════════════════════════╗");
  DPRINTLN("║   SafeSound ESP32 Receiver Starting    ║");
  DPRINTLN("╚════════════════════════════════════════╝\n");

  pinMode(VIB_PIN, OUTPUT);
  setupRGBPWM();
  setupWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    setupFirebase();
    delay(1000);
    loadSettingsFromFirebase();
  }
  
  DPRINTLN("Initializing BLE...");
  BLEDevice::init("SafeSound");
  DPRINTLN("✓ BLE initialized\n");
  
  DPRINTLN("\n✓ Setup complete\n");
}

// =============================
// LOOP
// =============================
void loop() {
  // feed watchdog / allow RTOS housekeeping
  yield();

  // Reconnect WiFi if needed
  if (WiFi.status() != WL_CONNECTED) {
    DPRINTLN("WiFi disconnected, trying to reconnect...");
    setupWiFi(); // your existing function
    // small delay to avoid tight loop
    delay(200);
  }

  // If an alert was posted by the BLE callback, handle it here
  if (alertPending) {
    // copy the sound out of the shared buffer while interrupts disabled
    char soundBuf[32];
    noInterrupts();
    strncpy(soundBuf, alertSound, sizeof(soundBuf) - 1);
    soundBuf[sizeof(soundBuf)-1] = '\0';
    alertPending = false;
    interrupts();

    DPRINT(F("Sound detected (deferred): "));
    DPRINTLN(soundBuf);

    // refresh settings if stale
    if (millis() - lastSettingsRefresh > SETTINGS_REFRESH_INTERVAL) {
      DPRINTLN("Refreshing settings from Firebase...");
      if (WiFi.status() == WL_CONNECTED) {
        loadSettingsFromFirebase();
      }
    }

    // find matching setting
    Setting* s = findSettingByType(soundBuf);
    const char* finalColor = "ffffff";

    if (s) {
      DPRINTLN("Applying user setting...");
      DPRINT(" color: "); DPRINTLN(s->color);
      DPRINT(" vibration: "); DPRINTLN(s->vibration);
      finalColor = s->color;
      setLedColor(s->color);        // your existing function (uses ledcWrite)
      vibratePattern(s->vibration); // your existing function (blocking short patterns)
    } else {
      DPRINTLN("No setting found. Using defaults.");
      setLedColor("ffffff");
      vibratePattern("short");
    }

    // push alert to Firebase (non-blocking attempt — this may block briefly)
    if (WiFi.status() == WL_CONNECTED) {
      pushAlertLog(soundBuf, finalColor); // your existing function
    } else {
      DPRINTLN("Skipping Firebase push (no WiFi)");
    }

    // restore LED to "off" (common-anode) after the event
    // KEEP using the same channel numbers you've been using (0,1,2)
    ledcWrite(0, 255);
    ledcWrite(1, 255);
    ledcWrite(2, 255);

    DPRINTLN("Alert handled.");
  }
  // If BLE not connected, try to connect (keeps your previous behavior)
  if (!connected) {
    connectToDetector(); // your existing function
    // back off a bit so we don't hammer BLE connect
    delay(3000);
  }

  // Periodic settings refresh while idle
  if (millis() - lastSettingsRefresh > SETTINGS_REFRESH_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      DPRINTLN("Periodic settings refresh...");
      loadSettingsFromFirebase();
    }
  }

  delay(50); // keep loop responsive but not tight
}
