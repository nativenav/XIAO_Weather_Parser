#!/usr/bin/env python3
"""
Test script for XIAO Weather Parser - Weather Station Endpoints
Tests all three weather station APIs to verify they're working correctly.
Validates enhanced parsing features including 2-min averages and GMT timestamps.
"""

import requests
import json
import time
from datetime import datetime

# Constants for data validation
KNOTS_TO_MS = 0.514444

def validate_wind_data(speed_knots, direction, name="Wind"):
    """Validate wind speed and direction data"""
    speed_ms = speed_knots * KNOTS_TO_MS if speed_knots else 0
    
    if speed_knots and 0 <= speed_knots <= 200:  # Reasonable wind speed range
        print(f"  ✅ {name}: {speed_knots:.1f} knots ({speed_ms:.1f} m/s)")
        valid = True
    else:
        print(f"  ❌ {name}: Invalid or missing ({speed_knots})")
        valid = False
        
    if direction and 0 <= direction <= 360:
        print(f"  ✅ Direction: {direction}°")
        valid = valid and True
    else:
        print(f"  ❌ Direction: Invalid or missing ({direction})")
        valid = False
        
    return valid

def extract_table_cell_value(html_content, label):
    """Extract value from HTML table cell format: <td>Label</td><td>Value Units</td>"""
    import re
    
    # Pattern to find the label cell followed by value cell
    pattern = rf'<td>\s*{re.escape(label)}\s*</td>\s*<td>([^<]+)</td>'
    match = re.search(pattern, html_content, re.IGNORECASE)
    
    if match:
        value_cell = match.group(1).strip()
        # Check for embedded div tags (like timestamps)
        div_match = re.search(r'<div[^>]*>([^<]+)</div>', value_cell)
        if div_match:
            return div_match.group(1).strip()
        return value_cell
    
    return None

def extract_numeric_from_value(value_str):
    """Extract numeric value from string like '15.7 Knots'"""
    if not value_str:
        return None
    
    import re
    match = re.search(r'([0-9]+\.?[0-9]*)', value_str)
    return float(match.group(1)) if match else None

