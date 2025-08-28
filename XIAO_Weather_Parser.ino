/*
 * XIAO Weather Station Data Parser
 * 
 * This sketch parses weather station data from various formats:
 * - JSON (weather APIs like OpenWeatherMap, WeatherAPI, etc.)
 * - CSV (comma-separated values with headers)
 * - XML (structured weather data markup)
 * 
 * Hardware: XIAO ESP32C3
 * Libraries: WiFi, HTTPClient, ArduinoJson
 * 
 * Serial Commands:
 * - help - Display available commands
 * - status - Show WiFi and system status  
 * - parse json <data> - Parse JSON weather data
 * - parse csv <data> - Parse CSV weather data
 * - parse xml <data> - Parse XML weather data
 * - fetch <url> - Fetch weather data from URL
 * - test - Run test parsing with sample data
 * - wifi connect - Connect to configured WiFi
 * - wifi disconnect - Disconnect from WiFi
 * - debug on/off - Toggle debug output
 */

#include "driver.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =============================================================================
// CONFIGURATION - UPDATE THESE VALUES
// =============================================================================

// TODO: Replace with your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

WiFiState currentWiFiState = WIFI_DISCONNECTED;
bool debugMode = false;
String commandBuffer = "";
WeatherData lastParsedData;

// =============================================================================
// SETUP FUNCTION
// =============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000); // Allow serial monitor to connect
  
  // Initialize built-in LED
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  
  // Display startup message
  Serial.println("========================================");
  Serial.println("  XIAO Weather Station Data Parser");
  Serial.println("========================================");
  Serial.printf("Hardware: ESP32-C3 @ %dMHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Flash size: %d bytes\n", ESP.getFlashChipSize());
  Serial.println("Type 'help' for available commands");
  Serial.println("========================================");
  
  // Initialize weather data structure
  initializeWeatherData(&lastParsedData);
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  Serial.println("Ready to parse weather data!");
  Serial.print("> ");
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  // Process serial commands
  if (Serial.available()) {
    processSerialInput();
  }
  
  // Monitor WiFi connection status
  monitorWiFiStatus();
  
  // Memory monitoring (warn if heap is low)
  static unsigned long lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 30000) { // Check every 30 seconds
    if (ESP.getFreeHeap() < HEAP_WARNING_THRESHOLD) {
      Serial.printf("[WARNING] Low memory: %d bytes free\n", ESP.getFreeHeap());
    }
    lastHeapCheck = millis();
  }
  
  delay(10); // Small delay to prevent watchdog issues
}

// =============================================================================
// SERIAL COMMAND PROCESSING
// =============================================================================

void processSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
        Serial.print("> ");
      }
    } else if (c >= 32 && c <= 126) { // Printable characters only
      commandBuffer += c;
      if (commandBuffer.length() >= COMMAND_BUFFER_SIZE - 1) {
        Serial.println("\n[ERROR] Command too long!");
        commandBuffer = "";
        Serial.print("> ");
      }
    }
  }
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  DEBUG_PRINTF("[DEBUG] Processing command: %s\n", command.c_str());
  
  if (command == "help") {
    showHelp();
  } else if (command == "status") {
    showStatus();
  } else if (command.startsWith("parse json ")) {
    String jsonData = command.substring(11);
    parseWeatherJSON(jsonData);
  } else if (command.startsWith("parse csv ")) {
    String csvData = command.substring(10);
    parseWeatherCSV(csvData);
  } else if (command.startsWith("parse xml ")) {
    String xmlData = command.substring(10);
    parseWeatherXML(xmlData);
  } else if (command.startsWith("fetch ")) {
    String url = command.substring(6);
    fetchWeatherData(url);
  } else if (command == "test") {
    runTestParsing();
  } else if (command == "wifi connect") {
    connectWiFi();
  } else if (command == "wifi disconnect") {
    disconnectWiFi();
  } else if (command == "debug on") {
    debugMode = true;
    Serial.println("Debug mode enabled");
  } else if (command == "debug off") {
    debugMode = false;
    Serial.println("Debug mode disabled");
  } else if (command == "clear") {
    // Clear screen (send ANSI escape sequence)
    Serial.write(27); // ESC
    Serial.print("[2J"); // Clear screen
    Serial.write(27); // ESC  
    Serial.print("[H"); // Home cursor
  } else {
    Serial.println("[ERROR] Unknown command. Type 'help' for available commands.");
  }
}

