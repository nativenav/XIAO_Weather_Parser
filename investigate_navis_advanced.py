#!/usr/bin/env python3
"""
Advanced investigation of Navis API endpoints to find averaged/peak data
"""
import requests
import json
import time
from datetime import datetime, timedelta

def investigate_navis_endpoints():
    """Investigate different Navis API endpoints and parameters"""
    print("🔍 Advanced Navis API Investigation...")
    
    # Establish session
    session = requests.Session()
    session_url = "https://www.navis-livedata.com/view.php?u=36371"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8',
        'Accept-Language': 'en-GB,en;q=0.9,fr;q=0.8',
        'Connection': 'keep-alive'
    }
    
    print(f"  🔄 Establishing session...")
    session_response = session.get(session_url, headers=headers, timeout=10)
    if session_response.status_code != 200:
        print(f"  ❌ Failed to establish session: {session_response.status_code}")
        return False
    
    print(f"  ✅ Session established")
    
    # Test different endpoint variations
    imei = "083af23b9b89_15_1"
    base_url = "https://www.navis-livedata.com/query.php"
    
    # Current time and recent past for 'from'/'to' parameters
    now = int(time.time())
    hour_ago = now - 3600
    day_ago = now - 86400
    
    endpoints_to_try = [
        # Basic live data (we know this works)
        f"{base_url}?imei={imei}&type=live",
        
        # Historical data endpoints with different time ranges
        f"{base_url}?imei={imei}&type=data",
        f"{base_url}?imei={imei}&type=data&from={hour_ago}&to={now}",
        f"{base_url}?imei={imei}&type=data&from={day_ago}&to={now}",
        f"{base_url}?imei={imei}&type=log",
        f"{base_url}?imei={imei}&type=log&from={hour_ago}&to={now}",
        
        # Try some variations
        f"{base_url}?imei={imei}&type=stats",
        f"{base_url}?imei={imei}&type=summary",
        f"{base_url}?imei={imei}&type=average",
        f"{base_url}?imei={imei}&type=peak",
        f"{base_url}?imei={imei}&type=chart",
        
        # Different time formats
        f"{base_url}?imei={imei}&type=data&from=0&to={now}",
        f"{base_url}?imei={imei}&type=data&range=hour",
        f"{base_url}?imei={imei}&type=data&range=day",
    ]
    
    api_headers = headers.copy()
    api_headers.update({
        'Accept': '*/*',
        'Referer': session_url,
        'Sec-Fetch-Dest': 'empty',
        'Sec-Fetch-Mode': 'cors',
        'Sec-Fetch-Site': 'same-origin'
    })
    
    successful_endpoints = []
    
    for i, endpoint in enumerate(endpoints_to_try, 1):
        print(f"\n📊 Testing endpoint {i}/{len(endpoints_to_try)}: {endpoint}")
        
        try:
            response = session.get(endpoint, headers=api_headers, timeout=10)
            print(f"    Status: {response.status_code}")
            
            if response.status_code == 200:
                content = response.text.strip()
                print(f"    Length: {len(content)} bytes")
                
                if content:
                    # Check if it looks like the live data format
                    if ':' in content and len(content.split(':')) == 3:
                        print(f"    Format: Colon-delimited (live data)")
                        print(f"    Content: {content}")
                    
                    # Check if it's JSON
                    elif content.startswith('{') or content.startswith('['):
                        print(f"    Format: JSON")
                        try:
                            data = json.loads(content)
                            print(f"    JSON keys: {list(data.keys()) if isinstance(data, dict) else f'Array with {len(data)} items'}")
                            if isinstance(data, list) and len(data) > 0:
                                print(f"    First item keys: {list(data[0].keys()) if isinstance(data[0], dict) else 'Not dict'}")
                        except json.JSONDecodeError:
                            print(f"    Invalid JSON")
                    
                    # Check for multi-line data (could be CSV or structured)
                    elif '\n' in content:
                        lines = content.split('\n')
                        print(f"    Format: Multi-line ({len(lines)} lines)")
                        print(f"    First few lines:")
                        for line in lines[:3]:
                            if line.strip():
                                print(f"      {line[:80]}...")
                    
                    # Single line, non-JSON
                    else:
                        print(f"    Format: Single line text")
                        print(f"    Content preview: {content[:100]}...")
                    
                    successful_endpoints.append((endpoint, content))
                else:
                    print(f"    ❌ Empty response")
            else:
                print(f"    ❌ HTTP Error: {response.status_code}")
                
        except Exception as e:
            print(f"    ❌ Exception: {e}")
        
        time.sleep(0.5)  # Be polite to the server
    
    print(f"\n📋 Summary: Found {len(successful_endpoints)} working endpoints")
    
    return successful_endpoints

if __name__ == "__main__":
    results = investigate_navis_endpoints()
    
    if results:
        print(f"\n🎉 Found {len(results)} endpoints with data!")
        for endpoint, content in results:
            print(f"\n✅ {endpoint}")
            if len(content) < 200:
                print(f"   Content: {content}")
            else:
                print(f"   Content preview: {content[:200]}...")
    else:
        print(f"\n😞 No additional endpoints found")
