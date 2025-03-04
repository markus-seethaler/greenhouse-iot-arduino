#include "SensorManager.h"

SensorManager::SensorManager(int dhtPin, int soilMoisturePin, int airValue, int waterValue)
  : dht(dhtPin, DHT22), soilPin(soilMoisturePin), 
    soilAirValue(airValue), soilWaterValue(waterValue),
    humidity(0), temperature(0), soilMoisture(0) {
}

void SensorManager::begin() {
  dht.begin();
  pinMode(soilPin, INPUT);
}

void SensorManager::readSensors() {
  // Read DHT22 sensor with retry mechanism
  for (int i = 0; i < 3; i++) { // Try up to 3 times
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature)) {
      break; // Reading successful, exit retry loop
    }
    delay(1000); // Wait before retrying
  }

  // Read soil moisture
  readSoilMoisture();

  // Check if DHT22 readings failed after all retries
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor after multiple attempts!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C\t");

    Serial.print("Soil Moisture: ");
    Serial.print(soilMoisture);
    Serial.println(" %");
  }
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