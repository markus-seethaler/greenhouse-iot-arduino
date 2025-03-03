#include <Arduino.h>
#include <WiFiS3.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Kindle";
const char* password = "Helbing4Lyfe";

// Google Cloud Function endpoint
const char* serverAddress = "us-central1-iot-greenhouse-16852.cloudfunctions.net";
const String path = "/recordSensorData";
const char* deviceId = "garden_monitor_1";

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
const long uploadInterval = 60000;
unsigned long lastUploadTime = 0;

// Store the latest sensor readings
float latestHumidity = 0;
float latestTemperature = 0;
int latestSoilMoisture = 0;

// Initialize the WiFi client
WiFiSSLClient client;

// Function prototypes
void connectToWiFi();
void readSensors();
void uploadData();

void setup() {
  Serial.begin(9600);
  
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("DHT22 and Soil Moisture Sensor with WiFi");
  Serial.println("-----------------------------------------");
  
  dht.begin();
  pinMode(SOIL_PIN, INPUT);
  connectToWiFi();
  
  Serial.println("Sensors initialized. Starting readings...");
  Serial.println();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= sensorInterval) {
    previousMillis = currentMillis;
    readSensors();
  }
  
  if (currentMillis - lastUploadTime >= uploadInterval) {
    uploadData();
    lastUploadTime = currentMillis;
  }
}

void connectToWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Wait up to 10 seconds for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed! Will retry later.");
  }
}

void readSensors() {
  // Read DHT22 sensor with retry mechanism
  for (int i = 0; i < 3; i++) {  // Try up to 3 times
    latestHumidity = dht.readHumidity();
    latestTemperature = dht.readTemperature();
    
    if (!isnan(latestHumidity) && !isnan(latestTemperature)) {
      break;  // Reading successful, exit retry loop
    }
    
    delay(1000);  // Wait before retrying
  }

  // Read soil moisture sensor
  soilMoistureValue = analogRead(SOIL_PIN);
  latestSoilMoisture = map(soilMoistureValue, airValue, waterValue, 0, 100);
  latestSoilMoisture = constrain(latestSoilMoisture, 0, 100);
  
  // Check if DHT22 readings failed after all retries
  if (isnan(latestHumidity) || isnan(latestTemperature)) {
    Serial.println("Failed to read from DHT sensor after multiple attempts!");
  } else {
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

void uploadData() {
  // Check if we have valid sensor readings
  if (isnan(latestHumidity) || isnan(latestTemperature)) {
    Serial.println("Invalid sensor readings, skipping upload");
    return;
  }
  
  Serial.println("Preparing to upload data to cloud...");
  
  // Create JSON document
  StaticJsonDocument<256> jsonDoc;
  
  // Populate the JSON document to match exactly what the Cloud Function expects
  jsonDoc["deviceId"] = deviceId;
  
  // Create sensors object
  JsonObject sensors = jsonDoc.createNestedObject("sensors");
  
  // Create air object with all required fields
  JsonObject air = sensors.createNestedObject("air");
  air["temperature"] = latestTemperature;
  air["humidity"] = latestHumidity;
  air["unit"] = "C";
  
  // Create soil object with all required fields
  JsonObject soil = sensors.createNestedObject("soil");
  soil["moisture"] = latestSoilMoisture;
  soil["unit"] = "%";
  
  // Serialize JSON to string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
  // Print JSON for debugging
  Serial.print("Sending: ");
  Serial.println(jsonString);
  
  // Make direct HTTPS request
  Serial.println("Making direct HTTPS request...");
  
  // Connect with timeout
  unsigned long connectStart = millis();
  Serial.print("Connecting to server...");
  
  bool connected = false;
  while (!connected && millis() - connectStart < 5000) {
    if (client.connect(serverAddress, 443)) {
      connected = true;
    }
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  
  if (connected) {
    Serial.println("Connected to server");
    
    // Send HTTP POST request with all required headers
    client.print("POST ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverAddress);
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Accept: application/json");
    client.println("User-Agent: ArduinoUnoR4/1.0");
    client.print("Content-Length: ");
    client.println(jsonString.length());
    client.println();
    client.println(jsonString);
    
    // Wait for server response with timeout
    Serial.println("Waiting for response...");
    unsigned long responseStart = millis();
    bool responseReceived = false;
    String statusLine = "";
    bool statusReceived = false;
    
    // Read response with timeout - only look for the status line
    while (client.connected() && millis() - responseStart < 10000) {
      if (client.available()) {
        responseReceived = true;
        char c = client.read();
        
        // Collect the first line only (status line)
        if (!statusReceived) {
          statusLine += c;
          
          // Check for end of status line
          if (statusLine.endsWith("\r\n")) {
            statusReceived = true;
            Serial.print("Response status: ");
            Serial.println(statusLine);
            
            // Check for successful status code
            if (statusLine.indexOf("HTTP/1.1 200") >= 0 || 
                statusLine.indexOf("HTTP/1.0 200") >= 0) {
              Serial.println("Data uploaded successfully (200 OK)");
            } else {
              Serial.println("Request failed with status: " + statusLine);
            }
            
            // Skip the rest of the response
            while (client.available()) {
              client.read();
            }
            break;
          }
        }
      }
    }
    
    if (!responseReceived) {
      Serial.println("No response received from server (timeout)");
    }
    
    client.stop();
    Serial.println("\nConnection closed");
  } else {
    Serial.println("Failed to connect to server");
  }
}