#!/usr/bin/env python3
"""
Parse Navis historical data to calculate averages and peaks
"""
import requests
import time
from datetime import datetime
import statistics

def decode_navis_hex(hex_data):
    """Decode Navis hex data using the same algorithm as before"""
    if len(hex_data) < 8:
        return None
        
    try:
        # Split hex into MSB and LSB
        MSB = 0
        if len(hex_data) > 8:
            MSB = int(hex_data[:-8], 16)  # All but last 8 chars
        LSB = int(hex_data[-8:], 16)      # Last 8 chars
        
        # Extract values using JavaScript bit manipulation
        temp_raw = MSB & 0x7FF                  # bits 0-10 of MSB
        speed_raw = LSB >> 16                   # bits 16-31 of LSB  
        direction_raw = (LSB >> 7) & 0x1FF     # bits 7-15 of LSB (9 bits)
        rssi_raw = LSB & 0x7F                   # bits 0-6 of LSB
        
        # Apply JavaScript recalc() conversions
        speed_ms = speed_raw / 10.0             # Divide by 10 first
        speed_knots = speed_ms * 1.94384449     # Convert m/s to knots
        temp_celsius = (temp_raw - 400) / 10.0  # Temperature formula
        
        return {
            'speed_knots': speed_knots,
            'speed_ms': speed_ms,
            'direction': direction_raw,
            'temperature': temp_celsius,
            'rssi': rssi_raw
        }
    except Exception as e:
        return None

def fetch_navis_historical_data(minutes_back=60):
    """Fetch historical data from Navis API"""
    print(f"🔍 Fetching Navis historical data (last {minutes_back} minutes)...")
    
    # Establish session
    session = requests.Session()
    session_url = "https://www.navis-livedata.com/view.php?u=36371"
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8',
        'Accept-Language': 'en-GB,en;q=0.9,fr;q=0.8',
        'Connection': 'keep-alive'
    }
    
    session_response = session.get(session_url, headers=headers, timeout=10)
    if session_response.status_code != 200:
        print(f"  ❌ Failed to establish session: {session_response.status_code}")
        return None
    
    # Calculate time range
    now = int(time.time())
    from_time = now - (minutes_back * 60)
    
    # Get historical data
    api_url = f"https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=data&from={from_time}&to={now}"
    
    api_headers = headers.copy()
    api_headers.update({
        'Accept': '*/*',
        'Referer': session_url,
        'Sec-Fetch-Dest': 'empty',
        'Sec-Fetch-Mode': 'cors',
        'Sec-Fetch-Site': 'same-origin'
    })
    
    print(f"  📊 Requesting data from {datetime.fromtimestamp(from_time)} to {datetime.fromtimestamp(now)}")
    
    response = session.get(api_url, headers=api_headers, timeout=15)
    if response.status_code != 200:
        print(f"  ❌ API Error: {response.status_code}")
        return None
    
    content = response.text.strip()
    if not content or content == "error":
        print(f"  ❌ No data or error response")
        return None
    
    print(f"  ✅ Received {len(content)} bytes of historical data")
    
    return content

def parse_navis_historical_data(raw_data):
    """Parse the pipe-delimited historical data format"""
    print(f"  🔄 Parsing historical data...")
    
    # Format appears to be: timestamp,interval:hexdata|interval:hexdata|...
    data_points = []
    
    # Split by pipe to get individual readings
    readings = raw_data.split('|')
    print(f"  📊 Found {len(readings)} data points")
    
    for reading in readings:
        if not reading.strip():
            continue
            
        # Each reading format: "interval:hexdata" or "timestamp,interval:hexdata"
        if ':' in reading:
            if ',' in reading:
                # Format: timestamp,interval:hexdata
                timestamp_part, hex_part = reading.split(',', 1)
                interval, hex_data = hex_part.split(':', 1)
            else:
                # Format: interval:hexdata
                interval, hex_data = reading.split(':', 1)
            
            # Decode the hex data
            decoded = decode_navis_hex(hex_data)
            if decoded:
                data_points.append(decoded)
    
    print(f"  ✅ Successfully parsed {len(data_points)} valid data points")
    return data_points

