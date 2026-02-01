#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

#define MAX_SOIL_SENSORS 6

class SensorManager {
private:
  DHT dht;
  bool dhtEnabled;
  int soilSensorCount;
  int soilPins[MAX_SOIL_SENSORS];
  int soilAirValues[MAX_SOIL_SENSORS];
  int soilWaterValues[MAX_SOIL_SENSORS];
  const char** soilNames;
  int maxRetries;
  unsigned long retryDelayMs;

  // Latest sensor readings
  float humidity;
  float temperature;
  bool dhtReadSuccess;
  int soilMoisture[MAX_SOIL_SENSORS];
  bool soilReadSuccess[MAX_SOIL_SENSORS];

  // Read soil moisture and map to percentage
  void readSoilMoisture();

public:
  SensorManager(bool enableDht, int dhtPin,
                int sensorCount, const int* pins, const char** names,
                const int* airValues, const int* waterValues,
                int retries = 3, unsigned long retryDelay = 500);

  // Initialize sensors
  void begin();

  // Read all sensors
  void readSensors();

  // Getters for sensor values
  float getHumidity() const;
  float getTemperature() const;
  int getSoilMoisture(int index) const;
  int getSoilSensorCount() const;
  const char* getSoilName(int index) const;
  bool isDhtEnabled() const;
  bool isDhtReadSuccess() const;
  bool isSoilReadSuccess(int index) const;
};

#endif // SENSOR_MANAGER_H
