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

// Weather Station Refresh Management
unsigned long lastBramblesRefresh = 0;
unsigned long lastSeaviewRefresh = 0;
unsigned long lastLymingtonRefresh = 0;
const unsigned long WEATHER_REFRESH_INTERVAL = 120000; // 2 minutes in milliseconds

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
  
  String url = "https://www.southamptonvts.co.uk/BackgroundSite/Ajax/LoadXmlFileWithTransform?xmlFilePath=D%3A%5Cftp%5Csouthampton%5CBramble.xml&xslFilePath=D%3A%5Cwwwroot%5CCMS_Southampton%5Ccontent%5Cfiles%5Cassets%5CSotonSnapshotmetBramble.xsl&w=51";
  
  // Retry logic for connection issues
  int maxRetries = 3;
  int retryDelay = 2000; // 2 seconds between retries
  
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    
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
      http.end();
      return; // Success - exit retry loop
      
    } else {
      Serial.printf("[ERROR] Attempt %d/%d failed. HTTP %d: %s\n", 
                    attempt, maxRetries, httpCode, http.errorToString(httpCode).c_str());
      
      http.end();
      
      // If connection refused (-1) or timeout, retry after delay
      if ((httpCode == -1 || httpCode == -11) && attempt < maxRetries) {
        Serial.printf("[INFO] Retrying in %d seconds...\n", retryDelay / 1000);
        delay(retryDelay);
        retryDelay *= 1.5; // Exponential backoff
      } else {
        // Final failure or non-retryable error
        break;
      }
    }
  }
  
  Serial.println("[ERROR] Failed to fetch Brambles data after all retry attempts");
}

