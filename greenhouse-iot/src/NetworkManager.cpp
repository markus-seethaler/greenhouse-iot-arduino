#include "NetworkManager.h"

NetworkManager::NetworkManager(const char* wifiSsid, const char* wifiPassword,
                               const char* mqttHost, int mqttPortNum, const char* devId)
  : ssid(wifiSsid), password(wifiPassword),
    mqttServer(mqttHost), mqttPort(mqttPortNum), deviceId(devId),
    mqttClient(wifiClient) {
  mqttTopic = String("greenhouse/") + deviceId;
}

void NetworkManager::connect() {
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
}

bool NetworkManager::connectMqtt() {
  mqttClient.setServer(mqttServer, mqttPort);

  Serial.print("Connecting to MQTT broker...");
  if (mqttClient.connect(deviceId)) {
    Serial.println(" connected");
    return true;
  } else {
    Serial.print(" failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void NetworkManager::loop() {
  if (!mqttClient.connected()) {
    connectMqtt();
  }
  mqttClient.loop();
}

bool NetworkManager::publishSensorData(float temperature, float humidity, int soilMoisture) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"soil_moisture\":%d}",
           temperature, humidity, soilMoisture);

  bool success = mqttClient.publish(mqttTopic.c_str(), payload);
  if (success) {
    Serial.print("Published to ");
    Serial.print(mqttTopic);
    Serial.print(": ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT publish failed");
  }
  return success;
}

bool NetworkManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool NetworkManager::isMqttConnected() {
  return mqttClient.connected();
}

String NetworkManager::getIPAddress() const {
  IPAddress ip = WiFi.localIP();
  return ip.toString();
}

WiFiSSLClient& NetworkManager::getSSLClient() {
  return sslClient;
}