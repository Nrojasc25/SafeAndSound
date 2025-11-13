#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h>


// ==== Pin map ====
const int LED_RED    = 19;
const int LED_BLUE   = 21;
const int LED_YELLOW = 22;
const int LED_GREEN  = 23;
const int VIB_PIN    = 25;


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
const unsigned long CONNECTION_RETRY_DELAY = 5000; // 5 seconds


// ==== LED + Vibration helpers ====
void allLedsOff() {
  digitalWrite(LED_RED, HIGH);    
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_GREEN, HIGH);
}


// For now, pick LED based on BLE color hex
void setLedColor(String colorHex) {
  allLedsOff();
  colorHex.toLowerCase();


  // match Firebase color hex to approximate LED
  if (colorHex == "ff2196f3") digitalWrite(LED_BLUE, LOW);       // blue
  else if (colorHex == "ffffeb3b") digitalWrite(LED_YELLOW, LOW); // amber/yellow
  else if (colorHex == "ff4caf50") digitalWrite(LED_GREEN, LOW);  // green
  else digitalWrite(LED_RED, LOW); // default red for unknown
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


// ==== Notification Callback ====
void notifyCallback(BLERemoteCharacteristic* rc, uint8_t* data, size_t len, bool isNotify) {
  String msg((char*)data, len);
  msg.trim();
  Serial.println("Notification received: " + msg);


  // Expected format now: color|vibration
  int delim = msg.indexOf('|');
  if (delim < 0) {
    Serial.println("Unknown message format, skipping");
    return;
  }


  String color = msg.substring(0, delim);
  String vibration = msg.substring(delim + 1);


  Serial.printf("Color=%s | Pattern=%s\n", color.c_str(), vibration.c_str());


  setLedColor(color);
  vibratePattern(vibration);
  delay(500);
  allLedsOff();
}


// ==== Client Disconnect Callback ====
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to BLE server");
  }


  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server, restarting scan...");
    BLEDevice::getScan()->start(0, nullptr, false);
  }
};


// ==== Connect to Detector ====
void connectToDetector(BLEAdvertisedDevice advertisedDevice) {
  Serial.println("Attempting to connect to Detector...");


  BLEDevice::getScan()->stop();
  delay(500);


  BLEAddress serverAddress = advertisedDevice.getAddress();
  Serial.print("Connecting to address: ");
  Serial.println(serverAddress.toString().c_str());


  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    Serial.println("Created BLE client");
  }


  bool success = pClient->connect(serverAddress);
  if (!success) {
    Serial.println("Connection failed!");
    pClient->disconnect();
    lastConnectionAttempt = millis();
    BLEDevice::getScan()->start(0, nullptr, false);
    return;
  }


  Serial.println("Connected! Discovering services...");
  delay(1000);


  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (!pRemoteService) {
    Serial.println("Service UUID not found!");
    pClient->disconnect();
    lastConnectionAttempt = millis();
    BLEDevice::getScan()->start(0, nullptr, false);
    return;
  }
  Serial.println("Service found");


  pRemoteChar = pRemoteService->getCharacteristic(charUUID);
  if (!pRemoteChar) {
    Serial.println("Characteristic UUID not found!");
    pClient->disconnect();
    lastConnectionAttempt = millis();
    BLEDevice::getScan()->start(0, nullptr, false);
    return;
  }
  Serial.println("Characteristic found");


  if (pRemoteChar->canNotify()) {
    pRemoteChar->registerForNotify(notifyCallback);
    Serial.println("Subscribed to notifications");
  } else {
    Serial.println("Characteristic cannot notify");
  }


  connected = true;
  Serial.println("Receiver ready and listening");


  setLedColor("ff4caf50");  // green LED for connected
  delay(500);
  allLedsOff();
}


// ==== Scan Callback ====
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (connected || shouldConnect) return;
   
    Serial.print("Found device: ");
    Serial.println(advertisedDevice.toString().c_str());
   
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.println("Detector found via UUID");
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
 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(VIB_PIN, OUTPUT);
  allLedsOff();


  Serial.println("Receiver BLE Client starting...");
  BLEDevice::init("SafeSound_Receiver");


  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);


  Serial.println("Starting continuous scan...");
  pScan->start(0, nullptr, false);
}


// ==== Loop ====
void loop() {
  if (shouldConnect && !connected && targetDevice != nullptr) {
    if (millis() - lastConnectionAttempt > CONNECTION_RETRY_DELAY) {
      Serial.println("Queued connection triggered");
      connectToDetector(*targetDevice);
      delete targetDevice;
      targetDevice = nullptr;
      shouldConnect = false;
    }
  }


  if (connected) {
    if (!pClient->isConnected()) {
      Serial.println("Connection lost, restarting scan...");
      connected = false;
      BLEDevice::getScan()->start(0, nullptr, false);
    } else {
      Serial.println("Connected and listening...");
    }
  } else {
    Serial.println("Scanning for detector...");
  }


  delay(2000);
}


