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
#include <Preferences.h>

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

WiFiState currentWiFiState = WIFI_DISCONNECTED;
bool debugMode = false;
String commandBuffer = "";
WeatherData lastParsedData;

// WiFi Management
Preferences preferences;
WiFiCredentials storedCredentials;
WiFiNetworkInfo scannedNetworks[MAX_WIFI_NETWORKS];
int numScannedNetworks = 0;
unsigned long lastWiFiReconnectAttempt = 0;
bool awaitingPasswordInput = false;
int selectedNetworkIndex = -1;

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
  
  // Initialize WiFi management
  initializeWiFiManagement();
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  
  // Try to auto-connect to stored WiFi credentials
  autoConnectWiFi();
  
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
  
  // Handle password input specially (don't convert to lowercase)
  if (awaitingPasswordInput) {
    DEBUG_PRINTF("[DEBUG] Processing password input\n");
    connectToSelectedNetwork(command);
    return;
  }
  
  // Convert to lowercase for command matching
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
  } else if (command == "brambles") {
    fetchBramblesWeather();
  } else if (command == "seaview") {
    fetchSeaviewWeather();
  } else if (command == "lymington") {
    fetchLymingtonWeather();
  } else if (command == "test") {
    runTestParsing();
  } else if (command == "wifi scan") {
    scanWiFiNetworks();
  } else if (command.startsWith("wifi select ")) {
    int networkIndex = command.substring(12).toInt();
    selectWiFiNetwork(networkIndex);
  } else if (command == "wifi connect") {
    connectToStoredNetwork();
  } else if (command == "wifi disconnect") {
    disconnectWiFi();
  } else if (command == "wifi forget") {
    clearStoredCredentials();
  } else if (command == "wifi info") {
    showWiFiInfo();
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
  Serial.println("\nWeather Parsing:");
  Serial.println("parse json <data>        - Parse JSON weather data");
  Serial.println("parse csv <data>         - Parse CSV weather data");
  Serial.println("parse xml <data>         - Parse XML weather data");
  Serial.println("fetch <url>              - Fetch weather data from URL");
  Serial.println("brambles                 - Fetch Brambles Bank weather station data");
  Serial.println("seaview                  - Fetch Seaview, Isle of Wight weather station data");
  Serial.println("lymington                - Fetch Lymington Starting Platform weather station data");
  Serial.println("test                     - Run test parsing with sample data");
  Serial.println("\nWiFi Management:");
  Serial.println("wifi scan                - Scan for available networks");
  Serial.println("wifi select <index>      - Select network by index (from scan)");
  Serial.println("wifi connect             - Connect to stored network");
  Serial.println("wifi disconnect          - Disconnect from WiFi");
  Serial.println("wifi forget              - Clear stored credentials");
  Serial.println("wifi info                - Show WiFi configuration info");
  Serial.println("\nSystem:");
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
// NETWORK FUNCTIONS
// =============================================================================

void fetchWeatherData(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.printf("[INFO] Fetching data from: %s\n", url.c_str());
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  // Begin HTTP request
  http.begin(url);
  
  // Set standard headers
  http.addHeader("User-Agent", USER_AGENT);
  http.addHeader("Accept", "text/html,*/*");
  
  unsigned long startTime = millis();
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    Serial.printf("[INFO] HTTP %d - Received %d bytes in %lu ms\n", 
                  httpCode, payload.length(), fetchTime);
    
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Response data:");
      Serial.println(payload);
    } else {
      Serial.printf("[WARNING] HTTP error code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[ERROR] HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

// =============================================================================
// WEATHER STATION SPECIFIC FUNCTIONS
// =============================================================================

void fetchBramblesWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.println("[INFO] Fetching Brambles weather data...");
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  String url = "https://www.southamptonvts.co.uk/BackgroundSite/Ajax/LoadXmlFileWithTransform?xmlFilePath=D%3A%5Cftp%5Csouthampton%5CBramble.xml&xslFilePath=D%3A%5Cwwwroot%5CCMS_Southampton%5Ccontent%5Cfiles%5Cassets%5CSotonSnapshotmetBramble.xsl&w=51";
  
  // Begin HTTPS request 
  http.begin(url);
  
  // Set specific headers for Southampton VTS
  http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
  http.addHeader("Accept", "text/html,*/*");
  http.addHeader("Referer", "https://www.southamptonvts.co.uk/Live_Information/Tides_and_Weather/");
  
  unsigned long startTime = millis();
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    DEBUG_PRINTF("[DEBUG] Received %d bytes in %lu ms\n", payload.length(), fetchTime);
    DEBUG_PRINTF("[DEBUG] Raw response: %s\n", payload.c_str());
    
    // Parse the Brambles weather data
    parseBramblesData(payload, fetchTime);
    
  } else {
    Serial.printf("[ERROR] Failed to fetch Brambles data. HTTP %d: %s\n", 
                  httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void parseBramblesData(String htmlData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Brambles weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Brambles Bank";
  
  unsigned long parseStart = millis();
  
  // Parse each field from the HTML text
  // Format: "Bramble Bank Wind Speed 19.8 Knots Max Gust 20.6 Knots Wind Direction 335 Degree Tide Height .0 m Air Temp 17.4 C Pressure 997.4 mBar Visibility .0 nM Updated 29/08/2025 14:13:00"
  
  // Extract wind speed in knots
  float windSpeedKnots = extractFloatAfterPhrase(htmlData, "Wind Speed ", " Knots");
  if (windSpeedKnots > 0) {
    data.windSpeed = windSpeedKnots * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Speed: %.1f knots = %.1f m/s\n", windSpeedKnots, data.windSpeed);
  } else {
    DEBUG_PRINTLN("[DEBUG] Wind Speed: n/a");
  }
  
  // Extract wind gust in knots
  float windGustKnots = extractFloatAfterPhrase(htmlData, "Max Gust ", " Knots");
  if (windGustKnots > 0) {
    data.windGust = windGustKnots * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Gust: %.1f knots = %.1f m/s\n", windGustKnots, data.windGust);
  } else {
    DEBUG_PRINTLN("[DEBUG] Wind Gust: n/a");
  }
  
  // Extract wind direction in degrees
  data.windDirection = (int)extractFloatAfterPhrase(htmlData, "Wind Direction ", " Degree");
  DEBUG_PRINTF("[DEBUG] Wind Direction: %d degrees\n", data.windDirection);
  
  // Extract air temperature in Celsius
  data.temperature = extractFloatAfterPhrase(htmlData, "Air Temp ", " C");
  DEBUG_PRINTF("[DEBUG] Temperature: %.1f°C\n", data.temperature);
  
  // Extract pressure in mBar and convert to hPa (same value, different name)
  data.pressure = extractFloatAfterPhrase(htmlData, "Pressure ", " mBar");
  DEBUG_PRINTF("[DEBUG] Pressure: %.1f mBar (%.1f hPa)\n", data.pressure, data.pressure);
  
  // Extract timestamp
  String timestamp = extractStringAfterPhrase(htmlData, "Updated ", "");
  if (timestamp.length() > 0) {
    data.timestamp = timestamp;
    DEBUG_PRINTF("[DEBUG] Timestamp: %s\n", timestamp.c_str());
  } else {
    DEBUG_PRINTLN("[DEBUG] Timestamp: n/a");
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = true;
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== BRAMBLES BANK WEATHER ===");
  Serial.printf("Wind Speed: %.1f knots (%.1f m/s)\n", windSpeedKnots, data.windSpeed);
  Serial.printf("Wind Gust: %.1f knots (%.1f m/s)\n", windGustKnots, data.windGust);
  Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
  Serial.printf("Air Temperature: %.1f°C\n", data.temperature);
  Serial.printf("Air Pressure: %.1f mBar\n", data.pressure);
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("=============================");
}

void fetchSeaviewWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.println("[INFO] Fetching Seaview weather data...");
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  String url = "https://www.weatherlink.com/embeddablePage/summaryData/0d117f9a7c7e425a8cc88e870f0e76fb";
  
  // Begin HTTPS request 
  http.begin(url);
  
  // Set specific headers for WeatherLink
  http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
  http.addHeader("Accept", "application/json,*/*");
  http.addHeader("X-Requested-With", "XMLHttpRequest");
  http.addHeader("Referer", "https://www.weatherlink.com/embeddablePage/show/0d117f9a7c7e425a8cc88e870f0e76fb/summary");
  
  unsigned long startTime = millis();
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    DEBUG_PRINTF("[DEBUG] Received %d bytes in %lu ms\n", payload.length(), fetchTime);
    DEBUG_PRINTF("[DEBUG] Raw response: %s\n", payload.c_str());
    
    // Parse the Seaview weather data
    parseSeaviewData(payload, fetchTime);
    
  } else {
    Serial.printf("[ERROR] Failed to fetch Seaview data. HTTP %d: %s\n", 
                  httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void parseSeaviewData(String jsonData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Seaview weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Seaview, Isle of Wight";
  
  unsigned long parseStart = millis();
  
  // Parse JSON using ArduinoJson library
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.printf("[ERROR] JSON parsing failed: %s\n", error.c_str());
    return;
  }
  
  // Handle both array and object root structures
  JsonObject rootObj;
  if (doc.is<JsonArray>()) {
    if (doc.size() > 0) {
      rootObj = doc[0];
    } else {
      Serial.println("[ERROR] Empty JSON array");
      return;
    }
  } else {
    rootObj = doc.as<JsonObject>();
  }
  
  // Extract current conditions array
  JsonArray conditions = rootObj["currConditionValues"];
  if (conditions.isNull()) {
    Serial.println("[ERROR] currConditionValues not found in JSON");
    return;
  }
  
  float windSpeed = 0.0;
  float windGust = 0.0;
  int windDirection = 0;
  float temperature = 0.0;
  
  // Parse current conditions
  for (JsonObject condition : conditions) {
    String dataName = condition["sensorDataName"] | "";
    String value = condition["convertedValue"] | "";
    String unit = condition["unitLabel"] | "";
    
    // Skip invalid values
    if (value == "--" || value == "" || value == "null") continue;
    
    DEBUG_PRINTF("[DEBUG] Found: %s = %s %s\n", dataName.c_str(), value.c_str(), unit.c_str());
    
    if (dataName == "Wind Speed") {
      windSpeed = value.toFloat();
      if (unit == "knots") {
        data.windSpeed = windSpeed * 0.514444; // Convert to m/s
      } else {
        data.windSpeed = windSpeed; // Assume m/s
      }
    } else if (dataName == "Wind Direction") {
      data.windDirection = value.toInt();
    } else if (dataName.indexOf("High Wind Speed") >= 0 || dataName.indexOf("10 Min High") >= 0) {
      float gustValue = value.toFloat();
      if (gustValue > windGust) {
        windGust = gustValue;
        if (unit == "knots") {
          data.windGust = windGust * 0.514444; // Convert to m/s
        } else {
          data.windGust = windGust; // Assume m/s
        }
      }
    } else if (dataName.indexOf("Temperature") >= 0 || dataName.indexOf("Temp") >= 0) {
      data.temperature = value.toFloat();
    }
  }
  
  // Check for high/low values array for additional gust data
  JsonArray extremes = rootObj["highLowValues"];
  if (!extremes.isNull()) {
    for (JsonObject extreme : extremes) {
      String dataName = extreme["sensorDataName"] | "";
      String value = extreme["convertedValue"] | "";
      String unit = extreme["unitLabel"] | "";
      
      if (value == "--" || value == "" || value == "null") continue;
      
      DEBUG_PRINTF("[DEBUG] Found extreme: %s = %s %s\n", dataName.c_str(), value.c_str(), unit.c_str());
      
      if (dataName.indexOf("High Wind Speed") >= 0) {
        float dailyGust = value.toFloat();
        if (dailyGust > windGust) {
          windGust = dailyGust;
          if (unit == "knots") {
            data.windGust = windGust * 0.514444; // Convert to m/s
          } else {
            data.windGust = windGust; // Assume m/s
          }
        }
      }
    }
  }
  
  // Set timestamp from lastUpdate if available
  String lastUpdate = rootObj["lastUpdate"] | "";
  if (lastUpdate.length() > 0) {
    data.timestamp = lastUpdate;
  } else {
    data.timestamp = "Live data";
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = (data.windSpeed >= 0 || data.windDirection >= 0);
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== SEAVIEW WEATHER STATION ===");
  Serial.printf("Wind Speed: %.1f knots (%.1f m/s)\n", windSpeed, data.windSpeed);
  Serial.printf("Wind Gust: %.1f knots (%.1f m/s)\n", windGust, data.windGust);
  Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
  if (data.temperature > 0.0) {
    Serial.printf("Temperature: %.1f°C\n", data.temperature);
  } else {
    Serial.println("Temperature: n/a");
  }
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("===============================");
}

void fetchLymingtonWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.println("[INFO] Fetching Lymington weather data...");
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  String url = "https://weatherfile.com/V03/loc/GBR00001/latest.json";
  
  // Begin HTTPS request 
  http.begin(url);
  
  // Set specific headers for WeatherFile.com
  http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
  http.addHeader("Accept", "application/json,*/*");
  http.addHeader("X-Requested-With", "XMLHttpRequest");
  http.addHeader("Referer", "https://weatherfile.com/location?loc_id=GBR00001&wt=KTS");
  
  unsigned long startTime = millis();
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    DEBUG_PRINTF("[DEBUG] Received %d bytes in %lu ms\n", payload.length(), fetchTime);
    DEBUG_PRINTF("[DEBUG] Raw response: %s\n", payload.c_str());
    
    // Parse the Lymington weather data
    parseLymingtonData(payload, fetchTime);
    
  } else {
    Serial.printf("[ERROR] Failed to fetch Lymington data. HTTP %d: %s\n", 
                  httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void parseLymingtonData(String jsonData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Lymington weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Lymington Starting Platform";
  
  unsigned long parseStart = millis();
  
  // Parse JSON using ArduinoJson library
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.printf("[ERROR] JSON parsing failed: %s\n", error.c_str());
    return;
  }
  
  // Check if the response was successful
  String status = doc["status"] | "";
  if (status != "ok") {
    String message = doc["message"] | "Unknown error";
    Serial.printf("[ERROR] API returned error: %s\n", message.c_str());
    return;
  }
  
  // Extract data object
  JsonObject dataObj = doc["data"];
  if (dataObj.isNull()) {
    Serial.println("[ERROR] No data object found in response");
    return;
  }
  
  // Parse weather parameters
  // Based on your example: {"wdc":262,"wsc":9.36}
  // wdc = wind direction in degrees
  // wsc = wind speed (likely in m/s or knots - need to determine)
  
  if (dataObj.containsKey("wdc")) {
    data.windDirection = dataObj["wdc"] | 0;
    DEBUG_PRINTF("[DEBUG] Wind Direction: %d degrees\n", data.windDirection);
  }
  
  float windSpeedRaw = 0.0;
  if (dataObj.containsKey("wsc")) {
    windSpeedRaw = dataObj["wsc"] | 0.0;
    // WeatherFile uses knots based on URL parameter wt=KTS
    data.windSpeed = windSpeedRaw * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Speed: %.2f knots = %.2f m/s\n", windSpeedRaw, data.windSpeed);
  }
  
  // Extract timestamp
  String timestamp = dataObj["ts"] | "";
  if (timestamp.length() > 0) {
    data.timestamp = timestamp;
    DEBUG_PRINTF("[DEBUG] Timestamp: %s\n", timestamp.c_str());
  } else {
    data.timestamp = "Live data";
  }
  
  // Extract location name (more descriptive than hardcoded)
  String locName = dataObj["loc_name"] | "Lymington Starting Platform";
  data.location = locName;
  
  // Extract coordinates for reference
  float lat = dataObj["lat"] | 0.0;
  float lng = dataObj["lng"] | 0.0;
  DEBUG_PRINTF("[DEBUG] Location: %s (%.4f, %.4f)\n", locName.c_str(), lat, lng);
  
  // Extract data quality info
  int delay = dataObj["delay"] | 0;
  int numParams = dataObj["num_params"] | 0;
  DEBUG_PRINTF("[DEBUG] Data delay: %d min, Parameters: %d\n", delay, numParams);
  
  data.parseTime = millis() - parseStart;
  data.isValid = (data.windSpeed >= 0.0 || data.windDirection >= 0);
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== LYMINGTON WEATHER STATION ===");
  Serial.printf("Wind Speed: %.1f knots (%.1f m/s)\n", windSpeedRaw, data.windSpeed);
  Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
  Serial.printf("Location: %s\n", data.location.c_str());
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  if (delay > 0) {
    Serial.printf("Data Delay: %d minutes\n", delay);
  }
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("=================================");
}

// Removed old connectWiFi function - now using connectToStoredNetwork()

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
// HTML PARSING UTILITY FUNCTIONS
// =============================================================================

// Extract a float value that appears after a specific phrase and before an end phrase
float extractFloatAfterPhrase(String text, String startPhrase, String endPhrase) {
  int startIndex = text.indexOf(startPhrase);
  if (startIndex == -1) {
    DEBUG_PRINTF("[DEBUG] Start phrase '%s' not found\n", startPhrase.c_str());
    return 0.0;
  }
  
  startIndex += startPhrase.length();
  
  int endIndex;
  if (endPhrase.length() > 0) {
    endIndex = text.indexOf(endPhrase, startIndex);
    if (endIndex == -1) {
      DEBUG_PRINTF("[DEBUG] End phrase '%s' not found after start\n", endPhrase.c_str());
      return 0.0;
    }
  } else {
    // Find next space or end of string if no end phrase specified
    endIndex = text.indexOf(' ', startIndex);
    if (endIndex == -1) endIndex = text.length();
  }
  
  String valueStr = text.substring(startIndex, endIndex);
  valueStr.trim();
  
  DEBUG_PRINTF("[DEBUG] Extracted value string: '%s'\n", valueStr.c_str());
  
  return valueStr.toFloat();
}

// Extract a string value that appears after a specific phrase
String extractStringAfterPhrase(String text, String startPhrase, String endPhrase) {
  int startIndex = text.indexOf(startPhrase);
  if (startIndex == -1) {
    DEBUG_PRINTF("[DEBUG] Start phrase '%s' not found\n", startPhrase.c_str());
    return "";
  }
  
  startIndex += startPhrase.length();
  
  int endIndex;
  if (endPhrase.length() > 0) {
    endIndex = text.indexOf(endPhrase, startIndex);
    if (endIndex == -1) endIndex = text.length();
  } else {
    // Take everything to the end of the string
    endIndex = text.length();
  }
  
  String result = text.substring(startIndex, endIndex);
  result.trim();
  
  DEBUG_PRINTF("[DEBUG] Extracted string: '%s'\n", result.c_str());
  
  return result;
}

// Convert knots to m/s
float knotsToMeterPerSecond(float knots) {
  return knots * 0.514444;
}

// Convert m/s to knots
float meterPerSecondToKnots(float mps) {
  return mps / 0.514444;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void initializeWeatherData(WeatherData* data) {
  data->temperature = 0.0;
  data->humidity = 0.0;
  data->pressure = 0.0;
  data->windSpeed = 0.0;
  data->windGust = 0.0;
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
  if (data->windGust > 0.0) {
    Serial.printf("Wind Gust: %.1f m/s\n", data->windGust);
  }
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

// =============================================================================
// WIFI MANAGEMENT FUNCTIONS
// =============================================================================

void initializeWiFiManagement() {
  // Initialize preferences for persistent storage
  preferences.begin(PREFS_NAMESPACE, false);
  
  // Load stored credentials
  loadStoredCredentials();
  
  DEBUG_PRINTF("[DEBUG] WiFi management initialized. Configured: %s\n", 
               storedCredentials.isConfigured ? "YES" : "NO");
  
  if (storedCredentials.isConfigured) {
    DEBUG_PRINTF("[DEBUG] Stored SSID: %s\n", storedCredentials.ssid);
  }
}

void loadStoredCredentials() {
  // Clear credentials structure
  memset(&storedCredentials, 0, sizeof(WiFiCredentials));
  
  // Check if WiFi is configured
  storedCredentials.isConfigured = preferences.getBool(PREFS_WIFI_CONFIGURED, false);
  
  if (storedCredentials.isConfigured) {
    // Load SSID and password
    preferences.getString(PREFS_WIFI_SSID, storedCredentials.ssid, WIFI_SSID_MAX_LEN);
    preferences.getString(PREFS_WIFI_PASS, storedCredentials.password, WIFI_PASS_MAX_LEN);
  }
}

void saveCredentials(const char* ssid, const char* password) {
  // Store credentials in preferences
  preferences.putString(PREFS_WIFI_SSID, ssid);
  preferences.putString(PREFS_WIFI_PASS, password);
  preferences.putBool(PREFS_WIFI_CONFIGURED, true);
  
  // Update local copy
  strncpy(storedCredentials.ssid, ssid, WIFI_SSID_MAX_LEN);
  strncpy(storedCredentials.password, password, WIFI_PASS_MAX_LEN);
  storedCredentials.isConfigured = true;
  
  Serial.printf("[INFO] WiFi credentials saved for '%s'\n", ssid);
}

void clearStoredCredentials() {
  preferences.putBool(PREFS_WIFI_CONFIGURED, false);
  preferences.remove(PREFS_WIFI_SSID);
  preferences.remove(PREFS_WIFI_PASS);
  
  memset(&storedCredentials, 0, sizeof(WiFiCredentials));
  
  Serial.println("[INFO] Stored WiFi credentials cleared");
}

void autoConnectWiFi() {
  if (storedCredentials.isConfigured) {
    Serial.printf("[INFO] Attempting auto-connect to '%s'...\n", storedCredentials.ssid);
    connectToStoredNetwork();
  } else {
    Serial.println("[INFO] No stored WiFi credentials. Use 'wifi scan' to set up WiFi.");
  }
}

void connectToStoredNetwork() {
  if (!storedCredentials.isConfigured) {
    Serial.println("[ERROR] No stored WiFi credentials available");
    return;
  }
  
  currentWiFiState = WIFI_CONNECTING;
  WiFi.begin(storedCredentials.ssid, storedCredentials.password);
  
  unsigned long startTime = millis();
  Serial.print("Connecting");
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    currentWiFiState = WIFI_CONNECTED;
    Serial.println("[SUCCESS] WiFi connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    digitalWrite(LED_BUILTIN_PIN, HIGH);
  } else {
    currentWiFiState = WIFI_CONNECTION_FAILED;
    Serial.println("[ERROR] WiFi connection failed after 40 seconds");
    Serial.println("[INFO] Use 'wifi scan' to select a different network");
    lastWiFiReconnectAttempt = millis();
  }
}

void scanWiFiNetworks() {
  if (currentWiFiState == WIFI_SCANNING) {
    Serial.println("[INFO] WiFi scan already in progress");
    return;
  }
  
  Serial.println("[INFO] Scanning for WiFi networks...");
  currentWiFiState = WIFI_SCANNING;
  
  WiFi.disconnect();
  delay(100);
  
  int networksFound = WiFi.scanNetworks();
  
  if (networksFound == 0) {
    Serial.println("[ERROR] No WiFi networks found");
    currentWiFiState = WIFI_DISCONNECTED;
    return;
  }
  
  // Store scan results
  numScannedNetworks = min(networksFound, MAX_WIFI_NETWORKS);
  
  Serial.println("\nAvailable WiFi Networks:");
  Serial.println("========================");
  
  for (int i = 0; i < numScannedNetworks; i++) {
    scannedNetworks[i].ssid = WiFi.SSID(i);
    scannedNetworks[i].rssi = WiFi.RSSI(i);
    scannedNetworks[i].channel = WiFi.channel(i);
    scannedNetworks[i].security = getSecurityType(WiFi.encryptionType(i));
    
    // Display network info
    Serial.printf("%2d: %-20s", i, scannedNetworks[i].ssid.c_str());
    Serial.printf(" %4d dBm  Ch:%2d  %s\n", 
                  scannedNetworks[i].rssi,
                  scannedNetworks[i].channel,
                  getSecurityString(scannedNetworks[i].security));
  }
  
  Serial.println("\nUse 'wifi select <number>' to connect to a network");
  Serial.println("Example: wifi select 0");
  
  currentWiFiState = WIFI_SCAN_COMPLETE;
}

WiFiSecurityType getSecurityType(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN: return WIFI_SECURITY_OPEN;
    case WIFI_AUTH_WEP: return WIFI_SECURITY_WEP;
    case WIFI_AUTH_WPA_PSK: return WIFI_SECURITY_WPA;
    case WIFI_AUTH_WPA2_PSK: return WIFI_SECURITY_WPA2;
    case WIFI_AUTH_WPA_WPA2_PSK: return WIFI_SECURITY_WPA2;
    case WIFI_AUTH_WPA3_PSK: return WIFI_SECURITY_WPA3;
    default: return WIFI_SECURITY_UNKNOWN;
  }
}

const char* getSecurityString(WiFiSecurityType security) {
  switch (security) {
    case WIFI_SECURITY_OPEN: return "Open";
    case WIFI_SECURITY_WEP: return "WEP";
    case WIFI_SECURITY_WPA: return "WPA";
    case WIFI_SECURITY_WPA2: return "WPA2";
    case WIFI_SECURITY_WPA3: return "WPA3";
    default: return "Unknown";
  }
}

void selectWiFiNetwork(int networkIndex) {
  if (currentWiFiState != WIFI_SCAN_COMPLETE) {
    Serial.println("[ERROR] No scan results available. Use 'wifi scan' first.");
    return;
  }
  
  if (networkIndex < 0 || networkIndex >= numScannedNetworks) {
    Serial.printf("[ERROR] Invalid network index. Valid range: 0-%d\n", numScannedNetworks - 1);
    return;
  }
  
  selectedNetworkIndex = networkIndex;
  WiFiNetworkInfo* selectedNetwork = &scannedNetworks[networkIndex];
  
  Serial.printf("[INFO] Selected network: %s\n", selectedNetwork->ssid.c_str());
  
  if (selectedNetwork->security == WIFI_SECURITY_OPEN) {
    // Open network - connect immediately
    Serial.println("[INFO] Open network detected - connecting...");
    connectToSelectedNetwork("");
  } else {
    // Secured network - request password
    Serial.printf("[INFO] Enter password for '%s': ", selectedNetwork->ssid.c_str());
    awaitingPasswordInput = true;
  }
}

void connectToSelectedNetwork(String password) {
  if (selectedNetworkIndex < 0 || selectedNetworkIndex >= numScannedNetworks) {
    Serial.println("[ERROR] No network selected");
    return;
  }
  
  WiFiNetworkInfo* network = &scannedNetworks[selectedNetworkIndex];
  
  Serial.printf("[INFO] Connecting to '%s'...\n", network->ssid.c_str());
  currentWiFiState = WIFI_CONNECTING;
  
  // Attempt connection
  if (password.length() > 0) {
    WiFi.begin(network->ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(network->ssid.c_str());
  }
  
  unsigned long startTime = millis();
  Serial.print("Connecting");
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    currentWiFiState = WIFI_CONNECTED;
    Serial.println("[SUCCESS] WiFi connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    
    // Save credentials for future use
    saveCredentials(network->ssid.c_str(), password.c_str());
    
    digitalWrite(LED_BUILTIN_PIN, HIGH);
  } else {
    currentWiFiState = WIFI_CONNECTION_FAILED;
    Serial.println("[ERROR] WiFi connection failed - incorrect password or other issue");
  }
  
  // Reset selection state
  selectedNetworkIndex = -1;
  awaitingPasswordInput = false;
}

void showWiFiInfo() {
  Serial.println("\nWiFi Configuration:");
  Serial.println("===================");
  
  Serial.printf("Current State: %s\n", getWiFiStateString());
  
  if (storedCredentials.isConfigured) {
    Serial.printf("Stored Network: %s\n", storedCredentials.ssid);
    Serial.println("Status: Configured");
  } else {
    Serial.println("Status: Not configured");
    Serial.println("Use 'wifi scan' to set up WiFi connection");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nCurrent Connection:");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Channel: %d\n", WiFi.channel());
    Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  }
  
  if (numScannedNetworks > 0) {
    Serial.printf("\nLast Scan Results: %d networks found\n", numScannedNetworks);
    Serial.printf("Use 'wifi scan' to refresh\n");
  }
  
  Serial.println();
}
