#include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include <FirebaseClient.h>
#include <NTPClient.h>

// Include secrets from separate file that won't be committed to git
#include "firebase_secret.h"

// WiFi credentials
const char* ssid = "Kindle";
const char* password = "Helbing4Lyfe";

// Define pin and sensor type
#define DHTPIN 2
#define DHTTYPE DHT22
#define SOIL_PIN A0

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables for soil moisture readings
int soilMoistureValue = 0;
int soilMoisturePercent = 0;

// Calibration values for the soil moisture sensor
const int airValue = 478;
const int waterValue = 206;

// Variables for timing
unsigned long previousMillis = 0;
const long sensorInterval = 2000;
const long uploadInterval = 30000;
unsigned long lastUploadTime = 0;

// Store the latest sensor readings
float latestHumidity = 0;
float latestTemperature = 0;
int latestSoilMoisture = 0;

// Initialize the WiFi client
WiFiSSLClient ssl_client;

const long utcOffsetWinter = 3600; // Offset from UTC in seconds (3600 seconds = 1h) -- UTC+1 (Central European Winter Time)
const long utcOffsetSummer = 7200; // Offset from UTC in seconds (7200 seconds = 2h) -- UTC+2 (Central European Summer Time)
unsigned long lastupdate = 0UL;
 
// Define NTP Client to get time
WiFiUDP udpSocket;
NTPClient ntpClient(udpSocket, "pool.ntp.org", utcOffsetWinter);

// Setup for Firebase
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) */);
FirebaseApp app;

// Create an AsyncClient for Firebase interactions
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

// Create a Firestore Documents instance
Firestore::Documents Docs;

void processData(AsyncResult &aResult);
void readSensors();
void uploadToFirestore();

void setup() {
  Serial.begin(9600);
  
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("DHT22 and Soil Moisture Sensor with Firestore");
  Serial.println("-----------------------------------------");
  
  dht.begin();
  pinMode(SOIL_PIN, INPUT);
  
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  ntpClient.begin();
  
  // Note: WiFiSSLClient doesn't need setInsecure() like other SSL clients
  
  Serial.println("Initializing Firebase app...");
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  
  app.getApp<Firestore::Documents>(Docs);
  
  Serial.println("Sensors initialized. Starting readings...");
  Serial.println();
}

void loop() {
  // To maintain the authentication and async tasks
  app.loop();
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= sensorInterval) {
    previousMillis = currentMillis;
    readSensors();
  }
  
  if (app.ready() && (currentMillis - lastUploadTime >= uploadInterval)) {
    uploadToFirestore();
    lastUploadTime = currentMillis;
    Serial.print(ntpClient.getHours());
    Serial.print(":");
    Serial.print(ntpClient.getMinutes());
    Serial.print(":");
    Serial.println(ntpClient.getSeconds());
  }
}

void readSensors() {
  // Read DHT22 sensor with retry mechanism
  for (int i = 0; i < 3; i++) {  // Try up to 3 times
    latestHumidity = dht.readHumidity();
    latestTemperature = dht.readTemperature();
    
    if (!isnan(latestHumidity) && !isnan(latestTemperature)) {
      break;  // Reading successful, exit retry loop
    }
    
    delay(1000);  // Wait before retrying
  }

  // Read soil moisture sensor
  soilMoistureValue = analogRead(SOIL_PIN);
  latestSoilMoisture = map(soilMoistureValue, airValue, waterValue, 0, 100);
  latestSoilMoisture = constrain(latestSoilMoisture, 0, 100);
  
  // Check if DHT22 readings failed after all retries
  if (isnan(latestHumidity) || isnan(latestTemperature)) {
    Serial.println("Failed to read from DHT sensor after multiple attempts!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(latestHumidity);
    Serial.print(" %\t");
    
    Serial.print("Temperature: ");
    Serial.print(latestTemperature);
    Serial.print(" °C\t");
    
    Serial.print("Soil Moisture: ");
    Serial.print(latestSoilMoisture);
    Serial.println(" %");
  }
}

void uploadToFirestore() {
  // Check if we have valid sensor readings
  if (isnan(latestHumidity) || isnan(latestTemperature)) {
    Serial.println("Invalid sensor readings, skipping upload");
    return;
  }
  
  Serial.println("Preparing to upload data to Firestore...");
  
  // Define the document path
  String documentPath = "devices/growbox/readings";
  
  // Create document
  Document<Values::Value> doc;
  
  // Add timestamp
  unsigned long timestamp = millis() / 1000;
  doc.add("timestamp", Values::Value(Values::IntegerValue(timestamp)));
  
  // STEP 1: Create the air section with humidity and temperature
  Values::MapValue airMap;
  
  // Humidity object with unit and value
  Values::MapValue humidityMap;
  humidityMap.add("unit", Values::Value(Values::StringValue("Percent")));
  humidityMap.add("value", Values::Value(Values::DoubleValue(latestHumidity)));
  airMap.add("humidity", Values::Value(humidityMap));
  
  // Temperature object with unit and value
  Values::MapValue temperatureMap;
  temperatureMap.add("unit", Values::Value(Values::StringValue("Celsius")));
  temperatureMap.add("value", Values::Value(Values::DoubleValue(latestTemperature)));
  airMap.add("temperature", Values::Value(temperatureMap));
  
  // STEP 2: Create the soil section with moisture
  Values::MapValue soilMap;
  
  // Moisture object with unit and value
  Values::MapValue moistureMap;
  moistureMap.add("unit", Values::Value(Values::StringValue("%")));
  moistureMap.add("value", Values::Value(Values::DoubleValue(latestSoilMoisture)));
  soilMap.add("moisture", Values::Value(moistureMap));
  
  // STEP 3: Create the sensors map containing air and soil
  Values::MapValue sensorsMap;
  sensorsMap.add("air", Values::Value(airMap));
  sensorsMap.add("soil", Values::Value(soilMap));
  
  // Add the sensors map to the document
  doc.add("sensors", Values::Value(sensorsMap));
  
  Serial.println("Complete document prepared. Uploading...");
  
  // Upload to Firestore
  Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath, DocumentMask(), doc, processData, "createDocumentTask");
}

void processData(AsyncResult &aResult) {
  // Handle Firebase async results
  
  // Exit if no result available
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()) {
    Serial.print("Event task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.print(aResult.eventLog().message().c_str());
    Serial.print(", code: ");
    Serial.println(aResult.eventLog().code());
  }

  if (aResult.isDebug()) {
    Serial.print("Debug task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.println(aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Serial.print("Error task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.print(aResult.error().message().c_str());
    Serial.print(", code: ");
    Serial.println(aResult.error().code());
  }

  if (aResult.available()) {
    Serial.print("Task: ");
    Serial.print(aResult.uid().c_str());
  }
}