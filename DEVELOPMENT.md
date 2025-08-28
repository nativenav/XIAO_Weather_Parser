# XIAO Weather Station Data Parser - Development Guide

## 📋 Project Overview

**Hardware**: XIAO ESP32C3 (ESP32-C3 RISC-V @ 160MHz)  
**Purpose**: Parse weather station data in JSON, XML, CSV formats  
**Libraries**: ArduinoJson, WiFiClientSecure, HTTPClient  
**GitHub**: https://github.com/nativenav/XIAO_Weather_Parser  
**Local Path**: `/Users/nives/Documents/Arduino/XIAO_Weather_Parser`

## 🚀 Quick Start Commands

### Navigate to Project
```bash
cd /Users/nives/Documents/Arduino/XIAO_Weather_Parser
```

### Check Project Status
```bash
git status
ls -la *.ino *.h *.md
```

## 🔧 Development Workflow

### Standard Git Operations
```bash
# Check what changed
git status
git diff

# Commit changes
git add .
git commit -m "Descriptive commit message"
git push origin main

# View history
git log --oneline -10
```

### View Detailed Git History
```bash
git log --graph --pretty=format:'%h - %an, %ar : %s'
```

## 🧪 Weather Parser Testing Commands

### Serial Monitor Commands (115200 baud)
```
help
status
wifi connect
parse json {"temperature":22.5,"humidity":65,"conditions":"sunny"}
parse csv timestamp,temperature,humidity,conditions\n2024-08-28T12:00:00Z,22.5,65,sunny
parse xml <weather><temperature>22.5</temperature><humidity>65</humidity></weather>
fetch https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_API_KEY
test
debug on
debug off
wifi disconnect
```

### Serial Monitor Setup
- **Baud Rate**: 115200
- **Line Ending**: Newline or Both NL & CR

## ⚙️ Hardware Configuration

### XIAO ESP32C3 Specifications
- **CPU**: ESP32-C3 RISC-V @ 160MHz
- **Memory**: 400KB SRAM, 4MB Flash
- **WiFi**: 802.11 b/g/n (2.4GHz only)
- **USB**: USB-C for programming and power
- **GPIO**: 11 digital I/O pins available

### Pin Connections (if needed for external sensors)
- **I2C**: SDA (D4), SCL (D5)
- **SPI**: MOSI (D10), MISO (D9), SCK (D8), SS (D7)
- **UART**: TX (D6), RX (D7)
- **ADC**: A0, A1, A2, A3

### Driver Configuration (`driver.h`)
```c
#pragma once
#define TARGET_BOARD_XIAO_ESP32C3

// Weather parsing configuration
#define WEATHER_BUFFER_SIZE 8192      // 8KB max data size
#define JSON_BUFFER_SIZE 4096         // JSON parsing buffer
#define CSV_MAX_COLUMNS 20            // Max CSV columns
#define XML_MAX_DEPTH 10              // Max XML nesting depth

// Network configuration
#define HTTP_TIMEOUT 10000            // 10 second timeout
#define WIFI_CONNECT_TIMEOUT 30000    // 30 second WiFi timeout
```

## 🌤️ Weather Data Features

### Supported Data Formats
- **JSON**: Standard weather API format (OpenWeatherMap, WeatherAPI, etc.)
- **CSV**: Comma-separated values with headers
- **XML**: Structured weather data markup
- **Custom**: Extensible parser framework

### Parsed Weather Fields
```cpp
struct WeatherData {
  float temperature;      // °C
  float humidity;         // %
  float pressure;         // hPa
  float windSpeed;        // m/s
  int windDirection;      // degrees
  String conditions;      // text description
  String timestamp;       // ISO 8601 format
  bool isValid;           // parse success flag
};
```

### Memory Management
- **Heap Usage**: ~4KB for JSON parsing
- **Stack Usage**: ~2KB for function calls
- **Buffer Limits**: 8KB max input data size
- **WiFi Memory**: ~40KB reserved by ESP32 core

## 🌐 Network Integration

### Popular Weather APIs
- **OpenWeatherMap**: `https://api.openweathermap.org/data/2.5/weather`
- **WeatherAPI**: `https://api.weatherapi.com/v1/current.json`
- **NOAA/NWS**: `https://api.weather.gov/stations/{stationId}/observations/latest`
- **AccuWeather**: `https://api.accuweather.com/current-conditions/v1`

