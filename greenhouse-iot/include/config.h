#ifndef CONFIG_H
#define CONFIG_H

// Device identification
#define DEVICE_ID "arduino-1"
#define DEVICE_LOCATION "windowsill"

// Timing
#define READ_INTERVAL_MS 60000

// DHT22 Configuration
#define DHT_ENABLED true
#define DHT_PIN 2
#define DHT_TYPE DHT22

// Soil Moisture Sensors (up to 6)
#define SOIL_SENSOR_COUNT 1
static const int SOIL_PINS[] = {A0};
static const char* SOIL_NAMES[] = {"soil_moisture"};
static const int SOIL_AIR_VALUES[] = {478};
static const int SOIL_WATER_VALUES[] = {206};

#endif // CONFIG_H
