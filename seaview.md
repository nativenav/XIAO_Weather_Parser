<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# seaview.md

## Seaview Weather Data Source - WeatherLink Integration

### Overview

**Seaview, Isle of Wight** weather station data accessed via WeatherLink's summaryData JSON API endpoint. Provides real-time wind conditions from a personal weather station located in Seaview.

### Data Source Details

- **Station Location**: Seaview, Isle of Wight
- **Station ID**: `0d117f9a7c7e425a8cc88e870f0e76fb`
- **Data Format**: JSON (structured sensor data)
- **Update Frequency**: Real-time (every few minutes)
- **Data Source**: WeatherLink personal weather station


### Working Endpoint

```
https://www.weatherlink.com/embeddablePage/summaryData/0d117f9a7c7e425a8cc88e870f0e76fb
```


### ESP32 HTTP Configuration

```cpp
http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
http.addHeader("Accept", "application/json,*/*");
http.addHeader("X-Requested-With", "XMLHttpRequest");
http.addHeader("Referer", "https://www.weatherlink.com/embeddablePage/show/0d117f9a7c7e425a8cc88e870f0e76fb/summary");
```


### Data Structure

**Current Conditions Array:**

```json
"currConditionValues": [
  {
    "sensorDataName": "Wind Speed",
    "convertedValue": "6",
    "unitLabel": "knots"
  },
  {
    "sensorDataName": "Wind Direction", 
    "convertedValue": "54",
    "unitLabel": "°"
  },
  {
    "sensorDataName": "10 Min High Wind Speed",
    "convertedValue": "8", 
    "unitLabel": "knots"
  }
]
```

**Daily Extremes Array:**

```json
"highLowValues": [
  {
    "sensorDataName": "High Wind Speed",
    "convertedValue": "20",
    "unitLabel": "knots"
  }
]
```


### Available Wind Data

- **Current Wind Speed** (knots)
- **Wind Direction** (degrees)
- **2 Min Average Wind Speed** (knots)
- **10 Min Average Wind Speed** (knots)
- **10 Min High Wind Speed** (gust data in knots)
- **Daily High Wind Speed** (knots)
- **Temperature** (when available)


### Proven Test Results

- Wind Speed: 6 knots
- Wind Direction: 54°
- 10 Min High (Gust): 8 knots
- Daily High: 20 knots
- Multiple time averages available


### JSON Parsing Function

```cpp
bool parseWeatherLinkData(const String& json, WeatherData& data) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  
  JsonObject obj = doc.is<JsonArray>() ? doc[0] : doc.as<JsonObject>();
  JsonArray conditions = obj["currConditionValues"];
  
  for (JsonObject condition : conditions) {
    String dataName = condition["sensorDataName"] | "";
    String value = condition["convertedValue"] | "";
    
    if (value == "--" || value == "") continue;
    
    if (dataName == "Wind Speed") {
      data.windSpeed = value.toFloat();
    } else if (dataName == "Wind Direction") {
      data.windDirection = value.toInt();
    } else if (dataName.indexOf("High Wind Speed") >= 0) {
      data.gustSpeed = max(data.gustSpeed, value.toFloat());
    }
  }
  
  return (data.windSpeed >= 0 && data.windDirection >= 0);
}
```


### Discovery Method

Found via browser Developer Tools by inspecting network requests on the WeatherLink embeddable page for this specific weather station.

### Integration Notes

- Works reliably with ESP32C3 and HTTPS
- Provides comprehensive wind data including averages and extremes
- Complements Bramble Bank marine data for dual-station weather dashboard
- JSON format easier to parse than HTML tables

