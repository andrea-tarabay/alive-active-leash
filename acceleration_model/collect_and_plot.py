import serial
import csv
import time
from datetime import datetime
import matplotlib.pyplot as plt
import numpy as np
import os

# Configuration
PORT = 'COM24'
BAUDRATE = 115200
CSV_FILE = 'acceleration_data.csv'

def collect_data():
    """Collect data from serial port and save to CSV"""
    print(f"Connecting to {PORT}...")
    
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
        time.sleep(2)  # Wait for connection to stabilize
        
        print("Connected! Collecting data...")
        print("Press Ctrl+C to stop early\n")
        
        with open(CSV_FILE, 'w', newline='') as csvfile:
            csv_writer = csv.writer(csvfile)
            
            while True:
                try:
                    line = ser.readline().decode('utf-8').strip()
                    
                    if line:
                        print(line)  # Print to console
                        
                        # Write to CSV (including header)
                        if ',' in line:
                            csv_writer.writerow(line.split(','))
                            csvfile.flush()  # Immediate write to disk
                        
                        # Check if test is complete
                        if "ALL_TESTS_COMPLETE" in line:
                            print("\nAll tests complete!")
                            break
                            
                except UnicodeDecodeError:
                    continue
                    
        ser.close()
        print(f"\nData saved to {CSV_FILE}")
        return True
        
    except serial.SerialException as e:
        print(f"Error: Could not connect to {PORT}")
        print(f"Details: {e}")
        return False
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        ser.close()
        return True

def plot_data():
    """Read CSV and create plot"""
    if not os.path.exists(CSV_FILE):
        print(f"Error: {CSV_FILE} not found")
        return
    
    print(f"\nPlotting data from {CSV_FILE}...")
    
    # Dictionary to store data for each test
    tests_data = {}
    
    with open(CSV_FILE, 'r') as csvfile:
        csv_reader = csv.reader(csvfile)
        header = next(csv_reader)  # Skip header
        
        for row in csv_reader:
            if len(row) >= 3:
                try:
                    test_num = int(row[0])
                    time_val = float(row[1])
                    speed_val = float(row[2])
                    
                    if test_num not in tests_data:
                        tests_data[test_num] = {'time': [], 'speed': []}
                    
                    tests_data[test_num]['time'].append(time_val)
                    tests_data[test_num]['speed'].append(speed_val)
                except ValueError:
                    continue
    
    if not tests_data:
        print("No data to plot")
        return
    
    # Create figure with 3 subplots
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10))
    
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']
    
    # Process each test
    for test_num in sorted(tests_data.keys()):
        time_data = np.array(tests_data[test_num]['time'])
        speed_data = np.array(tests_data[test_num]['speed'])
        
        # Conversion: motor speed to m/s
        # Max motor speed: 4096 corresponds to 85 RPM
        # Spool radius: 0.04 m (4 cm)
        max_motor_speed = 4096  # motor units
        max_rpm = 85  # revolutions per minute
        spool_radius = 0.04  # meters
        
        # Convert motor speed to RPM
        rpm = (speed_data / max_motor_speed) * max_rpm
        
        # Convert RPM to linear speed (m/s)
        # v = ω * r, where ω = (RPM * 2π) / 60
        angular_velocity = (rpm * 2 * np.pi) / 60  # rad/s
        speed_m_s = angular_velocity * spool_radius  # m/s
        
        # Calculate acceleration (dv/dt)
        time_s = time_data / 1000.0
        acceleration = np.zeros(len(speed_m_s))
        
        for i in range(1, len(speed_m_s)):
            dt = time_s[i] - time_s[i-1]
            if dt > 0:
                acceleration[i] = (speed_m_s[i] - speed_m_s[i-1]) / dt
        
        color = colors[test_num % len(colors)]
        label = f'Test {test_num + 1}'
        
        # Plot 1: Raw speed (steps/s)
        ax1.plot(time_data, speed_data, color=color, linewidth=1.5, alpha=0.7, label=label)
        
        # Plot 2: Speed in m/s
        ax2.plot(time_data, speed_m_s, color=color, linewidth=1.5, alpha=0.7, label=label)
        
        # Plot 3: Acceleration
        ax3.plot(time_data, acceleration, color=color, linewidth=1.5, alpha=0.7, label=label)
    
    # Add reference lines to all plots
    for ax in [ax1, ax2, ax3]:
        ax.axvline(x=500, color='k', linestyle='--', linewidth=1, alpha=0.5, label='Motor ON')
        ax.axvline(x=5500, color='k', linestyle=':', linewidth=1, alpha=0.5, label='Motor OFF')
    
    # Configure Plot 1: Raw speed
    ax1.set_xlabel('Time (ms)', fontsize=11)
    ax1.set_ylabel('Speed (steps/s)', fontsize=11)
    ax1.set_title('Motor Speed (Raw) - All Tests Superposed', fontsize=12, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    
    # Configure Plot 2: Speed in m/s
    ax2.set_xlabel('Time (ms)', fontsize=11)
    ax2.set_ylabel('Speed (m/s)', fontsize=11)
    ax2.set_title('Motor Speed (Converted) - All Tests Superposed', fontsize=12, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    
    # Configure Plot 3: Acceleration
    ax3.axhline(y=0, color='k', linestyle='-', linewidth=0.5, alpha=0.3)
    ax3.set_xlabel('Time (ms)', fontsize=11)
    ax3.set_ylabel('Acceleration (m/s²)', fontsize=11)
    ax3.set_title('Motor Acceleration - All Tests Superposed', fontsize=12, fontweight='bold')
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    
    plt.tight_layout()
    
    # Save plot
    plot_file = 'acceleration_plot.png'
    plt.savefig(plot_file, dpi=300)
    print(f"Plot saved to {plot_file}")
    
    # Print statistics for each test
    print(f"\n=== Statistics ===")
    for test_num in sorted(tests_data.keys()):
        speed_data = np.array(tests_data[test_num]['speed'])
        time_data = np.array(tests_data[test_num]['time'])
        
        # Conversion parameters
        max_motor_speed = 4096
        max_rpm = 85
        spool_radius = 0.04
        
        # Convert to m/s
        rpm = (speed_data / max_motor_speed) * max_rpm
        angular_velocity = (rpm * 2 * np.pi) / 60
        speed_m_s = angular_velocity * spool_radius
        
        time_s = time_data / 1000.0
        acceleration = np.zeros(len(speed_m_s))
        for i in range(1, len(speed_m_s)):
            dt = time_s[i] - time_s[i-1]
            if dt > 0:
                acceleration[i] = (speed_m_s[i] - speed_m_s[i-1]) / dt
        
        print(f"\nTest {test_num + 1}:")
        print(f"  Max speed: {np.max(speed_m_s):.3f} m/s")
        print(f"  Max acceleration: {np.max(acceleration):.3f} m/s²")
        print(f"  Min acceleration: {np.min(acceleration):.3f} m/s²")
    
    # Show plot
    plt.show()

if __name__ == "__main__":
    print("=== Motor Acceleration Data Collection ===\n")
    
    # Collect data
    if collect_data():
        # Plot data
        plot_data()
    else:
        print("\nData collection failed. Cannot plot.")
