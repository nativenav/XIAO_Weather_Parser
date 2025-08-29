#!/usr/bin/env python3
"""
Test script for XIAO Weather Parser - Weather Station Endpoints
Tests all three weather station APIs to verify they're working correctly.
"""

import requests
import json
import time
from datetime import datetime

def test_brambles_weather():
    """Test Brambles Bank weather station (Southampton VTS)"""
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
            if "Bramble Bank" in content and "Wind Speed" in content:
                print(f"  ✅ Success: Received {len(content)} bytes")
                print(f"  📊 Content preview: {content[:100]}...")
                return True
            else:
                print(f"  ❌ Unexpected content format")
                return False
        else:
            print(f"  ❌ HTTP Error: {response.status_code}")
            return False
    except Exception as e:
        print(f"  ❌ Exception: {e}")
        return False

def test_seaview_weather():
    """Test Seaview weather station (WeatherLink)"""
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
                    print(f"  ✅ Success: Valid JSON response")
                    print(f"  📊 Found {len(data.get('currConditionValues', []))} current conditions")
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
    """Test Lymington weather station (WeatherFile.com)"""
    print("\n⚓ Testing Lymington Weather Station...")
    
    url = "https://weatherfile.com/V03/loc/GBR00001/latest.json"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (compatible; WeatherStation/1.0)',
        'Accept': 'application/json,*/*',
        'X-Requested-With': 'XMLHttpRequest',
        'Referer': 'https://weatherfile.com/location?loc_id=GBR00001&wt=KTS'
    }
    
    try:
        response = requests.get(url, headers=headers, timeout=10)
        if response.status_code == 200:
            try:
                data = response.json()
                if data.get("status") == "ok" and "data" in data:
                    weather_data = data["data"]
                    print(f"  ✅ Success: API returned 'ok' status")
                    print(f"  📊 Wind Speed: {weather_data.get('wsc', 'N/A')} knots")
                    print(f"  📊 Wind Direction: {weather_data.get('wdc', 'N/A')} degrees")
                    print(f"  📊 Location: {weather_data.get('loc_name', 'N/A')}")
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