void parseBramblesData(String htmlData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Brambles weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Brambles Bank";
  
  unsigned long parseStart = millis();
  
  // Parse HTML table format
  // New format: <td>Wind Speed</td><td>15.7 Knots</td>
  // Extract wind speed in knots
  float windSpeedKnots = extractFloatFromTableCell(htmlData, "Wind Speed");
  if (windSpeedKnots > 0) {
    data.windSpeed = windSpeedKnots * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Speed: %.1f knots = %.1f m/s\n", windSpeedKnots, data.windSpeed);
  } else {
    DEBUG_PRINTLN("[DEBUG] Wind Speed: n/a");
  }
  
  // Extract wind gust in knots
  float windGustKnots = extractFloatFromTableCell(htmlData, "Max Gust");
  if (windGustKnots > 0) {
    data.windGust = windGustKnots * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Gust: %.1f knots = %.1f m/s\n", windGustKnots, data.windGust);
  } else {
    DEBUG_PRINTLN("[DEBUG] Wind Gust: n/a");
  }
  
  // Extract wind direction in degrees
  data.windDirection = (int)extractFloatFromTableCell(htmlData, "Wind Direction");
  DEBUG_PRINTF("[DEBUG] Wind Direction: %d degrees\n", data.windDirection);
  
  // Extract air temperature in Celsius
  data.temperature = extractFloatFromTableCell(htmlData, "Air Temp");
  DEBUG_PRINTF("[DEBUG] Temperature: %.1f°C\n", data.temperature);
  
  // Extract pressure in mBar and convert to hPa (same value, different name)
  data.pressure = extractFloatFromTableCell(htmlData, "Pressure");
  DEBUG_PRINTF("[DEBUG] Pressure: %.1f mBar (%.1f hPa)\n", data.pressure, data.pressure);
  
  // Extract timestamp from HTML table format
  String timestamp = extractStringFromTableCell(htmlData, "Updated");
  if (timestamp.length() > 0) {
    data.timestamp = timestamp;
    DEBUG_PRINTF("[DEBUG] Timestamp: %s\n", timestamp.c_str());
  } else {
    DEBUG_PRINTLN("[DEBUG] Timestamp: n/a");
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = (data.windSpeed > 0 || data.windDirection > 0 || data.temperature > 0);
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== BRAMBLES BANK WEATHER ===");
  Serial.printf("Wind Speed: %.1f knots (%.1f m/s)\n", windSpeedKnots, data.windSpeed);
  Serial.printf("Wind Gust: %.1f knots (%.1f m/s)\n", windGustKnots, data.windGust);
  Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
  Serial.printf("Air Temperature: %.1f°C\n", data.temperature);
  Serial.printf("Air Pressure: %.1f mBar\n", data.pressure);
  Serial.printf("Last Updated: %s\n", ensureGMTTimestamp(data.timestamp).c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("=============================");
}

void fetchSeaviewWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Use 'wifi connect' first.");
    return;
  }
  
  Serial.println("[INFO] Fetching Seaview historical weather data...");
  
  // Use historical data API with 1-minute window for averaged/peak calculations
  time_t now = time(nullptr);
  time_t fromTime = now - 60; // 1 minute ago for memory-efficient processing
  time_t toTime = now;
  
  // Format timestamps for URL
  char fromStr[32], toStr[32];
  sprintf(fromStr, "%ld", fromTime);
  sprintf(toStr, "%ld", toTime);
  
  String url = "https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=data&from=" + String(fromStr) + "&to=" + String(toStr);
  
  DEBUG_PRINTF("[DEBUG] Fetching historical data from %s to %s\n", fromStr, toStr);
  
  // Retry logic for connection issues
  int maxRetries = 3;
  int retryDelay = 2000; // 2 seconds between retries
  
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    
    // Begin HTTPS request 
    http.begin(url);
    
    // Set specific headers for navis-livedata.com (session-based headers)
    http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36");
    http.addHeader("Accept", "*/*");
    http.addHeader("Accept-Language", "en-GB,en;q=0.9,fr;q=0.8");
    http.addHeader("Connection", "keep-alive");
    http.addHeader("Referer", "https://www.navis-livedata.com/view.php?u=36371");
    http.addHeader("Cookie", "PHPSESSID=temp_session"); // Basic session header
    http.addHeader("Sec-Fetch-Dest", "empty");
    http.addHeader("Sec-Fetch-Mode", "cors");
    http.addHeader("Sec-Fetch-Site", "same-origin");
    
    unsigned long startTime = millis();
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      unsigned long fetchTime = millis() - startTime;
      
      DEBUG_PRINTF("[DEBUG] Received %d bytes in %lu ms\n", payload.length(), fetchTime);
      DEBUG_PRINTF("[DEBUG] Raw historical response: %s\n", payload.c_str());
      
      // Check if we got an error response
      if (payload == "error%" || payload.length() < 10) {
        Serial.printf("[WARNING] Attempt %d/%d: Historical API returned error: %s\n", 
                      attempt, maxRetries, payload.c_str());
        
        // Fallback to live data on final attempt
        if (attempt == maxRetries) {
          Serial.println("[INFO] Falling back to live data API...");
          fetchSeaviewLiveData();
          http.end();
          return;
        }
        
        http.end();
        if (attempt < maxRetries) {
          Serial.printf("[INFO] Retrying historical API in %d seconds...\n", retryDelay / 1000);
          delay(retryDelay);
          retryDelay *= 1.5; // Exponential backoff
          continue;
        }
      } else {
        // Parse the Seaview historical weather data
        parseSeaviewHistoricalData(payload, fetchTime);
        http.end();
        return; // Success - exit retry loop
      }
      
    } else {
      Serial.printf("[ERROR] Attempt %d/%d failed. HTTP %d: %s\n", 
                    attempt, maxRetries, httpCode, http.errorToString(httpCode).c_str());
      
      http.end();
      
      // Fallback to live data on final attempt
      if (attempt == maxRetries) {
        Serial.println("[INFO] Falling back to live data API...");
        fetchSeaviewLiveData();
        return;
      }
      
      // If connection refused (-1) or timeout, retry after delay
      if ((httpCode == -1 || httpCode == -11) && attempt < maxRetries) {
        Serial.printf("[INFO] Retrying historical API in %d seconds...\n", retryDelay / 1000);
        delay(retryDelay);
        retryDelay *= 1.5; // Exponential backoff
      } else {
        // Final failure or non-retryable error
        break;
      }
    }
  }
  
  Serial.println("[ERROR] Failed to fetch Seaview historical data, trying live fallback...");
  fetchSeaviewLiveData();
}

