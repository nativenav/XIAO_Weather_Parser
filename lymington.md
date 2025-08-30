# lymington.md

## Lymington Weather Data Source Investigation - WeatherFile.com

### Overview

**Location**: Lymington, Hampshire (GBR00001)
**Website**: https://weatherfile.com/location?loc_id=GBR00001&wt=KTS
**Status**: ✅ **WORKING** - API endpoint discovered and implemented

### Investigation Results - SOLVED! ✅

#### Working API Endpoints

**Primary Endpoint (Enhanced Data):**
- **API URL**: `https://weatherfile.com/V03/loc/GBR00001/infowindow.ggl`
- **Method**: POST request with specific headers
- **Response**: JSON with comprehensive averaged wind data
- **Data Format**: Wind average/high/low (wsa/wsh/wsl) in m/s, wind direction (wda) in degrees

**Basic Endpoint (Current Data Only):**
- **API URL**: `https://weatherfile.com/V03/loc/GBR00001/latest.json`
- **Method**: POST request with specific headers
- **Response**: JSON with instantaneous readings
- **Data Format**: Wind speed current (wsc) in m/s, wind direction (wdc) in degrees

#### Required Headers
```
User-Agent: Mozilla/5.0 (compatible; WeatherStation/1.0)
Accept: */*
X-Requested-With: XMLHttpRequest
Referer: https://weatherfile.com/location?loc_id=GBR00001&wt=KTS
Origin: https://weatherfile.com
Content-Length: 0
wf-tkn: PUBLIC
```

#### Enhanced Response Structure (infowindow.ggl)
```json
{
  "status": "ok",
  "data": {
    "loc_id": "GBR00001",
    "display_name": "Lymington Starting Platform",
    "lat": 50.74,
    "lng": -1.5071,
    "lastaverage": {
      "wsa": 2.32,      // Wind Speed Average (m/s)
      "wsh": 2.81,      // Wind Speed High/Max (m/s) - GUST DATA!
      "wsl": 1.97,      // Wind Speed Low (m/s)
      "wda": 283,       // Wind Direction Average (degrees)
      "ts": "2025-08-29 21:40:00",
      "date": "2025-08-29",
      "time": "21:40:00"
    }
  },
  "token": "PUBLIC"
}
```

#### Basic Response Structure (latest.json)
```json
{
  "status": "ok",
  "data": {
    "wdc": 262,        // Wind direction current (degrees)
    "wsc": 2.47,       // Wind speed current (m/s)
    "ts": "2025-08-29 21:41:00",
    "loc_name": "Lymington Starting Platform",
    "lat": 50.74,
    "lng": -1.5071,
    "delay": 7,        // Data delay (minutes)
    "num_params": 2
  }
}
```

### Implementation Details ✅

#### Arduino Functions Implemented
1. **`fetchLymingtonWeather()`**:
   - Makes HTTPS GET request to WeatherFile API
   - Sets required headers for authentication
   - Handles HTTP errors and timeouts
   - Measures fetch performance

2. **`parseLymingtonData()`**:
   - Parses JSON response using ArduinoJson library
   - Validates response status
   - Extracts wind speed and converts knots to m/s
   - Extracts wind direction, timestamp, location
   - Handles missing or invalid data gracefully

#### Serial Command
- **Command**: `lymington`
- **Usage**: Type `lymington` in serial monitor to fetch current data
- **Output**: Formatted weather display with fetch/parse timing

### Data Quality & Coverage

#### Available Parameters (Enhanced Endpoint)
- **Wind Speed Average**: Provided in m/s, converted to knots
- **Wind Speed High/Max**: Provided in m/s, converted to knots ✅ **GUST DATA**
- **Wind Speed Low**: Provided in m/s, converted to knots
- **Wind Direction Average**: Provided in degrees (0-360)
- **Timestamp**: Last update time from weather station
- **Location**: "Lymington Starting Platform" 
- **Coordinates**: Lat/Lng for reference
- **Data Quality**: Delay and parameter count indicators

#### Available Parameters (Basic Endpoint)
- **Wind Speed Current**: Provided in m/s (instantaneous reading)
- **Wind Direction Current**: Provided in degrees (0-360)
- **Timestamp**: Last update time from weather station
- **Location**: "Lymington Starting Platform"

#### Missing Parameters (Both Endpoints)
- Temperature: Not available in WeatherFile marine data
- Pressure: Not available in WeatherFile marine data  
- Humidity: Not available in WeatherFile marine data

*Note: This station focuses on marine wind data optimized for sailing/boating applications*

### Integration Status ✅

#### Complete Implementation
- **Function**: `fetchLymingtonWeather()` and `parseLymingtonData()`
- **Command**: `lymington` - available in serial interface
- **Help Text**: Updated to include Lymington station
- **Error Handling**: Robust error handling for network/parsing failures
- **Performance**: Fast JSON parsing with timing metrics

#### Usage Example (Enhanced Endpoint)
```
> lymington
[INFO] Fetching Lymington weather data...

=== LYMINGTON WEATHER STATION ===
Wind Speed: 5.4 knots (2.8 m/s)
Wind Gust: 7.5 knots (3.9 m/s)
Wind Direction: 278 degrees
Air Temperature: n/a
Air Pressure: n/a
Last Updated: 29/08/2025 21:45:00 GMT
Fetch Time: 250 ms, Parse Time: 3 ms
=================================
```

#### Integration with Other Stations
Now operational alongside:
- **Brambles Bank**: HTML parsing (full weather data)
- **Seaview, IoW**: JSON parsing (full weather data)  
- **Lymington**: JSON parsing (wind data focus)

### Next Steps

1. ✅ **Implementation Complete**
2. ✅ **Test all three stations - GUST DATA WORKING**
3. 🔄 **Update Arduino firmware**
4. ✅ **Documentation updated**

---

**Status**: ✅ **IMPLEMENTED AND OPERATIONAL WITH GUST DATA**
**Last Updated**: 29/08/2025
**Primary API Endpoint**: `https://weatherfile.com/V03/loc/GBR00001/infowindow.ggl` 
**Fallback API Endpoint**: `https://weatherfile.com/V03/loc/GBR00001/latest.json`
