import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

# CONFIGURATION
# Get the directory where this script is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CSV_FILE = os.path.join(SCRIPT_DIR, "acceleration_test_v2_data.csv")

def calculate_acceleration_stats(df):
    """Calculate mean max acceleration for each acceleration parameter"""
    test_df = df[df['Phase'] == 'TESTING'].copy()
    
    # Group by acceleration parameter
    acceleration_params = test_df['Acceleration'].unique()
    
    stats = []
    
    for acc_param in sorted(acceleration_params):
        acc_tests = test_df[test_df['Acceleration'] == acc_param]
        
        # Get unique test numbers for this acceleration
        test_nums = acc_tests['TestNum'].unique()
        
        max_accelerations = []
        
        for test_num in test_nums:
            test_data = acc_tests[acc_tests['TestNum'] == test_num].sort_values('Time_ms')
            
            if len(test_data) < 2:
                continue
            
            # Calculate instantaneous acceleration (change in speed / change in time)
            time_diff = test_data['Time_ms'].diff() / 1000.0  # Convert to seconds
            speed_diff = test_data['Speed'].diff()
            
            # Acceleration = dv/dt
            instantaneous_acc = speed_diff / time_diff
            
            # Get max acceleration for this test
            max_acc = instantaneous_acc.max()
            if not np.isnan(max_acc) and not np.isinf(max_acc):
                max_accelerations.append(max_acc)
        
        if max_accelerations:
            mean_max_acc = np.mean(max_accelerations)
            std_max_acc = np.std(max_accelerations)
            stats.append({
                'AccelerationParam': acc_param,
                'MeanMaxAcceleration': mean_max_acc,
                'StdMaxAcceleration': std_max_acc,
                'NumTests': len(max_accelerations)
            })
    
    return pd.DataFrame(stats)

def plot_tests():
    # Read CSV file
    try:
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"Error: {CSV_FILE} not found. Run collect_data_v2.py first to collect data.")
        return
    
    print(f"Loaded {len(df)} data points")
    
    # Filter only TESTING phase data
    test_df = df[df['Phase'] == 'TESTING'].copy()
    
    if len(test_df) == 0:
        print("No testing data found in CSV file.")
        return
    
    # Get unique tests
    unique_tests = test_df['TestNum'].unique()
    print(f"Found {len(unique_tests)} tests")
    
    # Create plot
    fig, ax = plt.subplots(figsize=(16, 10))
    
    # Plot each test
    for test_num in sorted(unique_tests):
        test_data = test_df[test_df['TestNum'] == test_num]
        
        if len(test_data) == 0:
            continue
        
        target_speed = test_data['TargetSpeed'].iloc[0]
        acc = test_data['Acceleration'].iloc[0]
        
        # Convert time to seconds
        time_sec = test_data['Time_ms'] / 1000.0
        speed = test_data['Speed']
        
        label = f"Test {test_num}: speed={target_speed}, acc={acc}"
        ax.plot(time_sec, speed, label=label, alpha=0.6, linewidth=1.5)
    
    ax.set_xlabel('Time (s)', fontsize=12)
    ax.set_ylabel('Speed', fontsize=12)
    ax.set_title(f'Motor Acceleration Tests - All Tests Superposed ({len(unique_tests)} tests)', fontsize=14)
    ax.grid(True, alpha=0.3)
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=7, ncol=2)
    plt.tight_layout()
    
    # Save plot in the same directory as the script
    output_file = os.path.join(SCRIPT_DIR, 'acceleration_tests_all_superposed.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Plot saved as '{output_file}'")
    
    plt.show()

if __name__ == "__main__":
    print("Creating superposed plot of all acceleration tests...")
    
    # Read data
    try:
        df = pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        print(f"Error: {CSV_FILE} not found.")
        exit(1)
    
    # Calculate and display acceleration statistics
    print("\n" + "="*60)
    print("MEAN MAX ACCELERATION FOR EACH ACCELERATION PARAMETER")
    print("="*60)
    
    stats_df = calculate_acceleration_stats(df)
    
    if len(stats_df) > 0:
        print(stats_df.to_string(index=False))
        
        # Save stats to CSV
        stats_file = os.path.join(SCRIPT_DIR, 'acceleration_stats.csv')
        stats_df.to_csv(stats_file, index=False)
        print(f"\nStatistics saved to '{stats_file}'")
    else:
        print("No acceleration data found.")
    
    print("\n" + "="*60 + "\n")
    
    # Create plot
    plot_tests()
