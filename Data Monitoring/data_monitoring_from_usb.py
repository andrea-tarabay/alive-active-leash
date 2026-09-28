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
# SUBPLOTS
# -----------------------------------------------------------
def animate(i):
    plt.clf()

    # Create subplots (5 rows, 1 column)
    ax1 = plt.subplot(5, 1, 1)
    ax2 = plt.subplot(5, 1, 2)
    ax3 = plt.subplot(5, 1, 3)
    ax4 = plt.subplot(5, 1, 4)
    ax5 = plt.subplot(5, 1, 5)

    # SPEED subplot
    ax1.plot(plot_buffer["M1_SPEED"], label="M1_SPEED")
    ax1.plot(plot_buffer["M1_SPEED_CMD"], label="M1_SPEED_CMD")
    ax1.plot(plot_buffer["M2_SPEED"], label="M2_SPEED")
    ax1.set_title("Speed (rope, brakes)")
    ax1.set_ylim(-6000, 6000)
    ax1.legend()
    ax1.grid(True)

    # M1 POSITION subplot
    ax2.plot(plot_buffer["M1_POS"], label="M1_POS")
    ax2.plot(plot_buffer["POT"], label="POT")
    ax2.set_title("Position (rope, potentiometer)")
    ax2.set_ylim(-500, 100000)
    ax2.legend()
    ax2.grid(True)

    # M2 POSITION subplot
    ax3.plot(plot_buffer["M2_POS"], label="M2_POS")
    ax3.set_title("Position (brakes)")
    ax3.set_ylim(-1000, 10000)
    ax3.legend()
    ax3.grid(True)

    # LOAD subplot
    ax4.plot(plot_buffer["M1_TORQUE"], label="M1_TORQUE")
    ax4.plot(plot_buffer["M1_MODELED_TORQUE"], label="M1_MODELED_TORQUE")
    ax4.plot(plot_buffer["M1_RESIDUAL_LOAD"], label="M1_RESIDUAL_LOAD")
    ax4.set_title("Torque, Modeled & Residual Load M1")
    ax4.set_ylim(-200, 200)
    ax4.legend()
    ax4.grid(True)

    # STATE subplot
    ax5.plot(plot_buffer["STATE"], label="STATE")
    ax5.set_title("State")
    ax5.set_ylim(-1, 5)
    ax5.legend()
    ax5.grid(True)

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
