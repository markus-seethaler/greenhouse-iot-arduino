#include "TimeManager.h"

TimeManager::TimeManager() {
  ntpClient = new NTPClient(udpClient, "pool.ntp.org", utcOffsetWinter);
}

TimeManager::~TimeManager() {
  delete ntpClient;
}

void TimeManager::begin() {
  ntpClient->begin();
  update(); // Force initial update
}

void TimeManager::update() {
  unsigned long currentMillis = millis();
  
  // Update NTP client periodically
  if (currentMillis - lastUpdate >= updateInterval) {
    ntpClient->update();
    lastUpdate = currentMillis;
    
    // Adjust for daylight saving time
    if (isDST()) {
      ntpClient->setTimeOffset(utcOffsetSummer);
    } else {
      ntpClient->setTimeOffset(utcOffsetWinter);
    }
  }
}

bool TimeManager::isDST() {
  // Get current date information from NTP
  unsigned long epochTime = ntpClient->getEpochTime();
  time_t rawtime = (time_t)epochTime;
  struct tm *ti;
  ti = gmtime(&rawtime);

  int month = ti->tm_mon + 1; // tm_mon is 0-11
  int day = ti->tm_mday;

  // Simple European DST rule (last Sunday of March to last Sunday of October)
  if (month > 3 && month < 10) {
    return true; // April to September
  } else if (month == 3) {
    // March: DST starts on the last Sunday
    return day >= 25; // Approximation for last week of March
  } else if (month == 10) {
    // October: DST ends on the last Sunday
    return day < 25; // Approximation for before the last week of October
  }
  return false;
}

String TimeManager::formatTimeComponent(int component) {
  return component < 10 ? "0" + String(component) : String(component);
}

String TimeManager::formatLocalTimestamp(unsigned long epochTime) {
  // Convert epoch time to time_t
  time_t rawtime = (time_t)epochTime;
  struct tm *timeinfo;
  
  // Get time info in UTC
  timeinfo = gmtime(&rawtime);
  
  // Apply the timezone offset manually
  int tzOffset = isDST() ? utcOffsetSummer : utcOffsetWinter;
  rawtime += tzOffset;
  timeinfo = gmtime(&rawtime);
  
  // Format as YYYY-MM-DDTHH:MM:SS+01:00 or +02:00 (RFC 3339 format with timezone)
  char buffer[30];
  char tzString[10];
  
  if (isDST()) {
    strcpy(tzString, "+02:00"); // UTC+2 for summer time
  } else {
    strcpy(tzString, "+01:00"); // UTC+1 for winter time
  }
  
  strftime(buffer, 30, "%Y-%m-%dT%H:%M:%S", timeinfo);
  
  // Append the timezone string
  String result = String(buffer) + tzString;
  
  return result;
}

String TimeManager::getFormattedTimestamp() {
  // Ensure time is updated
  ntpClient->update();
  
  // Get current time components for display/debugging
  int hours = ntpClient->getHours();
  int minutes = ntpClient->getMinutes();
  int seconds = ntpClient->getSeconds();
  
  // Get epoch time (seconds since Jan 1, 1970)
  unsigned long epochTime = ntpClient->getEpochTime();
  
  // For debugging - print human-readable time
  Serial.print("NTP Time: ");
  Serial.print(formatTimeComponent(hours));
  Serial.print(":");
  Serial.print(formatTimeComponent(minutes));
  Serial.print(":");
  Serial.println(formatTimeComponent(seconds));
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  // Convert to Firebase-compatible timestamp format with local timezone
  String localTimestamp = formatLocalTimestamp(epochTime);
  Serial.print("Local Timezone Timestamp: ");
  Serial.println(localTimestamp);
  
  return localTimestamp;
}