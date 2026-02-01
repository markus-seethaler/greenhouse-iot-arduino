#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <PubSubClient.h>
#include "SensorManager.h"

class NetworkManager {
private:
  const char* ssid;
  const char* password;
  const char* mqttServer;
  int mqttPort;
  const char* deviceId;

  WiFiClient wifiClient;
  WiFiSSLClient sslClient;
  PubSubClient mqttClient;

  String mqttTopic;

public:
  NetworkManager(const char* wifiSsid, const char* wifiPassword,
                 const char* mqttHost, int mqttPortNum,
                 const char* location, const char* devId);

  // Connect to WiFi network
  void connect();

  // Connect to MQTT broker
  bool connectMqtt();

  // Ensure MQTT stays connected (call in loop)
  void loop();

  // Publish sensor data as JSON (dynamic based on sensors)
  bool publishSensorData(SensorManager& sensors);

  // Check connection status
  bool isConnected() const;
  bool isMqttConnected();

  // Get IP address as string
  String getIPAddress() const;

  // Get SSL client for secure connections
  WiFiSSLClient& getSSLClient();
};

#endif // NETWORK_MANAGER_H
