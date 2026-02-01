#include <Arduino.h>
#include "config.h"
#include "SensorManager.h"
#include "NetworkManager.h"
#include "secrets.h"

// Global instances
SensorManager sensorManager(
  DHT_ENABLED, DHT_PIN,
  SOIL_SENSOR_COUNT, SOIL_PINS, SOIL_NAMES,
  SOIL_AIR_VALUES, SOIL_WATER_VALUES
);
NetworkManager networkManager(WIFI_SSID, WIFI_PASSWORD, MQTT_SERVER, MQTT_PORT, DEVICE_LOCATION, DEVICE_ID);

// Timing variables
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  Serial.println("Greenhouse Sensor System");
  Serial.println("------------------------");

  sensorManager.begin();
  networkManager.connect();
  networkManager.connectMqtt();

  Serial.println("System initialized");
}

void loop() {
  networkManager.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - lastReadTime >= READ_INTERVAL_MS) {
    sensorManager.readSensors();
    networkManager.publishSensorData(sensorManager);
    lastReadTime = currentMillis;
  }
}
