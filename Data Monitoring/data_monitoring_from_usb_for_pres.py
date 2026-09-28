import serial
import serial.tools.list_ports
import csv
import time
from datetime import datetime
import os
import threading
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# CSV File Path
CSV_FILE = 'motor_random_data.csv'

# Motor to position conversion constants
STEPS_PER_ROTATION = 4096  # 4096 steps = 360°
SPOOL_RADIUS_M = 0.035      # 3.5 cm in meters

# Global variable to control the reading loop
stop_reading = False

# Data buffers for plotting
plot_buffer = {
    "M1_TORQUE": [],
    "M1_MODELED_TORQUE": [],
    "M1_SPEED": [],
    "M1_SPEED_CMD": [],
    "M2_SPEED": [],
    "M1_POS": [],
    "M2_POS": [],
    "M1_RESIDUAL_LOAD": [],
    "POT": [],
    "STATE": []
}

max_points = 500   # number of points visible on graph


# -----------------------------------------------------------
# USB PORT DETECTION
# -----------------------------------------------------------
def recup_port_USB():
    try:
        mData = serial.Serial('COM24', 115200, timeout=1)
        print("Connected to COM24")
        return mData
    except:
        print("Could not connect to COM24")
        return None


# -----------------------------------------------------------
# PARSE SERIAL LINE (key: value format)
# -----------------------------------------------------------
def parse_key_value_line(line):
    """Convert: KEY: VALUE KEY: VALUE ... into a dict"""
    try:
        # Only process lines starting with M1_TORQUE
        if not line.startswith("M1_TORQUE"):
            return None
            
        parts = line.split()
        data = {}
        i = 0
        while i < len(parts) - 1:
            key = parts[i].rstrip(':')
            value = parts[i + 1]
            try:
                # Try to convert to float first (for M1_LOAD which may be float)
                data[key] = float(value)
            except:
                pass
            i += 2
        return data
    except:
        return None


# -----------------------------------------------------------
# CSV + DATA RECORDING
# -----------------------------------------------------------
def read_serial_data(port):
    global stop_reading

    print(f"Reading data from {port.name}...")

    # Open CSV file
    with open(CSV_FILE, 'a', newline='') as csvfile:
        csv_writer = csv.writer(csvfile)

        # Write header if file empty
        if os.stat(CSV_FILE).st_size == 0:
            csv_writer.writerow([
                "Timestamp",
                "M1_TORQUE", "M1_MODELED_TORQUE", "M1_SPEED", "M1_SPEED_CMD", "M1_POS", "M1_RESIDUAL_LOAD",
                "M2_SPEED", "M2_POS",
                "POT", "STATE"
            ])

        while not stop_reading:
            try:
                line = port.readline().decode('utf-8').strip()

                if line:
                    print(line)
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

                    data = parse_key_value_line(line)

                    if data:
                        # Append to CSV
                        csv_writer.writerow([
                            timestamp,
                            data.get("M1_TORQUE"),
                            data.get("M1_MODELED_TORQUE"),
                            data.get("M1_SPEED"),
                            data.get("M1_SPEED_CMD"),
                            data.get("M1_POS"),
                            data.get("M1_RESIDUAL_LOAD"),
                            data.get("M2_SPEED"),
                            data.get("M2_POS"),
                            data.get("POT"),
                            data.get("STATE")
                        ])

                        # Update live data buffer
                        update_plot_buffer(data)

            except Exception as e:
                print(f"Error reading serial data: {e}")
                break


# -----------------------------------------------------------
# PLOTTING BUFFER UPDATE
# -----------------------------------------------------------
def update_plot_buffer(data):
    for key in plot_buffer.keys():
        if key in data:
            plot_buffer[key].append(data[key])
            if len(plot_buffer[key]) > max_points:
                plot_buffer[key].pop(0)

# -----------------------------------------------------------
# CONVERSION FUNCTIONS
# -----------------------------------------------------------
def steps_to_meters(steps):
    """Convert motor steps to linear distance in meters
    4096 steps = 360° = 1 full rotation = 2*pi*r meters
    """
    import math
    rotations = steps / STEPS_PER_ROTATION
    distance_m = rotations * 2 * math.pi * SPOOL_RADIUS_M
    return distance_m

def get_state_name(state_value):
    """Convert state number to state name"""
    state_map = {
        0: "FREE",
        1: "FREE_REWIND",
        2: "BRAKING",
        3: "FORCE_REWIND"
    }
    return state_map.get(state_value, f"UNKNOWN ({state_value})")


# -----------------------------------------------------------
# SUBPLOTS
# -----------------------------------------------------------
def animate(i):
    plt.clf()

    # Create subplots (2 rows, 1 column)
    ax1 = plt.subplot(2, 1, 1)
    ax2 = plt.subplot(2, 1, 2)

    # M1 POSITION subplot (converted to meters)
    m1_pos_m = [steps_to_meters(pos) for pos in plot_buffer["M1_POS"]]
    pot_pos_m = [steps_to_meters(pos) for pos in plot_buffer["POT"]]
    
    ax1.plot(m1_pos_m, label="M1 Position (m)")
    ax1.plot(pot_pos_m, label="Potentiometer (m)")
    ax1.set_title("Rope Position (meters)")
    ax1.set_ylabel("Distance (m)")
    ax1.legend()
    ax1.grid(True)

    # STATE subplot with state names
    ax2.plot(plot_buffer["STATE"], label="STATE", color='green', linewidth=2)
    ax2.set_title("State")
    ax2.set_ylabel("State")
    ax2.set_ylim(-0.5, 3.5)
    
    # Add state names as y-tick labels
    ax2.set_yticks([0, 1, 2, 3])
    ax2.set_yticklabels(["FREE", "FREE_REWIND", "BRAKING", "FORCE_REWIND"])
    
    ax2.legend()
    ax2.grid(True)

    plt.tight_layout()


# -----------------------------------------------------------
# STOP LISTENER (ENTER KEY)
# -----------------------------------------------------------
def monitor_user_input():
    global stop_reading
    while not stop_reading:
        user_input = input("Press Enter to stop communication")
        if user_input == "":
            stop_reading = True
            break


# -----------------------------------------------------------
# MAIN
# -----------------------------------------------------------
def main():
    global stop_reading

    print("Searching for USB serial port...")
    port = recup_port_USB()

    if not port:
        return

    # Thread for user input
    input_thread = threading.Thread(target=monitor_user_input)
    input_thread.daemon = True
    input_thread.start()

    # Thread for serial reading
    reader_thread = threading.Thread(target=read_serial_data, args=(port,))
    reader_thread.daemon = True
    reader_thread.start()

    # Real-time subplot window
    ani = FuncAnimation(plt.gcf(), animate, interval=200)
    plt.show()

    stop_reading = True
    reader_thread.join()
    input_thread.join()

    port.close()
    print("Serial communication closed.")


if __name__ == "__main__":
    main()
