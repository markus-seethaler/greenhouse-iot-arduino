#include "SensorManager.h"

SensorManager::SensorManager(bool enableDht, int dhtPin,
                             int sensorCount, const int* pins, const char** names,
                             const int* airValues, const int* waterValues,
                             int retries, unsigned long retryDelay)
  : dht(dhtPin, DHT22), dhtEnabled(enableDht),
    soilSensorCount(min(sensorCount, MAX_SOIL_SENSORS)),
    soilNames(names),
    maxRetries(retries), retryDelayMs(retryDelay),
    humidity(0), temperature(0), dhtReadSuccess(false) {

  for (int i = 0; i < soilSensorCount; i++) {
    soilPins[i] = pins[i];
    soilAirValues[i] = airValues[i];
    soilWaterValues[i] = waterValues[i];
    soilMoisture[i] = 0;
    soilReadSuccess[i] = false;
  }
}

void SensorManager::begin() {
  if (dhtEnabled) {
    dht.begin();
  }
  for (int i = 0; i < soilSensorCount; i++) {
    pinMode(soilPins[i], INPUT);
  }
}

void SensorManager::readSensors() {
  // Read DHT22 sensor with retry mechanism
  if (dhtEnabled) {
    for (int i = 0; i < maxRetries; i++) {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();

      if (!isnan(humidity) && !isnan(temperature)) {
        break;
      }
      if (i < maxRetries - 1) {
        delay(retryDelayMs);
      }
    }

    dhtReadSuccess = !isnan(humidity) && !isnan(temperature);
    if (!dhtReadSuccess) {
      Serial.println("DHT sensor: read failed");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");
    }
  }

  // Read all soil moisture sensors
  readSoilMoisture();
  for (int i = 0; i < soilSensorCount; i++) {
    Serial.print(soilNames[i]);
    Serial.print(": ");
    if (soilReadSuccess[i]) {
      Serial.print(soilMoisture[i]);
      Serial.println(" %");
    } else {
      Serial.println("read failed");
    }
  }
}

void SensorManager::readSoilMoisture() {
  const int margin = 150;  // Allowed margin outside calibration range
  for (int i = 0; i < soilSensorCount; i++) {
    int rawValue = analogRead(soilPins[i]);
    int minValid = min(soilWaterValues[i], soilAirValues[i]) - margin;
    int maxValid = max(soilWaterValues[i], soilAirValues[i]) + margin;
    soilReadSuccess[i] = (rawValue >= minValid && rawValue <= maxValid);
    soilMoisture[i] = map(rawValue, soilAirValues[i], soilWaterValues[i], 0, 100);
    soilMoisture[i] = constrain(soilMoisture[i], 0, 100);
  }
}

float SensorManager::getHumidity() const {
  return humidity;
}

float SensorManager::getTemperature() const {
  return temperature;
}

int SensorManager::getSoilMoisture(int index) const {
  if (index >= 0 && index < soilSensorCount) {
    return soilMoisture[index];
  }
  return 0;
}

int SensorManager::getSoilSensorCount() const {
  return soilSensorCount;
}

const char* SensorManager::getSoilName(int index) const {
  if (index >= 0 && index < soilSensorCount) {
    return soilNames[index];
  }
  return "";
}

bool SensorManager::isDhtEnabled() const {
  return dhtEnabled;
}

bool SensorManager::isDhtReadSuccess() const {
  return dhtReadSuccess;
}

bool SensorManager::isSoilReadSuccess(int index) const {
  if (index >= 0 && index < soilSensorCount) {
    return soilReadSuccess[index];
  }
  return false;
}
