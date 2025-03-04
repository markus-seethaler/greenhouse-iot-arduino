#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFiS3.h>

class NetworkManager {
private:
  const char* ssid;
  const char* password;
  WiFiSSLClient sslClient;
  
public:
  NetworkManager(const char* wifiSsid, const char* wifiPassword);
  
  // Connect to WiFi network
  void connect();
  
  // Check connection status
  bool isConnected() const;
  
  // Get IP address as string
  String getIPAddress() const;
  
  // Get SSL client for secure connections
  WiFiSSLClient& getSSLClient();
};

#endif // NETWORK_MANAGER_H