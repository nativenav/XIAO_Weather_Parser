#pragma once

// Hardware Configuration
#define TARGET_BOARD_XIAO_ESP32C3

// Weather Data Parsing Configuration
#define WEATHER_BUFFER_SIZE 8192      // 8KB max data size per parsing operation
#define JSON_BUFFER_SIZE 4096         // JSON parsing buffer (ArduinoJson)
#define CSV_MAX_COLUMNS 20            // Maximum CSV columns to parse
#define XML_MAX_DEPTH 10              // Maximum XML nesting depth
#define COMMAND_BUFFER_SIZE 512       // Serial command input buffer
#define MAX_WEATHER_FIELDS 15         // Maximum parsed weather fields

// Network Configuration  
#define HTTP_TIMEOUT 10000            // 10 second HTTP request timeout
#define WIFI_CONNECT_TIMEOUT 40000    // 40 second WiFi connection timeout
#define WIFI_SCAN_TIMEOUT 10000       // 10 second WiFi scan timeout
#define MAX_REDIRECT_FOLLOW 3         // Maximum HTTP redirects to follow
#define USER_AGENT "XIAO-Weather-Parser/1.0"

// WiFi Management Configuration
#define MAX_WIFI_NETWORKS 20          // Maximum networks to display in scan
#define WIFI_SSID_MAX_LEN 32          // Maximum SSID length
#define WIFI_PASS_MAX_LEN 64          // Maximum password length
#define WIFI_RECONNECT_INTERVAL 30000 // 30 second reconnect attempt interval
#define WIFI_STATUS_CHECK_INTERVAL 5000 // 5 second status check interval

// Preferences (NVRAM) Storage Keys
#define PREFS_NAMESPACE "weather"     // Preferences namespace
#define PREFS_WIFI_SSID "wifi_ssid"   // Stored WiFi SSID key
#define PREFS_WIFI_PASS "wifi_pass"   // Stored WiFi password key
#define PREFS_WIFI_CONFIGURED "wifi_cfg" // WiFi configuration flag

// Serial Communication
#define SERIAL_BAUD_RATE 115200       // Standard baud rate
#define SERIAL_READ_TIMEOUT 1000      // 1 second timeout for serial input

// Memory Management
#define HEAP_WARNING_THRESHOLD 10000  // Warn if free heap drops below 10KB
#define STACK_SIZE_BYTES 8192         // Task stack size

// Pin Definitions (for future expansion)
#define LED_BUILTIN_PIN 2             // Built-in LED on XIAO ESP32C3
#define I2C_SDA_PIN 4                 // I2C data pin
#define I2C_SCL_PIN 5                 // I2C clock pin
#define SPI_MOSI_PIN 10               // SPI data out
#define SPI_MISO_PIN 9                // SPI data in  
#define SPI_SCK_PIN 8                 // SPI clock
#define SPI_SS_PIN 7                  // SPI chip select

// Debug Configuration
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) Serial.printf(format, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
#endif

// Weather Data Structure
struct WeatherData {
  float temperature;      // Temperature in Celsius
  float humidity;         // Relative humidity percentage
  float pressure;         // Atmospheric pressure in hPa
  float windSpeed;        // Wind speed in m/s
  int windDirection;      // Wind direction in degrees (0-359)
  float visibility;       // Visibility in kilometers
  float uvIndex;          // UV index
  float precipitation;    // Precipitation in mm
  String conditions;      // Weather conditions description
  String timestamp;       // ISO 8601 timestamp
  String location;        // Location name or coordinates
  bool isValid;           // Data validity flag
  unsigned long parseTime; // Time taken to parse in milliseconds
};

// Parser result codes
enum ParseResult {
  PARSE_SUCCESS = 0,
  PARSE_ERROR_INVALID_FORMAT = 1,
  PARSE_ERROR_BUFFER_OVERFLOW = 2,
  PARSE_ERROR_MEMORY_FULL = 3,
  PARSE_ERROR_NETWORK_TIMEOUT = 4,
  PARSE_ERROR_UNKNOWN_FORMAT = 5
};

// WiFi connection states
enum WiFiState {
  WIFI_DISCONNECTED = 0,
  WIFI_CONNECTING = 1,
  WIFI_CONNECTED = 2,
  WIFI_CONNECTION_FAILED = 3,
  WIFI_SCANNING = 4,
  WIFI_SCAN_COMPLETE = 5
};

// WiFi security types
enum WiFiSecurityType {
  WIFI_SECURITY_OPEN = 0,
  WIFI_SECURITY_WEP = 1,
  WIFI_SECURITY_WPA = 2,
  WIFI_SECURITY_WPA2 = 3,
  WIFI_SECURITY_WPA3 = 4,
  WIFI_SECURITY_UNKNOWN = 5
};

// WiFi network information structure
struct WiFiNetworkInfo {
  String ssid;                    // Network name
  int rssi;                       // Signal strength in dBm
  WiFiSecurityType security;      // Security type
  int channel;                    // WiFi channel
  bool isHidden;                  // Hidden network flag
};

// WiFi credentials structure for storage
struct WiFiCredentials {
  char ssid[WIFI_SSID_MAX_LEN + 1];     // Stored SSID
  char password[WIFI_PASS_MAX_LEN + 1]; // Stored password
  bool isConfigured;                     // Configuration status
};
