#include "FirestoreManager.h"

FirestoreManager::FirestoreManager(const char* key, const char* email, const char* password, const char* fbProjectId)
  : apiKey(key), userEmail(email), userPassword(password), projectId(fbProjectId) {
  
  userAuth = new UserAuth(apiKey, userEmail, userPassword, 3000); // 3000 second expiry
  asyncClient = nullptr;
}

FirestoreManager::~FirestoreManager() {
  delete userAuth;
  delete asyncClient;
}

void FirestoreManager::begin(WiFiSSLClient& sslClient) {
  Serial.println("Initializing Firebase app...");
  
  // Create async client for Firebase operations
  asyncClient = new AsyncClientClass(sslClient);
  
  // Initialize Firebase app and authentication
  initializeApp(*asyncClient, app, getAuth(*userAuth), processData, "🔐 authTask");
  
  // Get Firestore Documents instance
  app.getApp<Firestore::Documents>(docs);
  
  Serial.println("Firebase initialized.");
}

void FirestoreManager::loop() {
  // Process Firebase operations
  app.loop();
}

bool FirestoreManager::isReady() {
  return app.ready();
}

void FirestoreManager::uploadSensorData(float temperature, float humidity, int soilMoisture, const String& timestamp) {
  // Check if we have valid sensor readings
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Invalid sensor readings, skipping upload");
    return;
  }

  Serial.println("Preparing to upload data to Firestore...");

  // Define the document path
  String documentPath = "devices/growbox/readings";

  // Create document
  Document<Values::Value> doc;

  // Add timestamp
  doc.add("timestamp", Values::Value(Values::TimestampValue(timestamp)));

  // STEP 1: Create the air section with humidity and temperature
  Values::MapValue airMap;

  // Humidity object with unit and value
  Values::MapValue humidityMap;
  humidityMap.add("unit", Values::Value(Values::StringValue("Percent")));
  humidityMap.add("value", Values::Value(Values::DoubleValue(humidity)));
  airMap.add("humidity", Values::Value(humidityMap));

  // Temperature object with unit and value
  Values::MapValue temperatureMap;
  temperatureMap.add("unit", Values::Value(Values::StringValue("Celsius")));
  temperatureMap.add("value", Values::Value(Values::DoubleValue(temperature)));
  airMap.add("temperature", Values::Value(temperatureMap));

  // STEP 2: Create the soil section with moisture
  Values::MapValue soilMap;

  // Moisture object with unit and value
  Values::MapValue moistureMap;
  moistureMap.add("unit", Values::Value(Values::StringValue("%")));
  moistureMap.add("value", Values::Value(Values::DoubleValue(soilMoisture)));
  soilMap.add("moisture", Values::Value(moistureMap));

  // STEP 3: Create the sensors map containing air and soil
  Values::MapValue sensorsMap;
  sensorsMap.add("air", Values::Value(airMap));
  sensorsMap.add("soil", Values::Value(soilMap));

  // Add the sensors map to the document
  doc.add("sensors", Values::Value(sensorsMap));

  Serial.println("Complete document prepared. Uploading...");

  // Upload to Firestore
  docs.createDocument(*asyncClient, Firestore::Parent(projectId), documentPath, DocumentMask(), doc, processData, "createDocumentTask");
}

void FirestoreManager::processData(AsyncResult &aResult) {
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