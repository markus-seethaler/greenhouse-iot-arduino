#ifndef FIRESTORE_MANAGER_H
#define FIRESTORE_MANAGER_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <FirebaseClient.h>

class FirestoreManager {
private:
  const char* apiKey;
  const char* userEmail;
  const char* userPassword;
  const char* projectId;
  
  // Firebase components
  UserAuth* userAuth;
  FirebaseApp app;
  AsyncClientClass* asyncClient;
  Firestore::Documents docs;
  
  // Callback for Firebase operations
  static void processData(AsyncResult &aResult);
  
public:
  FirestoreManager(const char* apiKey, const char* email, const char* password, const char* projectId);
  ~FirestoreManager();
  
  // Initialize Firebase app and authentication
  void begin(WiFiSSLClient& sslClient);
  
  // Process Firebase operations
  void loop();
  
  // Check if Firebase is ready for operations
  bool isReady();
  
  // Upload sensor data to Firestore
  void uploadSensorData(float temperature, float humidity, int soilMoisture, const String& timestamp);
};

#endif // FIRESTORE_MANAGER_H