def calculate_statistics(data_points):
    """Calculate average and peak values from data points"""
    if not data_points:
        return None
    
    print(f"  🧮 Calculating statistics from {len(data_points)} data points...")
    
    # Extract just the speed values for statistics
    speeds_knots = [dp['speed_knots'] for dp in data_points if dp['speed_knots'] >= 0]
    speeds_ms = [dp['speed_ms'] for dp in data_points if dp['speed_ms'] >= 0]
    directions = [dp['direction'] for dp in data_points if 0 <= dp['direction'] <= 360]
    temps = [dp['temperature'] for dp in data_points if -20 <= dp['temperature'] <= 50]
    
    if not speeds_knots:
        print(f"  ❌ No valid speed data found")
        return None
    
    # Calculate statistics
    stats = {
        'avg_speed_knots': statistics.mean(speeds_knots),
        'avg_speed_ms': statistics.mean(speeds_ms),
        'peak_speed_knots': max(speeds_knots),
        'peak_speed_ms': max(speeds_ms),
        'min_speed_knots': min(speeds_knots),
        'min_speed_ms': min(speeds_ms),
        'data_points': len(speeds_knots),
    }
    
    if directions:
        # Circular mean for wind direction is complex, use simple mean for now
        stats['avg_direction'] = statistics.mean(directions)
    
    if temps:
        stats['avg_temperature'] = statistics.mean(temps)
    
    return stats

def main():
    """Main function to demonstrate Navis historical data analysis"""
    print("🌊 Navis Historical Data Analysis")
    print("=" * 50)
    
    # Fetch recent historical data
    raw_data = fetch_navis_historical_data(minutes_back=60)  # Last hour
    
    if not raw_data:
        print("❌ Failed to fetch historical data")
        return
    
    # Parse the data
    data_points = parse_navis_historical_data(raw_data)
    
    if not data_points:
        print("❌ Failed to parse historical data")
        return
    
    # Calculate statistics
    stats = calculate_statistics(data_points)
    
    if not stats:
        print("❌ Failed to calculate statistics")
        return
    
    # Display results
    print(f"\\n🎉 SEAVIEW WEATHER STATISTICS (Last 60 minutes)")
    print(f"=" * 50)
    print(f"Data Points: {stats['data_points']}")
    print(f"Average Wind Speed: {stats['avg_speed_knots']:.1f} knots ({stats['avg_speed_ms']:.1f} m/s)")
    print(f"Peak Wind Speed: {stats['peak_speed_knots']:.1f} knots ({stats['peak_speed_ms']:.1f} m/s)")
    print(f"Min Wind Speed: {stats['min_speed_knots']:.1f} knots ({stats['min_speed_ms']:.1f} m/s)")
    
    if 'avg_direction' in stats:
        print(f"Average Direction: {stats['avg_direction']:.0f}°")
    
    if 'avg_temperature' in stats:
        print(f"Average Temperature: {stats['avg_temperature']:.1f}°C")
    
    print(f"\\n🔍 Now compare with live instantaneous reading...")
    
    # Also get current live data for comparison
    session = requests.Session()
    session_url = "https://www.navis-livedata.com/view.php?u=36371"
    headers = {
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8',
    }
    
    session.get(session_url, headers=headers)
    
    live_response = session.get("https://www.navis-livedata.com/query.php?imei=083af23b9b89_15_1&type=live", 
                               headers={'Accept': '*/*', 'Referer': session_url})
    
    if live_response.status_code == 200:
        live_content = live_response.text.strip()
        if ':' in live_content:
            parts = live_content.split(':')
            if len(parts) >= 3:
                live_decoded = decode_navis_hex(parts[2])
                if live_decoded:
                    print(f"\\nCurrent Live Reading:")
                    print(f"Instantaneous Speed: {live_decoded['speed_knots']:.1f} knots ({live_decoded['speed_ms']:.1f} m/s)")
                    print(f"Direction: {live_decoded['direction']:.0f}°")
                    if -20 <= live_decoded['temperature'] <= 50:
                        print(f"Temperature: {live_decoded['temperature']:.1f}°C")

if __name__ == "__main__":
    main()
