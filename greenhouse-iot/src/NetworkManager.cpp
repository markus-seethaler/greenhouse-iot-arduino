#include "NetworkManager.h"

NetworkManager::NetworkManager(const char* wifiSsid, const char* wifiPassword)
  : ssid(wifiSsid), password(wifiPassword) {
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

bool NetworkManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::getIPAddress() const {
  IPAddress ip = WiFi.localIP();
  return ip.toString();
}

WiFiSSLClient& NetworkManager::getSSLClient() {
  return sslClient;
}