// =============================================================================
// HELP AND STATUS FUNCTIONS
// =============================================================================

void showHelp() {
  Serial.println("\nAvailable Commands:");
  Serial.println("==================");
  Serial.println("help                     - Show this help message");
  Serial.println("status                   - Show system and WiFi status");
  Serial.println("parse json <data>        - Parse JSON weather data");
  Serial.println("parse csv <data>         - Parse CSV weather data");
  Serial.println("parse xml <data>         - Parse XML weather data");
  Serial.println("fetch <url>              - Fetch weather data from URL");
  Serial.println("test                     - Run test parsing with sample data");
  Serial.println("wifi connect             - Connect to configured WiFi");
  Serial.println("wifi disconnect          - Disconnect from WiFi");
  Serial.println("debug on/off             - Toggle debug output");
  Serial.println("clear                    - Clear terminal screen");
  Serial.println();
}

void showStatus() {
  Serial.println("\nSystem Status:");
  Serial.println("==============");
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("Debug Mode: %s\n", debugMode ? "ON" : "OFF");
  
  Serial.println("\nWiFi Status:");
  Serial.println("============");
  Serial.printf("State: %s\n", getWiFiStateString());
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  }
  
  Serial.println("\nLast Parsed Data:");
  Serial.println("================");
  if (lastParsedData.isValid) {
    displayWeatherData(&lastParsedData);
  } else {
    Serial.println("No valid data parsed yet");
  }
  Serial.println();
}

// =============================================================================
// WEATHER PARSING FUNCTIONS - TODO: IMPLEMENT THESE
// =============================================================================

void parseWeatherJSON(String jsonData) {
  Serial.println("[INFO] JSON parsing not yet implemented");
  Serial.printf("JSON data received: %s\n", jsonData.c_str());
  
  // TODO: Implement JSON parsing using ArduinoJson library
  // 1. Create DynamicJsonDocument with appropriate size
  // 2. Parse the JSON string
  // 3. Extract weather fields (temperature, humidity, etc.)
  // 4. Populate WeatherData structure
  // 5. Validate and display results
}

void parseWeatherCSV(String csvData) {
  Serial.println("[INFO] CSV parsing not yet implemented");
  Serial.printf("CSV data received: %s\n", csvData.c_str());
  
  // TODO: Implement CSV parsing
  // 1. Split data into lines
  // 2. Parse header row to identify columns
  // 3. Parse data row(s) and map to weather fields
  // 4. Handle different CSV formats and delimiters
  // 5. Populate WeatherData structure
}

void parseWeatherXML(String xmlData) {
  Serial.println("[INFO] XML parsing not yet implemented");
  Serial.printf("XML data received: %s\n", xmlData.c_str());
  
  // TODO: Implement XML parsing
  // 1. Create simple XML parser or use existing library
  // 2. Parse XML structure and extract weather elements
  // 3. Handle nested elements and attributes
  // 4. Map XML data to WeatherData structure
  // 5. Validate and display results
}

// =============================================================================
// NETWORK FUNCTIONS - TODO: IMPLEMENT THESE
// =============================================================================

void fetchWeatherData(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.printf("[INFO] Fetching data from: %s\n", url.c_str());
  
  // TODO: Implement HTTP/HTTPS client
  // 1. Create HTTPClient instance
  // 2. Set timeout and user agent
  // 3. Make GET request to URL
  // 4. Handle response codes and redirects
  // 5. Parse response data based on content type
  // 6. Display results
  
  Serial.println("[INFO] HTTP client not yet implemented");
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[INFO] Already connected to WiFi");
    return;
  }
  
  Serial.printf("[INFO] Connecting to WiFi: %s\n", ssid);
  currentWiFiState = WIFI_CONNECTING;
  
  WiFi.begin(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    currentWiFiState = WIFI_CONNECTED;
    Serial.println("[SUCCESS] WiFi connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(LED_BUILTIN_PIN, HIGH); // LED on when connected
  } else {
    currentWiFiState = WIFI_CONNECTION_FAILED;
    Serial.println("[ERROR] WiFi connection failed");
  }
}

