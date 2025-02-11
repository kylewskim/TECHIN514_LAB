#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// Ultrasonic sensor pin configuration
#define trigPin 9
#define echoPin 10

long duration;
int distance;
const int N = 5;  // Moving Average window size
int distanceBuffer[N] = {0};  // Array to store recent distance values
int bufferIndex = 0;  // Index for circular buffer
int filtered_distance = 0;  // Filtered distance using moving average

// BLE UUID (Change to a unique UUID)
#define SERVICE_UUID        "53ae2a94-fa52-463c-aaf2-7017554616a3"
#define CHARACTERISTIC_UUID "40a59a13-933c-4f60-a9d5-e4d0723f7224"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// Moving Average Filter Function
int movingAverageFilter(int newValue) {
    static int sum = 0;
    
    // Remove the oldest value and add the new one
    sum -= distanceBuffer[bufferIndex];  
    distanceBuffer[bufferIndex] = newValue;  
    sum += newValue;

    // Update buffer index
    bufferIndex = (bufferIndex + 1) % N;

    // Return the moving average value
    return sum / N;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // BLE initialization and setup
    BLEDevice::init("VICTORIAISGREAT");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it on your phone!");
}

void loop() {
    // Read ultrasonic sensor values
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;

    // Apply moving average filter
    filtered_distance = movingAverageFilter(distance);

    // Print raw and filtered values on the Serial Monitor
    Serial.print("raw : ");
    Serial.print(distance);
    Serial.print(" | denoised : ");
    Serial.println(filtered_distance);

    // Send data via BLE only if filtered_distance is less than 30 cm
    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval && filtered_distance < 30) {
            std::string strValue = std::to_string(filtered_distance);
            pCharacteristic->setValue(strValue);
            pCharacteristic->notify();
            Serial.print("Notify value: ");
            Serial.println(filtered_distance);
            previousMillis = currentMillis;  // Update timestamp
        }
    }

    // Handle BLE connection states
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(1000);
}