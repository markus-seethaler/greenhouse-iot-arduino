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

bool NetworkManager::publishSensorData(SensorManager& sensors) {
  if (!mqttClient.connected()) {
    return false;
  }

  char payload[256];
  int pos = 0;
  pos += snprintf(payload + pos, sizeof(payload) - pos, "{");

  bool hasData = false;

  // Add DHT data if enabled and read succeeded
  if (sensors.isDhtEnabled() && sensors.isDhtReadSuccess()) {
    pos += snprintf(payload + pos, sizeof(payload) - pos,
                    "\"temperature\":%.1f,\"humidity\":%.1f",
                    sensors.getTemperature(), sensors.getHumidity());
    hasData = true;
  }

  // Add soil moisture sensors
  for (int i = 0; i < sensors.getSoilSensorCount(); i++) {
    if (sensors.isSoilReadSuccess(i)) {
      pos += snprintf(payload + pos, sizeof(payload) - pos,
                      "%s\"%s\":%d",
                      hasData ? "," : "",
                      sensors.getSoilName(i),
                      sensors.getSoilMoisture(i));
      hasData = true;
    }
  }

  pos += snprintf(payload + pos, sizeof(payload) - pos, "}");

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
