import serial
import csv
from datetime import datetime

SERIAL_PORT = "COM24"
BAUD_RATE = 115200
CSV_FILE = f"arduino_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")

    with open(CSV_FILE, mode="w", newline='') as file:
        writer = csv.writer(file)
        print(f"Saving data to {CSV_FILE}")
        print("Press Ctrl+C to stop recording")

        try:
            while True:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    values = line.split(',')
                    writer.writerow(values)
                    file.flush()  # Ensure data is written immediately
                    print(line)
        except KeyboardInterrupt:
            print("\nRecording stopped by user")
        except Exception as e:
            print(f"\nError during recording: {e}")
        finally:
            print(f"\nData saved to {CSV_FILE}")

except serial.SerialException as e:
    print(f"Error opening serial port {SERIAL_PORT}: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")
