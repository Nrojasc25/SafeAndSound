#include <WiFi.h>
#include "time.h"
#include <Firebase_ESP_Client.h>

// Config
#define WIFI_SSID "ufdevice"
#define WIFI_PASSWORD "gogators"
#define API_KEY "AIzaSyBIdBSaVKSNaA64Y7rfnucOdEI9K71m4_M"
#define DATABASE_URL "https://safeandsound-ea1e1-default-rtdb.firebaseio.com/"  
#define USER_ID "P4OTxL5xMVSeljSaqTN3PINrEh12"  // demo UID (nati.rojaschaverri@gmail.com)

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600; // adjust to your timezone
const int daylightOffset_sec = 3600;  // DST offset

struct Setting {
  char type[16];
  char color[8];
  char vibration[8];
};

#define MAX_SETTINGS 5
Setting settingsList[MAX_SETTINGS];
size_t settingsCount = 0;

// Timing
unsigned long lastDetectTime = 0;
const unsigned long DETECT_PERIOD = 10000; // 10s

// Simulated sounds 
const char* labels[] = {"Fire Alarm","Siren","Doorbell"};
uint8_t simIndex = 0;

// Firebase 
void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "nati.rojaschaverri@gmail.com";
  auth.user.password = "SafeAndSoundPassw0rd!";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(1000);
  Serial.println("Firebase initialized.");
}

// Load settings
void loadUserSettings() {
  Serial.println("Loading user settings...");
  settingsCount = 0;
  
  // Try to fetch settings 0-5 
  for (int i = 0; i < MAX_SETTINGS; i++) {
    char path[80];
    snprintf(path, sizeof(path), "/users/%s/settings/%d", USER_ID, i);
    if (Firebase.RTDB.getJSON(&fbdo, path)) {
      String jsonStr = fbdo.jsonString();
      if (jsonStr.length() > 0) {
        FirebaseJson json;
        json.setJsonData(jsonStr);
        FirebaseJsonData val;
        // type
        if (json.get(val, "type")) {
          strncpy(settingsList[settingsCount].type, val.stringValue.c_str(), sizeof(settingsList[settingsCount].type) - 1);
        } else {
          strncpy(settingsList[settingsCount].type, "Unknown", sizeof(settingsList[settingsCount].type) - 1);
        }
        settingsList[settingsCount].type[sizeof(settingsList[settingsCount].type) - 1] = '\0';
        // color
        if (json.get(val, "color")) {
          strncpy(settingsList[settingsCount].color, val.stringValue.c_str(), sizeof(settingsList[settingsCount].color) - 1);
        } else {
          strncpy(settingsList[settingsCount].color, "ffffff", sizeof(settingsList[settingsCount].color) - 1);
        }
        settingsList[settingsCount].color[sizeof(settingsList[settingsCount].color) - 1] = '\0';
        // vibrationPattern
        if (json.get(val, "vibrationPattern")) {
          strncpy(settingsList[settingsCount].vibration, val.stringValue.c_str(), sizeof(settingsList[settingsCount].vibration) - 1);
        } else {
          strncpy(settingsList[settingsCount].vibration, "s", sizeof(settingsList[settingsCount].vibration) - 1);
        }
        settingsList[settingsCount].vibration[sizeof(settingsList[settingsCount].vibration) - 1] = '\0';        
        settingsCount++;
      } else {
        // Empty JSON, no more settings
        break;
      }
    } else {
      // Setting doesn't exist, no more settings
      break;
    }
    
    delay(100);  // Small delay between requests to avoid overwhelming Firebase
  }
  Serial.print("Total loaded settings: ");
  Serial.println(settingsCount);
}

// Helpers 
Setting findSettingByType(const char* type) {
  for (size_t i = 0; i < settingsCount; i++) {
    if (strcasecmp(settingsList[i].type, type) == 0) return settingsList[i];
  }
  Setting none = {"Unknown","ffffff","s"};
  return none;
}

void sendBLEMessage(const Setting &s) {
  // Instead of BLE, just print
  Serial.print("BLE Msg -> ");
  Serial.print(s.type);
  Serial.print("|");
  Serial.print(s.color);
  Serial.print("|");
  Serial.println(s.vibration);
}

// Push alert log
void pushAlertLog(const char* soundType) {
  char path[64];
  snprintf(path, sizeof(path), "/users/%s/alerts", USER_ID);

  FirebaseJson json;
  json.set("sound", soundType);
  json.set("timestamp", getUnixMillis());
  json.set("device", "ESP32_Detector");

  if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
    Serial.print("Alert pushed: "); Serial.println(soundType);
  } else {
    Serial.print("Failed to push alert: "); Serial.println(fbdo.errorReason());
  }
}

long long getUnixMillis() {
  time_t now;
  time(&now); // seconds since epoch
  return ((long long)now) * 1000LL;
}

// Simulation 
const char* simulateClassification() {
  const char* label = labels[simIndex % 3];
  simIndex++;
  return label;
}

//Main

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  // Init NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  setupFirebase();

  char path[64];
  snprintf(path, sizeof(path), "/users/%s/settings", USER_ID);

  if (Firebase.RTDB.getJSON(&fbdo, path)) {
      Serial.println("JSON fetched:");
      Serial.println(fbdo.jsonString());
  } else {
      Serial.print("Failed to get JSON: ");
      Serial.println(fbdo.errorReason());
  }

  loadUserSettings();
  lastDetectTime = millis();
} 

void loop() {
  unsigned long now = millis();
  if (now - lastDetectTime > DETECT_PERIOD) {
    lastDetectTime = now;

    // Simulate detection
    const char* detectedType = simulateClassification();
    Serial.print("Detected: "); Serial.println(detectedType);

    // Match settings
    Setting match = findSettingByType(detectedType);
    sendBLEMessage(match);
    pushAlertLog(detectedType);
  }
}


