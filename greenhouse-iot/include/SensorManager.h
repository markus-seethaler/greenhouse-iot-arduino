#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class SensorManager {
private:
  DHT dht;
  int soilPin;
  int soilAirValue;
  int soilWaterValue;
  
  // Latest sensor readings
  float humidity;
  float temperature;
  int soilMoisture;
  
  // Read soil moisture and map to percentage
  void readSoilMoisture();

public:
  SensorManager(int dhtPin, int soilMoisturePin, int airValue, int waterValue);
  
  // Initialize sensors
  void begin();
  
  // Read all sensors
  void readSensors();
  
  // Getters for sensor values
  float getHumidity() const;
  float getTemperature() const;
  int getSoilMoisture() const;
};

#endif // SENSOR_MANAGER_H