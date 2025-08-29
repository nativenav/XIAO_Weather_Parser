<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# bramble.md

## Bramble Bank Weather Data Source - Southampton VTS Integration

### Overview

**Bramble Bank, Solent** marine weather data accessed via Southampton VTS (ABP) AJAX endpoint. Provides real-time wind conditions from the Bramble Bank weather station in the Solent, ideal for marine and sailing applications.

### Data Source Details

- **Station Location**: Bramble Bank, Solent
- **Data Provider**: ABP Southampton VTS (Vessel Traffic Services)
- **Station Type**: Professional marine weather station
- **Data Format**: HTML table structure
- **Update Frequency**: Real-time (every few minutes)
- **Data Source**: Southampton VTS Bramblemet aggregated data


### Working Endpoint

```
https://www.southamptonvts.co.uk/BackgroundSite/Ajax/LoadXmlFileWithTransform?xmlFilePath=D%3A%5Cftp%5Csouthampton%5CBramble.xml&xslFilePath=D%3A%5Cwwwroot%5CCMS_Southampton%5Ccontent%5Cfiles%5Cassets%5CSotonSnapshotmetBramble.xsl&w=51
```


### ESP32 HTTP Configuration

```cpp
http.addHeader("User-Agent", "Mozilla/5.0 (compatible; WeatherStation/1.0)");
http.addHeader("Accept", "text/html,*/*");
http.addHeader("Referer", "https://www.southamptonvts.co.uk/Live_Information/Tides_and_Weather/");
```


### Data Structure

**HTML Table Format:**

```html
<th colspan="2">Bramble Bank - Bramblemet</th>
<td>Wind Speed</td><td>5.5 Knots</td>
<td>Max Gust</td><td>6.1 Knots</td>
<td>Wind Direction</td><td>144 Degree</td>
<td>Air Temp</td><td>19.9 C</td>
<td>Pressure</td><td>1017.6 mBar</td>
<td>Updated</td><td>18/08/2025 18:13:00</td>
```


### Available Weather Data

- **Wind Speed** (knots)
- **Wind Direction** (degrees)
- **Maximum Gust** (knots)
- **Air Temperature** (Celsius)
- **Barometric Pressure** (mBar)
- **Last Updated** timestamp
- **Tide Height** (meters, when available)
- **Visibility** (nautical miles, when available)


### Proven Test Results

- Wind Speed: 5.5 knots
- Wind Direction: 144°
- Max Gust: 6.1 knots
- Temperature: 19.9°C
- Pressure: 1017.6 mBar
- Updated: 18/08/2025 18:13:00


### HTML Parsing Function

```cpp
bool parseBrambleData(const String& html, WeatherData& data) {
  String windSpeedStr, windDirStr, gustStr, tempStr;
  
  // Extract wind speed: <td>Wind Speed</td><td>5.5 Knots</td>
  if (extractTableValue(html, "Wind Speed", windSpeedStr)) {
    data.windSpeed = extractNumber(windSpeedStr);
  }
  
  // Extract wind direction: <td>Wind Direction</td><td>144 Degree</td>
  if (extractTableValue(html, "Wind Direction", windDirStr)) {
    data.windDirection = extractNumber(windDirStr);
  }
  
  // Extract gust: <td>Max Gust</td><td>6.1 Knots</td>
  if (extractTableValue(html, "Max Gust", gustStr)) {
    data.gustSpeed = extractNumber(gustStr);
  }
  
  // Extract temperature: <td>Air Temp</td><td>19.9 C</td>
  if (extractTableValue(html, "Air Temp", tempStr)) {
    data.temperature = extractNumber(tempStr);
  }
  
  return (data.windSpeed >= 0 && data.windDirection >= 0);
}
```


### Table Value Extraction Utility

```cpp
bool extractTableValue(const String& html, const String& fieldName, String& result) {
  int fieldPos = html.indexOf("<td>" + fieldName + "</td>");
  if (fieldPos < 0) return false;
  
  int nextTdStart = html.indexOf("<td>", fieldPos + fieldName.length());
  if (nextTdStart < 0) return false;
  
  int valueStart = nextTdStart + 4;
  int valueEnd = html.indexOf("</td>", valueStart);
  if (valueEnd < 0) return false;
  
  result = html.substring(valueStart, valueEnd);
  result.trim();
  return result.length() > 0;
}
```


### Discovery Method

Found via browser Developer Tools by inspecting network requests on the Southampton VTS Tides \& Weather page. The AJAX endpoint was identified in the XHR requests that populate the weather data tables.

### Integration Notes

- Works reliably with ESP32C3 and HTTPS
- Professional marine weather data from ABP Southampton
- HTML table parsing required (more complex than JSON)
- Provides comprehensive marine conditions including pressure and tide data
- Ideal for sailing and marine applications in the Solent area
- Complements Seaview personal weather station data for comprehensive coverage


### Station Significance

Bramble Bank is a critical navigation hazard and weather reference point in the Solent, making this data source highly relevant for marine activities between Portsmouth and Southampton. The station provides professional-grade meteorological data used by commercial and recreational mariners.
<span style="display:none">[^1][^2][^3][^4][^5][^6][^7]</span>

<div style="text-align: center">⁂</div>

[^1]: readme.txt

[^2]: https://www.southamptonvts.co.uk/Yachting_Leisure/Navigational_Safety/

[^3]: https://www.southamptonvts.co.uk/Admin/content/files/Port Information/PUNG/Southampton Port Users Information and Navigation Guidelines.pdf

[^4]: https://gist.github.com/pmarreck/28b3049a1a70b8b4f2eaff4466d0c76a

[^5]: https://lists.llvm.org/pipermail/llvm-commits/Week-of-Mon-20040301/012699.html

[^6]: http://lib.mexmat.ru/abc.php?letter=b

[^7]: https://www.lingexp.uni-tuebingen.de/z2/Morphology/baroni.rows