void disconnectWiFi() {
  WiFi.disconnect();
  currentWiFiState = WIFI_DISCONNECTED;
  digitalWrite(LED_BUILTIN_PIN, LOW); // LED off when disconnected
  Serial.println("[INFO] WiFi disconnected");
}

// =============================================================================
// TEST FUNCTIONS
// =============================================================================

void runTestParsing() {
  Serial.println("\nRunning Test Parsing:");
  Serial.println("====================");
  
  // Test JSON parsing
  Serial.println("Testing JSON parsing...");
  String testJson = "{\"temperature\":22.5,\"humidity\":65,\"conditions\":\"sunny\"}";
  parseWeatherJSON(testJson);
  
  // Test CSV parsing  
  Serial.println("\nTesting CSV parsing...");
  String testCsv = "temperature,humidity,conditions\\n22.5,65,sunny";
  parseWeatherCSV(testCsv);
  
  // Test XML parsing
  Serial.println("\nTesting XML parsing...");
  String testXml = "<weather><temperature>22.5</temperature><humidity>65</humidity></weather>";
  parseWeatherXML(testXml);
  
  Serial.println("\nTest parsing complete!");
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void initializeWeatherData(WeatherData* data) {
  data->temperature = 0.0;
  data->humidity = 0.0;
  data->pressure = 0.0;
  data->windSpeed = 0.0;
  data->windDirection = 0;
  data->visibility = 0.0;
  data->uvIndex = 0.0;
  data->precipitation = 0.0;
  data->conditions = "";
  data->timestamp = "";
  data->location = "";
  data->isValid = false;
  data->parseTime = 0;
}

void displayWeatherData(WeatherData* data) {
  Serial.printf("Temperature: %.1f°C\n", data->temperature);
  Serial.printf("Humidity: %.1f%%\n", data->humidity);
  Serial.printf("Pressure: %.1f hPa\n", data->pressure);
  Serial.printf("Wind: %.1f m/s at %d°\n", data->windSpeed, data->windDirection);
  Serial.printf("Visibility: %.1f km\n", data->visibility);
  Serial.printf("UV Index: %.1f\n", data->uvIndex);
  Serial.printf("Precipitation: %.1f mm\n", data->precipitation);
  Serial.printf("Conditions: %s\n", data->conditions.c_str());
  Serial.printf("Location: %s\n", data->location.c_str());
  Serial.printf("Timestamp: %s\n", data->timestamp.c_str());
  Serial.printf("Parse Time: %lu ms\n", data->parseTime);
}

const char* getWiFiStateString() {
  switch (currentWiFiState) {
    case WIFI_DISCONNECTED: return "Disconnected";
    case WIFI_CONNECTING: return "Connecting";  
    case WIFI_CONNECTED: return "Connected";
    case WIFI_CONNECTION_FAILED: return "Connection Failed";
    default: return "Unknown";
  }
}

void monitorWiFiStatus() {
  static WiFiState lastState = WIFI_DISCONNECTED;
  WiFiState newState = (WiFiState)WiFi.status();
  
  // Update LED based on WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    if (currentWiFiState != WIFI_CONNECTED) {
      currentWiFiState = WIFI_CONNECTED;
    }
  } else {
    digitalWrite(LED_BUILTIN_PIN, LOW);
    if (currentWiFiState == WIFI_CONNECTED) {
      currentWiFiState = WIFI_DISCONNECTED;
      DEBUG_PRINTLN("[DEBUG] WiFi connection lost");
    }
  }
  
  // Debug state changes
  if (debugMode && newState != lastState) {
    DEBUG_PRINTF("[DEBUG] WiFi state changed: %s\n", getWiFiStateString());
    lastState = newState;
  }
}
