<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# seaview.md

## Seaview Weather Data Source - Navis Live Data Integration

### Overview

**Seaview, Isle of Wight** weather station data accessed via Navis Live Data API endpoint. Provides real-time wind conditions from a Navis WSD Sensor 15 located in Seaview.

### Data Source Details

- **Station Location**: Seaview, Isle of Wight
- **Station Type**: Navis WSD Sensor 15 (Wind Speed/Direction)
- **IMEI**: `083af23b9b89_15_1`
- **Data Format**: Colon-delimited hex-encoded sensor data
- **Update Frequency**: Real-time (every few minutes)
- **Data Source**: Navis live telemetry system


### Working Endpoint

**Session URL (required first):**
```
https://www.navis-livedata.com/view.php?u=36371
```

**Live Data API URL:**
```
https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=live
```

**Historical Data API URL (Enhanced):**
```
https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=data&from={timestamp}&to={timestamp}
```

### ESP32 HTTP Configuration

```cpp
// First establish session
session.get("https://www.navis-livedata.com/view.php?u=36371");

// Then query API with session cookie
http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
http.addHeader("Accept", "*/*");
http.addHeader("Referer", "https://www.navis-livedata.com/view.php?u=36371");
http.addHeader("Sec-Fetch-Dest", "empty");
http.addHeader("Sec-Fetch-Mode", "cors");
http.addHeader("Sec-Fetch-Site", "same-origin");
```

### Data Structure

**Response Format (Colon-Delimited):**
```
272808525:0:23200377de0
```

**Structure:**
- `272808525` - Timestamp
- `0` - Status flag (0 = OK)
- `23200377de0` - Hex-encoded sensor data

**Hex Data Decoding:**
```cpp
// Split hex into MSB and LSB
MSB = int(hex_data[:-8], 16);     // All but last 8 chars
LSB = int(hex_data[-8:], 16);     // Last 8 chars

// Extract values using bit manipulation
temp_raw = MSB & 0x7FF;           // bits 0-10 of MSB
speed_raw = LSB >> 16;            // bits 16-31 of LSB  
direction_raw = (LSB >> 7) & 0x1FF; // bits 7-15 of LSB (9 bits)
rssi_raw = LSB & 0x7F;            // bits 0-6 of LSB

// Apply conversions
speed_ms = speed_raw / 10.0;       // Divide by 10 first
speed_knots = speed_ms * 1.94384449; // Convert m/s to knots
temp_celsius = (temp_raw - 400) / 10.0; // Temperature formula
```


### Available Wind Data

- **Average Wind Speed** (knots) - Calculated from recent historical data (1-minute window)
- **Peak Wind Speed** (knots) - Maximum reading from recent historical data (gust)
- **Wind Direction** (degrees) - Current direction
- **Air Temperature** (Celsius) - When sensor reading is valid
- **Signal Strength** (RSSI) - Sensor telemetry quality

**Enhanced Implementation:** Now uses Navis historical data API (`type=data`) to calculate proper averages and peaks from recent readings, providing true wind speed vs gust differentiation like the website displays.

### Proven Test Results

- Wind Speed: 10.7 knots (5.5 m/s)
- Wind Direction: 251°
- Temperature: 16.2°C
- Response Time: ~500ms (including session establishment)
- Parse Time: ~5ms (hex decoding)

### Hex Parsing Function

```cpp
bool parseNavisData(const String& response, WeatherData& data) {
  // Parse colon-delimited format: timestamp:status:hexdata
  int firstColon = response.indexOf(':');
  int secondColon = response.indexOf(':', firstColon + 1);
  
  if (firstColon < 0 || secondColon < 0) return false;
  
  String hexData = response.substring(secondColon + 1);
  if (hexData.length() < 8) return false;
  
  // Split hex into MSB and LSB
  uint32_t MSB = 0, LSB = 0;
  if (hexData.length() > 8) {
    String msbHex = hexData.substring(0, hexData.length() - 8);
    MSB = strtoul(msbHex.c_str(), NULL, 16);
  }
  String lsbHex = hexData.substring(hexData.length() - 8);
  LSB = strtoul(lsbHex.c_str(), NULL, 16);
  
  // Extract values using bit manipulation
  uint16_t temp_raw = MSB & 0x7FF;          // bits 0-10 of MSB
  uint16_t speed_raw = LSB >> 16;           // bits 16-31 of LSB
  uint16_t direction_raw = (LSB >> 7) & 0x1FF; // bits 7-15 of LSB
  uint8_t rssi_raw = LSB & 0x7F;            // bits 0-6 of LSB
  
  // Apply conversions
  float speed_ms = speed_raw / 10.0;
  data.windSpeed = speed_ms * 1.94384449;   // Convert to knots
  data.windDirection = direction_raw;
  data.temperature = (temp_raw - 400) / 10.0;
  
  return (data.windSpeed >= 0 && data.windDirection >= 0);
}
```

### Discovery Method

Found by analyzing the Navis Live Data website JavaScript and network requests. The hex decoding algorithm was reverse-engineered from the site's NetData2.js file.

### Integration Notes

- Requires session management (cookie-based authentication)
- Works reliably with ESP32C3 and HTTPS
- Provides real-time instantaneous readings (no averaging)
- Hex decoding is computationally lightweight
- Complements Brambles Bank and Lymington averaged data
- Most responsive of the three weather sources