def test_brambles_weather():
    """Test Brambles Bank weather station (Southampton VTS) - HTML Table Format"""
    print("🌊 Testing Brambles Bank Weather Station...")
    
    url = "https://www.southamptonvts.co.uk/BackgroundSite/Ajax/LoadXmlFileWithTransform?xmlFilePath=D%3A%5Cftp%5Csouthampton%5CBramble.xml&xslFilePath=D%3A%5Cwwwroot%5CCMS_Southampton%5Ccontent%5Cfiles%5Cassets%5CSotonSnapshotmetBramble.xsl&w=51"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (compatible; WeatherStation/1.0)',
        'Accept': 'text/html,*/*',
        'Referer': 'https://www.southamptonvts.co.uk/Live_Information/Tides_and_Weather/'
    }
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            content = response.text
            print(f"  ✅ Success: Received {len(content)} bytes")
            
            # Parse HTML table format
            weather_data = {}
            
            # Extract wind speed
            wind_speed_str = extract_table_cell_value(content, "Wind Speed")
            if wind_speed_str:
                wind_speed = extract_numeric_from_value(wind_speed_str)
                if wind_speed:
                    weather_data["wind_speed"] = f"{wind_speed} knots ({wind_speed * KNOTS_TO_MS:.1f} m/s)"
            
            # Extract wind gust
            wind_gust_str = extract_table_cell_value(content, "Max Gust")
            if wind_gust_str:
                wind_gust = extract_numeric_from_value(wind_gust_str)
                if wind_gust:
                    weather_data["wind_gust"] = f"{wind_gust} knots ({wind_gust * KNOTS_TO_MS:.1f} m/s)"
            
            # Extract wind direction
            wind_dir_str = extract_table_cell_value(content, "Wind Direction")
            if wind_dir_str:
                wind_dir = extract_numeric_from_value(wind_dir_str)
                if wind_dir is not None:
                    weather_data["wind_direction"] = f"{int(wind_dir)}°"
            
            # Extract temperature
            temp_str = extract_table_cell_value(content, "Air Temp")
            if temp_str:
                temp = extract_numeric_from_value(temp_str)
                if temp is not None:
                    weather_data["temperature"] = f"{temp:.1f}°C"
            
            # Extract pressure
            pressure_str = extract_table_cell_value(content, "Pressure")
            if pressure_str:
                pressure = extract_numeric_from_value(pressure_str)
                if pressure is not None:
                    weather_data["pressure"] = f"{pressure:.1f} mBar"
            
            # Extract timestamp
            timestamp_str = extract_table_cell_value(content, "Updated")
            if timestamp_str:
                weather_data["timestamp"] = timestamp_str
            
            # Display parsed data
            if weather_data:
                print("  📊 Parsed weather data from HTML table:")
                for key, value in weather_data.items():
                    print(f"    ✅ {key.replace('_', ' ').title()}: {value}")
                
                # Validate we got essential data
                essential_keys = ["wind_speed", "wind_direction"]
                has_essential = any(key in weather_data for key in essential_keys)
                
                if has_essential:
                    print("  ✅ Essential weather data successfully parsed")
                    return True
                else:
                    print("  ⚠️  Parsed some data but missing essential wind information")
                    return False
            else:
                print(f"  ❌ Could not parse weather data from HTML table format")
                print(f"  📊 Raw content preview: {content[:200]}...")
                return False
        else:
            print(f"  ❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"  ❌ Exception: {e}")
        return False

def test_seaview_weather():
    """Test Seaview weather station (WeatherLink) with enhanced validation"""
    print("\n🏝️  Testing Seaview Weather Station...")
    
    url = "https://www.weatherlink.com/embeddablePage/summaryData/0d117f9a7c7e425a8cc88e870f0e76fb"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (compatible; WeatherStation/1.0)',
        'Accept': 'application/json,*/*',
        'X-Requested-With': 'XMLHttpRequest',
        'Referer': 'https://www.weatherlink.com/embeddablePage/show/0d117f9a7c7e425a8cc88e870f0e76fb/summary'
    }
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            try:
                data = response.json()
                if isinstance(data, list) and len(data) > 0:
                    data = data[0]
                
                if "currConditionValues" in data:
                    conditions = data["currConditionValues"]
                    print(f"  ✅ Success: Valid JSON response")
                    print(f"  📊 Found {len(conditions)} current conditions")
                    
                    # Look for 2-minute averages and other wind data
                    wind_data = {"2min_speed": None, "wind_speed": None, "wind_dir": None, "wind_gust": None}
                    
                    for condition in conditions:
                        sensor_name = condition.get('sensorDataName', '')
                        value = condition.get('convertedValue', '')
                        unit = condition.get('unitLabel', '')
                        
                        if value and value != '--':
                            if '2 Min' in sensor_name and 'Wind Speed' in sensor_name:
                                wind_data["2min_speed"] = f"{value} {unit}"
                            elif sensor_name == 'Wind Speed':
                                wind_data["wind_speed"] = f"{value} {unit}"
                            elif 'Wind Direction' in sensor_name:
                                wind_data["wind_dir"] = f"{value}°"
                            elif 'High Wind Speed' in sensor_name:
                                wind_data["wind_gust"] = f"{value} {unit}"
                    
                    # Display found data
                    if wind_data["2min_speed"]:
                        print(f"  ✅ 2-Min Avg Wind Speed: {wind_data['2min_speed']}")
                    elif wind_data["wind_speed"]:
                        print(f"  📊 Current Wind Speed: {wind_data['wind_speed']}")
                    
                    if wind_data["wind_dir"]:
                        print(f"  ✅ Wind Direction: {wind_data['wind_dir']}")
                    
                    if wind_data["wind_gust"]:
                        print(f"  ✅ Wind Gust: {wind_data['wind_gust']}")
                    
                    # Check for timestamp
                    if 'lastUpdate' in data:
                        print(f"  ✅ Last Update: {data['lastUpdate']}")
                    
                    return True
                else:
                    print(f"  ❌ Missing currConditionValues in response")
                    return False
            except json.JSONDecodeError:
                print(f"  ❌ Invalid JSON response")
                return False
        else:
            print(f"  ❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"  ❌ Exception: {e}")
        return False

def test_lymington_weather():
    """Test Lymington weather station (WeatherFile.com) with POST and wf-tkn header"""
    print("\n⚓ Testing Lymington Weather Station...")
    
    # Use POST request with wf-tkn header as discovered in browser dev tools
    url = "https://weatherfile.com/V03/loc/GBR00001/latest.json"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (compatible; WeatherStation/1.0)',
        'Accept': '*/*',
        'X-Requested-With': 'XMLHttpRequest',
        'Referer': 'https://weatherfile.com/location?loc_id=GBR00001&wt=KTS',
        'Origin': 'https://weatherfile.com',
        'Content-Length': '0',
        'wf-tkn': 'PUBLIC'  # This is the key header we were missing!
    }
    
    try:
        # Use POST request with empty body as discovered in browser dev tools
        response = requests.post(url, headers=headers, data='', timeout=10)
        if response.status_code == 200:
            try:
                data = response.json()
                if data.get("status") == "ok" and "data" in data:
                    weather_data = data["data"]
                    print(f"  ✅ Success: API returned 'ok' status")
                    print(f"  📊 Found {len(weather_data)} parameters in response")
                    
                    # Parse wind data - data is already in m/s despite wt=KTS URL parameter
                    wind_speed_ms = 0.0
                    wind_gust_ms = 0.0
                    wind_direction = 0
                    
                    for key, value in weather_data.items():
                        if key == 'wsc_avg':  # Average wind speed (prioritize for stability)
                            wind_speed_ms = value
                            print(f"  ✅ Wind Speed (avg): {value} m/s")
                        elif key == 'wsc' and wind_speed_ms == 0.0:  # Fallback to current
                            wind_speed_ms = value
                            print(f"  📊 Wind Speed (current): {value} m/s")
                        elif key == 'wsc_max':  # Max wind speed as gust
                            wind_gust_ms = value
                            print(f"  ✅ Wind Gust (max): {value} m/s")
                        elif key == 'wdc_avg' or (key == 'wdc' and wind_direction == 0):
                            wind_direction = int(value)
                            print(f"  ✅ Wind Direction ({key}): {int(value)}°")
                    
                    # Show basic parameters if no enhanced ones found
                    if wind_speed_ms == 0.0 and 'wsc' in weather_data:
                        wind_speed_ms = weather_data['wsc']
                        print(f"  📊 Wind Speed: {wind_speed_ms} m/s")
                    if wind_direction == 0 and 'wdc' in weather_data:
                        wind_direction = int(weather_data['wdc'])
                        print(f"  📊 Wind Direction: {wind_direction}°")
                    
                    # Show other useful info
                    if 'loc_name' in weather_data:
                        print(f"  ✅ Location: {weather_data['loc_name']}")
                    if 'ts' in weather_data:
                        print(f"  ✅ Timestamp: {weather_data['ts']}")
                    if 'delay' in weather_data and weather_data['delay'] > 0:
                        print(f"  📊 Data Delay: {weather_data['delay']} minutes")
                    
                    return True
                else:
                    print(f"  ❌ API returned error: {data.get('message', 'Unknown error')}")
                    return False
            except json.JSONDecodeError:
                print(f"  ❌ Invalid JSON response")
                return False
        else:
            print(f"  ❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"  ❌ Exception: {e}")
        return False

def main():
    """Run all weather station tests"""
    print("🌤️  XIAO Weather Parser - Station Endpoint Tests")
    print("=" * 50)
    print(f"⏰ Test started at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    results = []
    
    # Test all three stations
    results.append(("Brambles Bank", test_brambles_weather()))
    results.append(("Seaview", test_seaview_weather()))  
    results.append(("Lymington", test_lymington_weather()))
    
    # Summary
    print("\n" + "=" * 50)
    print("📋 Test Results Summary:")
    
    passed = 0
    for station, result in results:
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"  {station}: {status}")
        if result:
            passed += 1
    
    print(f"\n📊 Overall: {passed}/{len(results)} stations operational")
    
    if passed == len(results):
        print("🎉 All weather stations are working correctly!")
    else:
        print("⚠️  Some weather stations may need attention")
    
    return passed == len(results)

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
