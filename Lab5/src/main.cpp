#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h> // Install Firebase ESP32 library
#include "esp_sleep.h"

#define trigPin 9
#define echoPin 10

// Firebase Configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const char* ssid     = "UW MPSK";
const char* password = "NF~5$yHVr$"; // Replace with your actual password

// Firebase Credentials
#define DATABASE_URL "https://techin514-bml-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyD03sgKMDk0KVoXdrhRU2ku72FPHjRsyCo"

// Ultrasonic Sensor Variables
long duration;
int distance;

void measureUltrasonic() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void deepSleepMode() {
  Serial.println("\n[5] Deep Sleep Mode");

  Serial.println("Going into deep sleep for 2 seconds...");
  esp_sleep_enable_timer_wakeup(60 * 1000000); // Sleep for 60 seconds
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

    config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-in successful.");
  } else {
    Serial.println("Firebase sign-in failed.");
    return;
  }
}

void loop() {
  // Sequence of modes to match the graph behavior
    Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  for (int i = 0; i < 10; i++) { // Run ultrasonic sensor while sending data to Firebase
    measureUltrasonic();

    // Send data to Firebase
    if (Firebase.setInt(fbdo, "/sensor/distance", distance)) {
      Serial.println("Data sent to Firebase successfully.");
    } else {
      Serial.println("Failed to send data to Firebase.");
      Serial.println(fbdo.errorReason());
    }

    delay(100); // Simulate Firebase transmission mode
  }
  deepSleepMode();
}