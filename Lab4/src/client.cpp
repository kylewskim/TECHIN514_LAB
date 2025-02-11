#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// The remote service and characteristic UUIDs (must match the server)
static BLEUUID serviceUUID("53ae2a94-fa52-463c-aaf2-7017554616a3");
static BLEUUID charUUID("40a59a13-933c-4f60-a9d5-e4d0723f7224");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Variables to track max and min received distance values
int maxDistance = 0;
int minDistance = 0;
bool isFirstReading = false;  // Renamed from 'initialized' to avoid conflict

// Callback function to handle received BLE notifications
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
    // Convert received data to string, then to integer
    std::string receivedStr(reinterpret_cast<char*>(pData), length);
    int receivedDistance = atoi(receivedStr.c_str());

    // If this is the first received value, initialize max and min
    if (!isFirstReading) {
        maxDistance = receivedDistance;
        minDistance = receivedDistance;
        isFirstReading = true;  // Renamed variable
    }

    // Update max and min distance values
    if (receivedDistance > maxDistance) maxDistance = receivedDistance;
    if (receivedDistance < minDistance) minDistance = receivedDistance;

    // Print data in the required format: Current | Max | Min
    Serial.print("Current: ");
    Serial.print(receivedDistance);
    Serial.print(" | Max: ");
    Serial.print(maxDistance);
    Serial.print(" | Min: ");
    Serial.println(minDistance);
}

// BLE client callbacks
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server.");
  }
};

// Function to connect to BLE Server
bool connectToServer() {
    Serial.print("Connecting to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");
    pClient->setMTU(517); // Request maximum MTU size

    // Get the remote service
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found service");

    // Get the remote characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found characteristic");

    // Read initial characteristic value
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("Initial characteristic value: ");
      Serial.println(value.c_str());
    }

    // Register notification callback
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

// BLE Scan callback class
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found BLE Device: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");

  BLEDevice::init("");

  // Start BLE scanning
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // If we found a BLE server, connect to it
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to BLE Server.");
    } else {
      Serial.println("Failed to connect to BLE Server.");
    }
    doConnect = false;
  }

  // If connected, send a message periodically
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    // Write value to characteristic
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // Restart scan if disconnected
  }

  delay(1000);
}