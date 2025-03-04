#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

class TimeManager {
private:
  WiFiUDP udpClient;
  NTPClient* ntpClient;
  
  const long utcOffsetWinter = 3600; // UTC+1 (Central European Winter Time)
  const long utcOffsetSummer = 7200; // UTC+2 (Central European Summer Time)
  
  unsigned long lastUpdate = 0;
  const unsigned long updateInterval = 60000; // Update every minute
  
  // Helper functions
  String formatTimeComponent(int component);
  String formatLocalTimestamp(unsigned long epochTime);
  
public:
  TimeManager();
  ~TimeManager();
  
  // Initialize NTP client
  void begin();
  
  // Update time periodically and adjust for DST
  void update();
  
  // Check if Daylight Saving Time is in effect
  bool isDST();
  
  // Get formatted timestamp for Firestore
  String getFormattedTimestamp();
};

#endif // TIME_MANAGER_H