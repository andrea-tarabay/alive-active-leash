import serial
import serial.tools.list_ports
import csv
import time

# CONFIGURATION
COM_PORT = "COM24"
CSV_FILE = "acceleration_test_data.csv"

def find_port():
    # Try configured port first
    try:
        port = serial.Serial(COM_PORT, 115200, timeout=1)
        print(f"Connected to {COM_PORT}")
        return port
    except:
        print(f"Could not connect to {COM_PORT}, searching...")
        
    # Auto-detect
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if 'n/a' not in p.description:
            try:
                port = serial.Serial(p.device, 115200, timeout=1)
                print(f"Connected to {p.device}")
                return port
            except:
                continue
    print("No USB port found.")
    return None

def main():
    print("Connecting to Arduino...")
    port = find_port()
    
    if not port:
        return
    
    print(f"Collecting data to {CSV_FILE}...")
    
    # Open CSV file
    with open(CSV_FILE, 'w', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)
        
        all_complete = False
        header_written = False
        
        try:
            while not all_complete:
                if port.in_waiting:
                    line = port.readline().decode('utf-8').strip()
                    
                    if line:
                        print(line)  # Print to console
                        
                        if line == "ALL_TESTS_COMPLETE":
                            all_complete = True
                            print("\nAll tests completed!")
                        elif ',' in line:
                            # Write CSV data
                            if not header_written and line.startswith("TestNum"):
                                # Write header
                                csv_writer.writerow(line.split(','))
                                header_written = True
                            elif not line.startswith("TestNum"):
                                # Write data
                                csv_writer.writerow(line.split(','))
                                csvfile.flush()  # Flush to disk immediately
                
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\nData collection interrupted by user.")
    
    port.close()
    print(f"\nData saved to {CSV_FILE}")

if __name__ == "__main__":
    main()
