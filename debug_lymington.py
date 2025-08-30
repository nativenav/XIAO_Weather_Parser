#!/usr/bin/env python3
"""
Debug script to inspect Lymington WeatherFile API response structure
"""
import requests
import json

def debug_lymington_api():
    """Debug the Lymington WeatherFile API to see all available parameters"""
    print("🔍 Debugging Lymington WeatherFile API...")
    
    url = "https://weatherfile.com/V03/loc/GBR00001/latest.json"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (compatible; WeatherStation/1.0)',
        'Accept': '*/*',
        'X-Requested-With': 'XMLHttpRequest',
        'Referer': 'https://weatherfile.com/location?loc_id=GBR00001&wt=KTS',
        'Origin': 'https://weatherfile.com',
        'Content-Length': '0',
        'wf-tkn': 'PUBLIC'
    }
    
    try:
        response = requests.post(url, headers=headers, data='', timeout=10)
        if response.status_code == 200:
            data = response.json()
            
            print(f"Status: {data.get('status')}")
            print(f"Raw response keys: {list(data.keys())}")
            
            if "data" in data:
                weather_data = data["data"]
                print(f"\nWeather data contains {len(weather_data)} parameters:")
                print("-" * 50)
                
                # Sort keys for easier reading
                sorted_keys = sorted(weather_data.keys())
                
                for key in sorted_keys:
                    value = weather_data[key]
                    print(f"  {key:<15}: {value} ({type(value).__name__})")
                
                # Look for potential gust/max parameters
                print(f"\n🔍 Checking for gust/max/peak parameters:")
                gust_related = [k for k in sorted_keys if any(term in k.lower() for term in ['max', 'gust', 'peak', 'high'])]
                if gust_related:
                    print(f"  Found gust-related parameters: {gust_related}")
                    for key in gust_related:
                        print(f"    {key}: {weather_data[key]}")
                else:
                    print(f"  ❌ No obvious gust/max/peak parameters found")
                
                # Check wind-related parameters specifically
                print(f"\n💨 All wind-related parameters:")
                wind_related = [k for k in sorted_keys if 'w' in k.lower() and ('c' in k.lower() or 'd' in k.lower() or 'speed' in k.lower() or 'dir' in k.lower())]
                for key in wind_related:
                    print(f"    {key}: {weather_data[key]} - {type(weather_data[key]).__name__}")
                    
            return True
        else:
            print(f"❌ HTTP Error: {response.status_code}")
            return False
            
    except Exception as e:
        print(f"❌ Exception: {e}")
        return False

if __name__ == "__main__":
    debug_lymington_api()
