# XIAO Weather Station Data Parser Project

This project allows you to parse weather station data from various formats (JSON, XML, CSV) using your Seeed Studio XIAO ESP32C3. Connect to weather APIs, receive data via Serial, or parse local weather station files.

## Hardware Setup

### Required Hardware
- Seeed Studio XIAO ESP32C3
- USB-C cable for programming and power
- WiFi network for internet connectivity (optional)

### Connections
No additional hardware connections required for basic parsing functionality. The XIAO ESP32C3 handles all processing internally and communicates via Serial Monitor.

## Software Setup

### 1. Arduino IDE Configuration
1. Open Arduino IDE
2. Select Board: "XIAO_ESP32C3" from the ESP32 boards
3. Select the correct COM port for your XIAO

### 2. Library Dependencies
Install the following libraries from the Arduino Library Manager:
- **ArduinoJson** - For JSON parsing and generation
- **WiFiClientSecure** - For HTTPS weather API calls (built into ESP32 core)
- **HTTPClient** - For HTTP requests (built into ESP32 core)

### 3. Upload the Sketch
1. Open `XIAO_Weather_Parser.ino` in Arduino IDE
2. Make sure both `driver.h` and the `.ino` file are in the same folder
3. Update WiFi credentials in the code if using network features
4. Compile and upload to your XIAO ESP32C3

## Usage

### 1. Open Serial Monitor
- Set baud rate to 115200
- Set line ending to "Newline" or "Both NL & CR"

### 2. Commands
- `help` - Display available commands
- `status` - Show WiFi and system status
- `parse json <data>` - Parse JSON weather data
- `parse csv <data>` - Parse CSV weather data  
- `parse xml <data>` - Parse XML weather data
- `fetch <url>` - Fetch weather data from URL (requires WiFi)
- `test` - Run test parsing with sample data
- `wifi connect` - Connect to configured WiFi network
- `wifi disconnect` - Disconnect from WiFi
- `debug on/off` - Toggle debug output

### 3. Supported Data Formats

#### JSON Format
```json
{
  "temperature": 22.5,
  "humidity": 65,
  "pressure": 1013.25,
  "wind_speed": 12.3,
  "wind_direction": 180,
  "conditions": "partly cloudy"
}
```

#### CSV Format
```csv
timestamp,temperature,humidity,pressure,wind_speed,wind_direction,conditions
2024-08-28T12:00:00Z,22.5,65,1013.25,12.3,180,partly cloudy
```

#### XML Format
```xml
<weather>
  <temperature>22.5</temperature>
  <humidity>65</humidity>
  <pressure>1013.25</pressure>
  <wind_speed>12.3</wind_speed>
  <wind_direction>180</wind_direction>
  <conditions>partly cloudy</conditions>
</weather>
```

### 4. Features
- Parse multiple weather data formats
- WiFi connectivity for API calls
- Serial command interface
- Structured data output
- Error handling and validation
- Debug mode for troubleshooting
- Memory-efficient parsing
- Support for common weather station protocols

## Weather API Integration

### Popular Weather APIs Supported
- **OpenWeatherMap** - JSON format
- **WeatherAPI** - JSON format  
- **NOAA/NWS** - XML format
- **Custom weather stations** - Various formats

### Example API Usage
```
fetch https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_API_KEY
```

## Troubleshooting

### If parsing fails:
1. Check that input data matches expected format
2. Verify JSON/XML syntax is valid
3. Ensure CSV has proper headers
4. Check serial monitor for error messages
5. Enable debug mode with `debug on`

### If WiFi connection fails:
1. Verify WiFi credentials in code
2. Check network connectivity
3. Ensure 2.4GHz network (ESP32C3 doesn't support 5GHz)
4. Check serial monitor for connection status

### If compilation fails:
1. Make sure the `driver.h` file exists in your sketch folder
2. Verify ArduinoJson library is properly installed
3. Check that you've selected the correct board (XIAO ESP32C3)
4. Ensure ESP32 board package is up to date

## System Specifications
- **Microcontroller**: ESP32-C3 RISC-V @ 160MHz
- **Memory**: 400KB SRAM, 4MB Flash
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Supported Formats**: JSON, XML, CSV, custom delimited
- **Max Data Size**: 8KB per parsing operation
- **Network Protocols**: HTTP/HTTPS, TCP/UDP
