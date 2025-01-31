#include <Arduino.h>

// put function declarations here:
const int analogPin = A1;
const float referenceVoltage = 3.3;

void setup() {
  Serial.begin(115200);
}

void loop() {
  int analogValue = analogRead(analogPin);
  analogReadResolution(12);
  float voltage = (analogValue / 2095.0) * referenceVoltage;

  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  delay(500);
}