// Fallback function for live data (original implementation)
void fetchSeaviewLiveData() {
  Serial.println("[INFO] Fetching Seaview live data (fallback)...");
  
  String url = "https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=live";
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  // Begin HTTPS request 
  http.begin(url);
  
  // Set headers for live data
  http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36");
  http.addHeader("Accept", "*/*");
  http.addHeader("Referer", "https://www.navis-livedata.com/view.php?u=36371");
  
  unsigned long startTime = millis();
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    DEBUG_PRINTF("[DEBUG] Live data received %d bytes in %lu ms\n", payload.length(), fetchTime);
    
    if (payload != "error%" && payload.length() >= 10) {
      // Parse as live data (hex format)
      parseSeaviewLiveData(payload, fetchTime);
    } else {
      Serial.printf("[ERROR] Live data API also failed: %s\n", payload.c_str());
    }
  } else {
    Serial.printf("[ERROR] Live data fetch failed. HTTP %d\n", httpCode);
  }
  
  http.end();
}

// Parse Seaview historical data (hex format)
void parseSeaviewHistoricalData(String hexData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Seaview historical weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Seaview, Isle of Wight";
  
  unsigned long parseStart = millis();
  
  // Parse hex data - expect colon-separated hex strings
  // Format: "timestamp1:hexdata1,timestamp2:hexdata2,..."
  
  // Memory-efficient processing: use small arrays for wind speed collection
  const int MAX_SAMPLES = 20; // Limit for ESP32 memory constraints
  float windSpeeds[MAX_SAMPLES];
  int windDirections[MAX_SAMPLES];
  int sampleCount = 0;
  
  // Split by comma to get individual readings
  int lastIndex = 0;
  int commaIndex = hexData.indexOf(',', lastIndex);
  
  while ((commaIndex != -1 || lastIndex < hexData.length()) && sampleCount < MAX_SAMPLES) {
    String reading;
    if (commaIndex != -1) {
      reading = hexData.substring(lastIndex, commaIndex);
      lastIndex = commaIndex + 1;
      commaIndex = hexData.indexOf(',', lastIndex);
    } else {
      reading = hexData.substring(lastIndex);
      lastIndex = hexData.length();
    }
    
    // Parse individual reading: "timestamp:hexdata"
    int colonIndex = reading.indexOf(':');
    if (colonIndex != -1 && colonIndex < reading.length() - 1) {
      String timestamp = reading.substring(0, colonIndex);
      String hexValue = reading.substring(colonIndex + 1);
      
      DEBUG_PRINTF("[DEBUG] Processing sample %d: time=%s, hex=%s\n", 
                   sampleCount, timestamp.c_str(), hexValue.c_str());
      
      // Extract wind data from hex
      if (hexValue.length() >= 8) {
        windSpeeds[sampleCount] = extractWindSpeedFromHex(hexValue);
        windDirections[sampleCount] = extractWindDirectionFromHex(hexValue);
        sampleCount++;
      }
    }
  }
  
  if (sampleCount > 0) {
    // Calculate statistics
    float avgSpeed, peakSpeed;
    calculateWindStats(windSpeeds, sampleCount, &avgSpeed, &peakSpeed);
    
    // Convert from knots to m/s
    data.windSpeed = avgSpeed * 0.514444;
    data.windGust = peakSpeed * 0.514444;
    
    // Calculate average direction (simple average - could be improved with circular statistics)
    int totalDirection = 0;
    for (int i = 0; i < sampleCount; i++) {
      totalDirection += windDirections[i];
    }
    data.windDirection = totalDirection / sampleCount;
    
    data.timestamp = "Historical average (1 min)";
    
    DEBUG_PRINTF("[DEBUG] Calculated from %d samples: avg=%.1f kts, peak=%.1f kts, dir=%d°\n", 
                 sampleCount, avgSpeed, peakSpeed, data.windDirection);
  } else {
    Serial.println("[ERROR] No valid samples found in historical data");
    data.isValid = false;
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = (sampleCount > 0 && (data.windSpeed > 0 || data.windDirection > 0));
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== SEAVIEW WEATHER STATION (Historical) ===");
  if (data.isValid) {
    Serial.printf("Wind Speed (avg): %.1f knots (%.1f m/s)\n", meterPerSecondToKnots(data.windSpeed), data.windSpeed);
    Serial.printf("Wind Gust (peak): %.1f knots (%.1f m/s)\n", meterPerSecondToKnots(data.windGust), data.windGust);
    Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
    Serial.printf("Samples processed: %d\n", sampleCount);
  } else {
    Serial.println("No valid data available");
  }
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("===========================================");
}

// Parse Seaview live data (hex format) - fallback function
void parseSeaviewLiveData(String hexData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Seaview live weather data (fallback)...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Seaview, Isle of Wight";
  
  unsigned long parseStart = millis();
  
  // Parse single hex reading - expect 8 hex characters
  String cleanHex = hexData;
  cleanHex.trim();
  
  if (cleanHex.length() >= 8) {
    // Extract wind speed and direction
    float windSpeedKnots = extractWindSpeedFromHex(cleanHex);
    int windDirection = extractWindDirectionFromHex(cleanHex);
    
    data.windSpeed = windSpeedKnots * 0.514444; // Convert knots to m/s
    data.windGust = data.windSpeed; // Live data: gust = current speed (instantaneous)
    data.windDirection = windDirection;
    data.timestamp = "Live instantaneous data";
    
    DEBUG_PRINTF("[DEBUG] Live data: speed=%.1f kts, dir=%d°\n", windSpeedKnots, windDirection);
  } else {
    Serial.printf("[ERROR] Invalid hex data format: %s\n", cleanHex.c_str());
    data.isValid = false;
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = (data.windSpeed >= 0 || data.windDirection >= 0);
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== SEAVIEW WEATHER STATION (Live) ===");
  if (data.isValid) {
    Serial.printf("Wind Speed: %.1f knots (%.1f m/s)\n", meterPerSecondToKnots(data.windSpeed), data.windSpeed);
    Serial.printf("Wind Gust: %.1f knots (%.1f m/s)\n", meterPerSecondToKnots(data.windGust), data.windGust);
    Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
    Serial.println("Note: Live data - gust equals current speed");
  } else {
    Serial.println("No valid data available");
  }
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("=======================================");
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
  
  // Parse current conditions - prioritize 2-minute averages for more stable readings
  for (JsonObject condition : conditions) {
    String dataName = condition["sensorDataName"] | "";
    String value = condition["convertedValue"] | "";
    String unit = condition["unitLabel"] | "";
    
    // Skip invalid values
    if (value == "--" || value == "" || value == "null") continue;
    
    DEBUG_PRINTF("[DEBUG] Found: %s = %s %s\n", dataName.c_str(), value.c_str(), unit.c_str());
    
    // Prioritize 2-minute average wind speed for more stable readings
    if (dataName.indexOf("2 Min") >= 0 && dataName.indexOf("Wind Speed") >= 0) {
      windSpeed = value.toFloat();
      if (unit == "knots") {
        data.windSpeed = windSpeed * 0.514444; // Convert knots to m/s
      } else {
        data.windSpeed = windSpeed; // Assume m/s
      }
      DEBUG_PRINTF("[DEBUG] Using 2-min avg wind speed: %.1f\n", windSpeed);
    } else if (dataName == "Wind Speed" && data.windSpeed == 0.0) {
      // Fallback to current wind speed if no 2-min average found
      windSpeed = value.toFloat();
      if (unit == "knots") {
        data.windSpeed = windSpeed * 0.514444; // Convert knots to m/s
      } else {
        data.windSpeed = windSpeed; // Assume m/s
      }
    }
    
    // Prioritize 2-minute average wind direction
    if (dataName.indexOf("2 Min") >= 0 && dataName.indexOf("Wind Direction") >= 0) {
      data.windDirection = value.toInt();
      DEBUG_PRINTF("[DEBUG] Using 2-min avg wind direction: %d\n", data.windDirection);
    } else if (dataName == "Wind Direction" && data.windDirection == 0) {
      // Fallback to current wind direction if no 2-min average found
      data.windDirection = value.toInt();
    }
    
    // Look for wind gust data (10-minute high or other high values)
    if (dataName.indexOf("High Wind Speed") >= 0 || dataName.indexOf("10 Min High") >= 0) {
      float gustValue = value.toFloat();
      if (gustValue > windGust) {
        windGust = gustValue;
        if (unit == "knots") {
          data.windGust = windGust * 0.514444; // Convert knots to m/s
        } else {
          data.windGust = windGust; // Assume m/s
        }
        DEBUG_PRINTF("[DEBUG] Wind gust found: %.1f %s\n", gustValue, unit.c_str());
      }
    }
    
    // Temperature data
    if (dataName.indexOf("Temperature") >= 0 || dataName.indexOf("Temp") >= 0) {
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
  
  Serial.println("[INFO] Fetching Lymington enhanced weather data...");
  
  // Use the enhanced infowindow endpoint for proper averaged and gust data
  String url = "https://lymingtonharbour.sofarocean.com/infowindow.ggl";
  
  // Retry logic for connection issues
  int maxRetries = 3;
  int retryDelay = 2000; // 2 seconds between retries
  
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    
    // Begin HTTPS request 
    http.begin(url);
    
    // Set specific headers for Lymington Harbor Sofar Ocean endpoint
    http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
    http.addHeader("Accept", "*/*");
    http.addHeader("Accept-Language", "en-GB,en;q=0.9");
    http.addHeader("Referer", "https://lymingtonharbour.sofarocean.com/");
    http.addHeader("Origin", "https://lymingtonharbour.sofarocean.com");
    http.addHeader("Connection", "keep-alive");
    
    unsigned long startTime = millis();
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      unsigned long fetchTime = millis() - startTime;
      
      DEBUG_PRINTF("[DEBUG] Received %d bytes in %lu ms\n", payload.length(), fetchTime);
      DEBUG_PRINTF("[DEBUG] Raw enhanced response: %s\n", payload.c_str());
      
      // Parse the enhanced Lymington weather data
      parseLymingtonEnhancedData(payload, fetchTime);
      http.end();
      return; // Success - exit retry loop
      
    } else {
      Serial.printf("[ERROR] Attempt %d/%d failed. HTTP %d: %s\n", 
                    attempt, maxRetries, httpCode, http.errorToString(httpCode).c_str());
      
      http.end();
      
      // Fallback to basic API on final attempt
      if (attempt == maxRetries) {
        Serial.println("[INFO] Falling back to basic WeatherFile API...");
        fetchLymingtonBasicWeather();
        return;
      }
      
      // If connection refused (-1) or timeout, retry after delay
      if ((httpCode == -1 || httpCode == -11) && attempt < maxRetries) {
        Serial.printf("[INFO] Retrying enhanced API in %d seconds...\n", retryDelay / 1000);
        delay(retryDelay);
        retryDelay *= 1.5; // Exponential backoff
      } else {
        // Final failure or non-retryable error
        break;
      }
    }
  }
  
  Serial.println("[ERROR] Failed to fetch enhanced Lymington data, trying basic fallback...");
  fetchLymingtonBasicWeather();
}

// Fallback function for basic WeatherFile data (original implementation)
void fetchLymingtonBasicWeather() {
  Serial.println("[INFO] Fetching Lymington basic data (fallback)...");
  
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);
  
  String url = "https://weatherfile.com/V03/loc/GBR00001/latest.json";
  
  // Begin HTTPS request 
  http.begin(url);
  
  // Set specific headers for WeatherFile.com - discovered via browser dev tools
  http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
  http.addHeader("Accept", "*/*");
  http.addHeader("X-Requested-With", "XMLHttpRequest");
  http.addHeader("Referer", "https://weatherfile.com/location?loc_id=GBR00001&wt=KTS");
  http.addHeader("Origin", "https://weatherfile.com");
  http.addHeader("Content-Length", "0");
  http.addHeader("wf-tkn", "PUBLIC"); // Key authentication header!
  
  unsigned long startTime = millis();
  // Use POST request with empty body (as discovered in browser dev tools)
  int httpCode = http.POST("");
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    unsigned long fetchTime = millis() - startTime;
    
    DEBUG_PRINTF("[DEBUG] Basic data received %d bytes in %lu ms\n", payload.length(), fetchTime);
    
    // Parse the basic Lymington weather data
    parseLymingtonData(payload, fetchTime);
    
  } else {
    Serial.printf("[ERROR] Basic WeatherFile API also failed. HTTP %d: %s\n", 
                  httpCode, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

// Parse Lymington enhanced data from Sofar Ocean endpoint
void parseLymingtonEnhancedData(String jsonData, unsigned long fetchTime) {
  Serial.println("\n[INFO] Parsing Lymington enhanced weather data...");
  
  // Initialize weather data
  WeatherData data;
  initializeWeatherData(&data);
  data.location = "Lymington Harbor (Enhanced)";
  
  unsigned long parseStart = millis();
  
  // Parse JSON using ArduinoJson library
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.printf("[ERROR] Enhanced JSON parsing failed: %s\n", error.c_str());
    return;
  }
  
  // Extract wind data from enhanced endpoint structure
  // Expected format: {"wsa": avg_wind_speed, "wsh": high_wind_speed, "wsl": low_wind_speed, "wdc": direction, ...}
  
  float windSpeedAvg = 0.0; // Average wind speed (use as primary)
  float windSpeedHigh = 0.0; // High wind speed (use as gust)
  float windDirection = 0.0;
  
  // Parse wind speed average (wsa)
  if (doc.containsKey("wsa")) {
    windSpeedAvg = doc["wsa"] | 0.0;
    data.windSpeed = windSpeedAvg * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Speed Avg (wsa): %.1f knots = %.1f m/s\n", windSpeedAvg, data.windSpeed);
  }
  
  // Parse wind speed high (wsh) as gust
  if (doc.containsKey("wsh")) {
    windSpeedHigh = doc["wsh"] | 0.0;
    data.windGust = windSpeedHigh * 0.514444; // Convert knots to m/s
    DEBUG_PRINTF("[DEBUG] Wind Speed High (wsh): %.1f knots = %.1f m/s\n", windSpeedHigh, data.windGust);
  }
  
  // Parse wind direction (wdc)
  if (doc.containsKey("wdc")) {
    windDirection = doc["wdc"] | 0.0;
    data.windDirection = (int)windDirection;
    DEBUG_PRINTF("[DEBUG] Wind Direction (wdc): %.0f degrees\n", windDirection);
  }
  
  // Parse additional parameters if available
  if (doc.containsKey("at")) {
    data.temperature = doc["at"] | 0.0;
    DEBUG_PRINTF("[DEBUG] Air Temperature: %.1f°C\n", data.temperature);
  }
  
  if (doc.containsKey("bp")) {
    data.pressure = doc["bp"] | 0.0;
    DEBUG_PRINTF("[DEBUG] Barometric Pressure: %.1f hPa\n", data.pressure);
  }
  
  // Extract timestamp if available
  if (doc.containsKey("timestamp")) {
    data.timestamp = doc["timestamp"] | "Enhanced data";
  } else {
    data.timestamp = "Enhanced live data";
  }
  
  data.parseTime = millis() - parseStart;
  data.isValid = (data.windSpeed >= 0.0 || data.windDirection >= 0);
  
  // Store as last parsed data
  lastParsedData = data;
  
  // Display results
  Serial.println("\n=== LYMINGTON WEATHER STATION (Enhanced) ===");
  if (data.isValid) {
    Serial.printf("Wind Speed (avg): %.1f knots (%.1f m/s)\n", windSpeedAvg, data.windSpeed);
    if (windSpeedHigh > 0.0) {
      Serial.printf("Wind Gust (high): %.1f knots (%.1f m/s)\n", windSpeedHigh, data.windGust);
    }
    Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
    if (data.temperature > 0.0) {
      Serial.printf("Air Temperature: %.1f°C\n", data.temperature);
    }
    if (data.pressure > 0.0) {
      Serial.printf("Barometric Pressure: %.1f hPa\n", data.pressure);
    }
  } else {
    Serial.println("No valid data available");
  }
  Serial.printf("Location: %s\n", data.location.c_str());
  Serial.printf("Last Updated: %s\n", data.timestamp.c_str());
  Serial.printf("Fetch Time: %lu ms, Parse Time: %lu ms\n", fetchTime, data.parseTime);
  Serial.println("=============================================");
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
  
  // Parse weather parameters - data is already in m/s despite wt=KTS URL parameter
  // WeatherFile API provides various wind parameters:
  // wdc = wind direction, wsc = wind speed (m/s), wsc_avg = average wind speed
  // wsc_max = max wind speed (use for gust)
  
  float windSpeedRaw = 0.0;
  float windGustRaw = 0.0;
  
  // Parse all available wind parameters
  for (JsonPair kv : dataObj) {
    String key = kv.key().c_str();
    float value = kv.value() | 0.0;
    
    DEBUG_PRINTF("[DEBUG] Found parameter: %s = %.2f\n", key.c_str(), value);
    
    // Wind direction (prioritize average direction)
    if (key == "wdc_avg" || (key == "wdc" && data.windDirection == 0)) {
      data.windDirection = (int)value;
      DEBUG_PRINTF("[DEBUG] Wind Direction (%s): %d degrees\n", key.c_str(), data.windDirection);
    }
    
    // Wind speed - use average for stability (data already in m/s)
    if (key == "wsc_avg") {
      windSpeedRaw = value;
      data.windSpeed = windSpeedRaw; // No conversion needed - already m/s
      DEBUG_PRINTF("[DEBUG] Wind Speed (avg): %.2f m/s\n", data.windSpeed);
    } else if (key == "wsc" && windSpeedRaw == 0.0) {
      // Fallback to current wind speed if no average found
      windSpeedRaw = value;
      data.windSpeed = windSpeedRaw; // No conversion needed - already m/s
      DEBUG_PRINTF("[DEBUG] Wind Speed (current): %.2f m/s\n", data.windSpeed);
    }
    
    // Wind gust - use max wind speed as gust (data already in m/s)
    if (key == "wsc_max") {
      windGustRaw = value;
      data.windGust = windGustRaw; // No conversion needed - already m/s
      DEBUG_PRINTF("[DEBUG] Wind Gust (max): %.2f m/s\n", data.windGust);
    }
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
  Serial.printf("Wind Speed: %.1f m/s\n", data.windSpeed);
  if (windGustRaw > 0.0) {
    Serial.printf("Wind Gust: %.1f m/s\n", data.windGust);
  }
  Serial.printf("Wind Direction: %d degrees\n", data.windDirection);
  Serial.printf("Location: %s\n", data.location.c_str());
  Serial.printf("Last Updated: %s\n", ensureGMTTimestamp(data.timestamp).c_str());
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

// Extract float value from HTML table cell format: <td>Label</td><td>Value Units</td>
float extractFloatFromTableCell(String html, String label) {
  // Find the label cell
  String labelCell = "<td>" + label + "</td>";
  int labelIndex = html.indexOf(labelCell);
  if (labelIndex == -1) {
    DEBUG_PRINTF("[DEBUG] Label '%s' not found in HTML table\n", label.c_str());
    return 0.0;
  }
  
  // Find the next <td> tag after the label (should contain the value)
  int valueStart = html.indexOf("<td>", labelIndex + labelCell.length());
  if (valueStart == -1) {
    DEBUG_PRINTF("[DEBUG] Value cell not found after label '%s'\n", label.c_str());
    return 0.0;
  }
  
  int valueEnd = html.indexOf("</td>", valueStart);
  if (valueEnd == -1) {
    DEBUG_PRINTF("[DEBUG] Value cell end tag not found\n");
    return 0.0;
  }
  
  String valueCell = html.substring(valueStart + 4, valueEnd);
  valueCell.trim();
  
  // Extract numeric value (first token before space)
  int spaceIndex = valueCell.indexOf(' ');
  String numberStr;
  if (spaceIndex > 0) {
    numberStr = valueCell.substring(0, spaceIndex);
  } else {
    numberStr = valueCell;
  }
  
  DEBUG_PRINTF("[DEBUG] Extracted float for '%s': '%s' from cell '%s'\n", 
               label.c_str(), numberStr.c_str(), valueCell.c_str());
  
  return numberStr.toFloat();
}

// Extract string value from HTML table cell format: <td>Label</td><td>String Value</td>
String extractStringFromTableCell(String html, String label) {
  // Find the label cell
  String labelCell = "<td>" + label + "</td>";
  int labelIndex = html.indexOf(labelCell);
  if (labelIndex == -1) {
    DEBUG_PRINTF("[DEBUG] Label '%s' not found in HTML table\n", label.c_str());
    return "";
  }
  
  // Find the next <td> tag after the label (should contain the value)
  int valueStart = html.indexOf("<td>", labelIndex + labelCell.length());
  if (valueStart == -1) {
    DEBUG_PRINTF("[DEBUG] Value cell not found after label '%s'\n", label.c_str());
    return "";
  }
  
  int valueEnd = html.indexOf("</td>", valueStart);
  if (valueEnd == -1) {
    DEBUG_PRINTF("[DEBUG] Value cell end tag not found\n");
    return "";
  }
  
  String valueCell = html.substring(valueStart + 4, valueEnd);
  valueCell.trim();
  
  // Check for embedded HTML (like the timestamp div)
  int divStart = valueCell.indexOf("<div");
  if (divStart != -1) {
    int divContentStart = valueCell.indexOf(">", divStart);
    int divEnd = valueCell.indexOf("</div>", divContentStart);
    if (divContentStart != -1 && divEnd != -1) {
      String divContent = valueCell.substring(divContentStart + 1, divEnd);
      divContent.trim();
      DEBUG_PRINTF("[DEBUG] Extracted string from div for '%s': '%s'\n", label.c_str(), divContent.c_str());
      return divContent;
    }
  }
  
  DEBUG_PRINTF("[DEBUG] Extracted string for '%s': '%s'\n", label.c_str(), valueCell.c_str());
  return valueCell;
}

// Get current UTC timestamp string
String getCurrentUTCTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timestamp);
}

// Convert timestamp to GMT indication if needed
String ensureGMTTimestamp(String timestamp) {
  if (timestamp.length() == 0 || timestamp == "Live data") {
    return getCurrentUTCTimestamp();
  }
  
  // If timestamp doesn't end with Z or GMT, assume it needs conversion indication
  if (!timestamp.endsWith("Z") && timestamp.indexOf("GMT") == -1 && timestamp.indexOf("UTC") == -1) {
    return timestamp + " GMT";
  }
  
  return timestamp;
}

// =============================================================================
// HEX DATA PARSING UTILITY FUNCTIONS (for Seaview historical data)
// =============================================================================

// Parse hex string to integer value
unsigned long parseHexString(String hexStr) {
  unsigned long result = 0;
  for (int i = 0; i < hexStr.length(); i++) {
    char c = hexStr.charAt(i);
    result <<= 4;
    if (c >= '0' && c <= '9') {
      result += c - '0';
    } else if (c >= 'A' && c <= 'F') {
      result += c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      result += c - 'a' + 10;
    }
  }
  return result;
}

// Extract wind speed from Navis hex data (bits 16-31)
float extractWindSpeedFromHex(String hexData) {
  if (hexData.length() < 8) return 0.0;
  
  // Extract the wind speed portion (bits 16-31) - middle 4 hex digits
  String windSpeedHex = hexData.substring(2, 6);
  unsigned long rawValue = parseHexString(windSpeedHex);
  
  // Convert to knots (scaling factor from Navis documentation)
  float knots = rawValue * 0.01;
  
  DEBUG_PRINTF("[DEBUG] Wind speed hex: %s, raw: %lu, knots: %.2f\n", 
               windSpeedHex.c_str(), rawValue, knots);
  
  return knots;
}

// Extract wind direction from Navis hex data (bits 0-15)
int extractWindDirectionFromHex(String hexData) {
  if (hexData.length() < 8) return 0;
  
  // Extract the wind direction portion (bits 0-15) - last 4 hex digits
  String windDirHex = hexData.substring(4, 8);
  unsigned long rawValue = parseHexString(windDirHex);
  
  // Convert to degrees (scaling factor from Navis documentation)
  int degrees = (int)(rawValue * 0.1) % 360;
  
  DEBUG_PRINTF("[DEBUG] Wind direction hex: %s, raw: %lu, degrees: %d\n", 
               windDirHex.c_str(), rawValue, degrees);
  
  return degrees;
}

// Calculate statistics from array of wind speed values
void calculateWindStats(float* speeds, int count, float* avgSpeed, float* peakSpeed) {
  if (count == 0) {
    *avgSpeed = 0.0;
    *peakSpeed = 0.0;
    return;
  }
  
  float sum = 0.0;
  float max = 0.0;
  
  for (int i = 0; i < count; i++) {
    sum += speeds[i];
    if (speeds[i] > max) {
      max = speeds[i];
    }
  }
  
  *avgSpeed = sum / count;
  *peakSpeed = max;
  
  DEBUG_PRINTF("[DEBUG] Wind stats from %d samples: avg=%.2f, peak=%.2f\n", 
               count, *avgSpeed, *peakSpeed);
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
