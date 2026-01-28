#include <Arduino.h>
#include "SensorManager.h"
#include "NetworkManager.h"
#include "secrets.h"

// Sensor pin configuration
const int DHT_PIN = 2;
const int SOIL_PIN = A0;
const int SOIL_AIR_VALUE = 478;
const int SOIL_WATER_VALUE = 206;

// Timing constants
const long READ_INTERVAL = 5000;

// Global instances
SensorManager sensorManager(DHT_PIN, SOIL_PIN, SOIL_AIR_VALUE, SOIL_WATER_VALUE);
NetworkManager networkManager(WIFI_SSID, WIFI_PASSWORD);

// Timing variables
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  Serial.println("DHT22 and Soil Moisture Sensor");
  Serial.println("------------------------------");

  sensorManager.begin();
  networkManager.connect();

  Serial.println("System initialized");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= READ_INTERVAL) {
    sensorManager.readSensors();
    lastReadTime = currentMillis;
  }
}