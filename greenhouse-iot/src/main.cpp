#include <Arduino.h>
#include "SensorManager.h"
#include "NetworkManager.h"
#include "TimeManager.h"
#include "FirestoreManager.h"
#include "firebase_secret.h"

// Configuration constants
const char *WIFI_SSID = SSID;
const char *WIFI_PASSWORD = WIFI_PW;
const int DHT_PIN = 2;
const int SOIL_PIN = A0;
const int SOIL_AIR_VALUE = 478;
const int SOIL_WATER_VALUE = 206;

// Timing constants
const long SENSOR_READ_INTERVAL = 2000;
const long UPLOAD_INTERVAL = 30000;

// Global instances
SensorManager sensorManager(DHT_PIN, SOIL_PIN, SOIL_AIR_VALUE, SOIL_WATER_VALUE);
NetworkManager networkManager(WIFI_SSID, WIFI_PASSWORD);
TimeManager timeManager;
FirestoreManager firestoreManager(API_KEY, USER_EMAIL, USER_PASSWORD, FIREBASE_PROJECT_ID);

// Timing variables
unsigned long previousSensorReadMillis = 0;
unsigned long lastUploadTime = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("DHT22 and Soil Moisture Sensor with Firestore");
  Serial.println("-----------------------------------------");
  
  // Initialize components
  sensorManager.begin();
  networkManager.connect();
  timeManager.begin();
  firestoreManager.begin(networkManager.getSSLClient());
  
  Serial.println("System initiliazed");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Update Firebase authentication
  firestoreManager.loop();
  
  // Update time periodically
  timeManager.update();
  
 // Read sensors at regular intervals
 /*
  if (currentMillis - previousSensorReadMillis >= SENSOR_READ_INTERVAL) {
    previousSensorReadMillis = currentMillis;
    sensorManager.readSensors();
  }
  */
  // Upload data to Firestore at regular intervals
  if (firestoreManager.isReady() && (currentMillis - lastUploadTime >= UPLOAD_INTERVAL)) {
    sensorManager.readSensors();
    // Get formatted timestamp for this upload
    String timestamp = timeManager.getFormattedTimestamp();
    
    // Upload sensor data with timestamp
    firestoreManager.uploadSensorData(
      sensorManager.getTemperature(),
      sensorManager.getHumidity(),
      sensorManager.getSoilMoisture(),
      timestamp
    );
    
    lastUploadTime = currentMillis;
  }
}