### WiFi Configuration
```cpp
// Update in sketch
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Optional: Static IP configuration
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

## 🐛 Troubleshooting

### Common Issues
```bash
# If parsing fails
# 1. Check data format matches expected structure
# 2. Enable debug mode: debug on
# 3. Verify input size < 8KB
# 4. Check for malformed JSON/XML syntax

# If WiFi connection fails
# 1. Verify 2.4GHz network (not 5GHz)
# 2. Check credentials in code
# 3. Monitor serial output for connection status
# 4. Try wifi disconnect then wifi connect

# If memory issues occur
# 1. Reduce input data size
# 2. Check for memory leaks in custom code
# 3. Monitor heap usage with ESP.getFreeHeap()
```

### Git Troubleshooting
```bash
# Revert to last commit
git reset --hard HEAD

# Revert to specific commit
git reset --hard <commit-hash>

# See what changed in a commit
git show <commit-hash>

# Compare commits
git diff HEAD~1..HEAD
```

### Advanced Git (Optional)
```bash
# Create feature branch for experiments
git checkout -b feature/new-parser

# Switch back to main
git checkout main

# Merge feature branch
git merge feature/new-parser

# Compare two commits
git diff <commit1>..<commit2>
```

## 📁 Project Files

### Core Files
- `XIAO_Weather_Parser.ino` - Main Arduino sketch
- `driver.h` - Hardware and parsing configuration  
- `README.md` - Project documentation
- `DEVELOPMENT.md` - This complete development guide

### Key Functions in Sketch
- `parseWeatherJSON()` - JSON format parser
- `parseWeatherCSV()` - CSV format parser  
- `parseWeatherXML()` - XML format parser
- `fetchWeatherData()` - HTTP/HTTPS data fetcher
- `connectWiFi()` - WiFi connection handler
- `processSerialCommand()` - Command line interface

## 🎯 Development Notes

### Current Implementation Status
- [x] Project scaffold and documentation
- [ ] JSON weather data parsing
- [ ] CSV weather data parsing  
- [ ] XML weather data parsing
- [ ] WiFi HTTP client implementation
- [ ] Serial command interface
- [ ] Error handling and validation
- [ ] Memory optimization
- [ ] API integration examples

### Next Steps Ideas
- [ ] Add weather data logging to SD card
- [ ] Implement weather alerts/thresholds
- [ ] Create web dashboard interface
- [ ] Add multiple API source support
- [ ] Implement data filtering/averaging
- [ ] Add time series data collection
- [ ] Create weather prediction algorithms
- [ ] Add sensor fusion capabilities
- [ ] Implement OTA (Over-The-Air) updates
- [ ] Add MQTT weather data publishing

## 🧪 Testing Procedures

### Unit Testing Commands
```bash
# Test JSON parsing
parse json {"temperature":25.5,"humidity":60,"pressure":1013.25}

# Test CSV parsing
parse csv temp,humid,press\n25.5,60,1013.25

# Test XML parsing
parse xml <weather><temp>25.5</temp><humid>60</humid></weather>

# Test network connectivity
wifi connect
fetch https://httpbin.org/json

# Test error handling
parse json {invalid_json}
parse csv incomplete_csv_data
```

### Performance Benchmarking
```cpp
// Add to sketch for timing measurements
unsigned long startTime = millis();
// ... parsing operation ...
unsigned long parseTime = millis() - startTime;
Serial.printf("Parse time: %lu ms\n", parseTime);
```

## 📊 Memory Usage Guidelines

### Recommended Limits
- **JSON Input**: < 4KB for optimal performance
- **CSV Rows**: < 100 rows per parse operation
- **XML Depth**: < 8 nested levels
- **Concurrent Connections**: 1 HTTP request at a time
- **String Buffers**: Use String sparingly, prefer char arrays

### Memory Monitoring
```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Largest free block: %d bytes\n", ESP.getMaxAllocHeap());
```

## 🔄 Usage Notes

**For Warp Drive**: Save this file as "XIAO Weather Parser Development Guide"

This development guide contains everything needed to develop and extend the weather parser project:
- Complete command reference for copy/paste
- Hardware specifications and pin configurations  
- Network integration examples
- Comprehensive troubleshooting guide
- Development workflow and testing procedures
- Performance optimization guidelines

Perfect for rapid development and team collaboration! 🚀
