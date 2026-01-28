#include "SensorManager.h"

SensorManager::SensorManager(int dhtPin, int soilMoisturePin, int airValue, int waterValue,
                             int retries, unsigned long retryDelay)
  : dht(dhtPin, DHT22), soilPin(soilMoisturePin),
    soilAirValue(airValue), soilWaterValue(waterValue),
    maxRetries(retries), retryDelayMs(retryDelay),
    humidity(0), temperature(0), soilMoisture(0) {
}

void SensorManager::begin() {
  dht.begin();
  pinMode(soilPin, INPUT);
}

void SensorManager::readSensors() {
  // Read DHT22 sensor with retry mechanism
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

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT sensor: read failed");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  // Read soil moisture independently
  readSoilMoisture();
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.println(" %");
}

void SensorManager::readSoilMoisture() {
  int rawValue = analogRead(soilPin);
  soilMoisture = map(rawValue, soilAirValue, soilWaterValue, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);
}

float SensorManager::getHumidity() const {
  return humidity;
}

float SensorManager::getTemperature() const {
  return temperature;
}

int SensorManager::getSoilMoisture() const {
  return soilMoisture;
}