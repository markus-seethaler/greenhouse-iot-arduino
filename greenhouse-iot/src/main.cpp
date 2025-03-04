#include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include <FirebaseClient.h>
#include <NTPClient.h>

// Include secrets from separate file that won't be committed to git
#include "firebase_secret.h"

// WiFi credentials
const char *ssid = "Kindle";
const char *password = "Helbing4Lyfe";

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

// In your global variables section, add this helper function to format time components
String formatTimeComponent(int component)
{
  // Add leading zero if component is less than 10
  return component < 10 ? "0" + String(component) : String(component);
}

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
String getFormattedTimestamp();
bool isDST();

void setup()
{
  Serial.begin(9600);

  while (!Serial)
  {
    ; // Wait for serial port to connect
  }

  Serial.println("DHT22 and Soil Moisture Sensor with Firestore");
  Serial.println("-----------------------------------------");

  dht.begin();
  pinMode(SOIL_PIN, INPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
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

// Modify your loop function to regularly update NTP time
void loop()
{
  // To maintain the authentication and async tasks
  app.loop();
  unsigned long currentMillis = millis();

  // Update NTP client periodically (every minute)
  if (currentMillis - lastupdate >= 60000)
  {
    ntpClient.update();
    lastupdate = currentMillis;

    // Check if daylight saving time adjustment is needed
    // This is a simplified approach - you may want a more robust DST detection
    if (isDST())
    {
      ntpClient.setTimeOffset(utcOffsetSummer);
    }
    else
    {
      ntpClient.setTimeOffset(utcOffsetWinter);
    }
  }

  if (currentMillis - previousMillis >= sensorInterval)
  {
    previousMillis = currentMillis;
    readSensors();
  }

  if (app.ready() && (currentMillis - lastUploadTime >= uploadInterval))
  {
    uploadToFirestore();
    lastUploadTime = currentMillis;
  }
}

void readSensors()
{
  // Read DHT22 sensor with retry mechanism
  for (int i = 0; i < 3; i++)
  { // Try up to 3 times
    latestHumidity = dht.readHumidity();
    latestTemperature = dht.readTemperature();

    if (!isnan(latestHumidity) && !isnan(latestTemperature))
    {
      break; // Reading successful, exit retry loop
    }

    delay(1000); // Wait before retrying
  }

  // Read soil moisture sensor
  soilMoistureValue = analogRead(SOIL_PIN);
  latestSoilMoisture = map(soilMoistureValue, airValue, waterValue, 0, 100);
  latestSoilMoisture = constrain(latestSoilMoisture, 0, 100);

  // Check if DHT22 readings failed after all retries
  if (isnan(latestHumidity) || isnan(latestTemperature))
  {
    Serial.println("Failed to read from DHT sensor after multiple attempts!");
  }
  else
  {
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

void uploadToFirestore()
{
  // Check if we have valid sensor readings
  if (isnan(latestHumidity) || isnan(latestTemperature))
  {
    Serial.println("Invalid sensor readings, skipping upload");
    return;
  }

  Serial.println("Preparing to upload data to Firestore...");

  // Define the document path
  String documentPath = "devices/growbox/readings";

  // Create document
  Document<Values::Value> doc;

  // Add timestamp
  String timestamp = getFormattedTimestamp();
  doc.add("timestamp", Values::Value(Values::TimestampValue(timestamp)));

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

void processData(AsyncResult &aResult)
{
  // Handle Firebase async results

  // Exit if no result available
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
  {
    Serial.print("Event task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.print(aResult.eventLog().message().c_str());
    Serial.print(", code: ");
    Serial.println(aResult.eventLog().code());
  }

  if (aResult.isDebug())
  {
    Serial.print("Debug task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.println(aResult.debug().c_str());
  }

  if (aResult.isError())
  {
    Serial.print("Error task: ");
    Serial.print(aResult.uid().c_str());
    Serial.print(", msg: ");
    Serial.print(aResult.error().message().c_str());
    Serial.print(", code: ");
    Serial.println(aResult.error().code());
  }

  if (aResult.available())
  {
    Serial.print("Task: ");
    Serial.print(aResult.uid().c_str());
  }
}

// Function to convert epoch time to RFC 3339 format with timezone offset
String formatLocalTimestamp(unsigned long epochTime) {
  // Convert epoch time to time_t
  time_t rawtime = (time_t)epochTime;
  struct tm *timeinfo;
  
  // Get time info in UTC
  timeinfo = gmtime(&rawtime);
  
  // Apply the timezone offset manually
  // For UTC+1, add 3600 seconds (1 hour)
  // For UTC+2 (during DST), add 7200 seconds (2 hours)
  int tzOffset = isDST() ? 7200 : 3600; // Seconds to add based on DST
  rawtime += tzOffset;
  timeinfo = gmtime(&rawtime);
  
  // Format as YYYY-MM-DDTHH:MM:SS+01:00 or +02:00 (RFC 3339 format with timezone)
  char buffer[40];
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

// Updated getFormattedTimestamp function
String getFormattedTimestamp() {
  // Update NTP client to ensure we have the latest time
  ntpClient.update();
  
  // Get current time components for display/debugging
  int hours = ntpClient.getHours();
  int minutes = ntpClient.getMinutes();
  int seconds = ntpClient.getSeconds();
  
  // Get epoch time (seconds since Jan 1, 1970)
  unsigned long epochTime = ntpClient.getEpochTime();
  
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

// Simple function to determine if Daylight Saving Time is in effect
// You may want to replace this with a more accurate implementation for your location
bool isDST()
{
  // Get current date information from NTP
  unsigned long epochTime = ntpClient.getEpochTime();
  time_t rawtime = (time_t)epochTime;
  struct tm *ti;
  ti = gmtime(&rawtime);

  int month = ti->tm_mon + 1; // tm_mon is 0-11
  int day = ti->tm_mday;

  // Simple European DST rule (last Sunday of March to last Sunday of October)
  // This is a simplified approach and might not be accurate for all years
  if (month > 3 && month < 10)
  {
    return true; // April to September
  }
  else if (month == 3)
  {
    // March: DST starts on the last Sunday
    // This is a simplified approach - proper implementation would check the exact date
    return day >= 25; // Approximation for last week of March
  }
  else if (month == 10)
  {
    // October: DST ends on the last Sunday
    return day < 25; // Approximation for before the last week of October
  }
  return false;
}