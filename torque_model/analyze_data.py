import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures
import matplotlib.pyplot as plt

# Read the CSV file and handle all available columns
try:
    # Read the raw CSV data as text
    with open('arduino_data_brakes_motor.csv', 'r') as file:
        lines = file.readlines()
    
    # Process each line
    processed_data = []
    for line in lines[1:]:  # Skip header
        values = line.strip().split(',')
        if values[0].strip() == 'Measurement complete.':
            continue
        try:
            speed = float(values[0])
            loads = [float(x) for x in values[1:] if x.strip()]  # Convert all non-empty values
            if loads:  # If we have any load values
                processed_data.append([speed] + loads)
        except (ValueError, IndexError):
            continue
    
    # Convert to DataFrame
    data = pd.DataFrame(processed_data)
    # Name the speed column
    data = data.rename(columns={0: 'Speed'})
    
    # Convert all other columns to numeric, replacing errors with NaN
    for col in data.columns:
        if col != 'Speed':
            data[col] = pd.to_numeric(data[col], errors='coerce')
    # Drop any rows where Speed is NaN (non-numeric)
    data = data.dropna(subset=['Speed'])
    
    # Calculate the mean load across all available load columns
    # Get all columns except 'Speed'
    load_columns = [col for col in data.columns if col != 'Speed']
    print(f"Found {len(load_columns)} load columns")
    
    # Calculate mean of all available values in each row (ignoring NaN)
    data['mean_load'] = data[load_columns].mean(axis=1)

    # Define the threshold for near-zero values
    NEAR_ZERO_THRESHOLD = 10  # Speed values between -10 and 10 will be excluded
    
    # Split data into positive and negative speeds
    negative_data = data[data['Speed'] < -NEAR_ZERO_THRESHOLD]
    positive_data = data[data['Speed'] > NEAR_ZERO_THRESHOLD]
    near_zero_data = data[(data['Speed'] >= -NEAR_ZERO_THRESHOLD) & (data['Speed'] <= NEAR_ZERO_THRESHOLD)]

    # Prepare data for linear regression
    X_neg = negative_data['Speed'].values.reshape(-1, 1)
    y_neg = negative_data['mean_load'].values
    X_pos = positive_data['Speed'].values.reshape(-1, 1)
    y_pos = positive_data['mean_load'].values

    # Create and fit separate linear regression models
    model_neg = LinearRegression()
    model_pos = LinearRegression()
    model_neg.fit(X_neg, y_neg)
    model_pos.fit(X_pos, y_pos)

    # Get the coefficients
    a_neg = model_neg.coef_[0]  # slope for negative speeds
    b_neg = model_neg.intercept_  # y-intercept for negative speeds
    a_pos = model_pos.coef_[0]  # slope for positive speeds
    b_pos = model_pos.intercept_  # y-intercept for positive speeds

    # Print the formulas
    print(f"\nLinear Regression Formulas:")
    print(f"For negative speeds (Speed < -{NEAR_ZERO_THRESHOLD}):")
    print(f"Load = {a_neg:.6f} * Speed + {b_neg:.6f}")
    print(f"R² Score: {model_neg.score(X_neg, y_neg):.4f}")
    
    print(f"\nFor positive speeds (Speed > {NEAR_ZERO_THRESHOLD}):")
    print(f"Load = {a_pos:.6f} * Speed + {b_pos:.6f}")
    print(f"R² Score: {model_pos.score(X_pos, y_pos):.4f}")

    # Create the full range plot
    plt.figure(figsize=(10, 6))
    plt.scatter(data['Speed'], data['mean_load'], color='blue', alpha=0.5, label='Measured Data')
    
    # Plot regression lines for full range
    speed_neg = np.linspace(X_neg.min(), -NEAR_ZERO_THRESHOLD, 100).reshape(-1, 1)
    speed_pos = np.linspace(NEAR_ZERO_THRESHOLD, X_pos.max(), 100).reshape(-1, 1)
    
    plt.plot(speed_neg, model_neg.predict(speed_neg), color='red', 
             label=f'Negative speeds\nLoad = {a_neg:.6f}*Speed + {b_neg:.6f}')
    plt.plot(speed_pos, model_pos.predict(speed_pos), color='green',
             label=f'Positive speeds\nLoad = {a_pos:.6f}*Speed + {b_pos:.6f}')

    plt.xlabel('Speed')
    plt.ylabel('Average Load')
    plt.title('Load vs Speed - Full Range')
    plt.legend()
    plt.grid(True)
    plt.savefig('regression_plot_full.png')
    plt.close()

    # Create the zoomed plot (-500 to 500)
    plt.figure(figsize=(10, 6))
    
    # Filter data for zoomed range
    mask = (data['Speed'] >= -500) & (data['Speed'] <= 500)
    zoomed_data = data[mask]
    
    # Plot zoomed data points
    plt.scatter(zoomed_data['Speed'], zoomed_data['mean_load'], 
               color='blue', alpha=0.5, label='Measured Data')
    
    # Plot regression lines for zoomed range
    speed_neg_zoom = np.linspace(-500, -NEAR_ZERO_THRESHOLD, 100).reshape(-1, 1)
    speed_pos_zoom = np.linspace(NEAR_ZERO_THRESHOLD, 500, 100).reshape(-1, 1)
    
    plt.plot(speed_neg_zoom, model_neg.predict(speed_neg_zoom), color='red', 
             label=f'Negative speeds\nLoad = {a_neg:.6f}*Speed + {b_neg:.6f}')
    plt.plot(speed_pos_zoom, model_pos.predict(speed_pos_zoom), color='green',
             label=f'Positive speeds\nLoad = {a_pos:.6f}*Speed + {b_pos:.6f}')

    # Highlight near-zero data points
    if len(near_zero_data) > 0:
        plt.scatter(near_zero_data['Speed'], near_zero_data['mean_load'], 
                   color='orange', alpha=0.5, label=f'Near-zero values (±{NEAR_ZERO_THRESHOLD})')

    plt.xlabel('Speed')
    plt.ylabel('Average Load')
    plt.title('Load vs Speed - Zoomed Range (-500 to 500)')
    plt.legend()
    plt.grid(True)
    plt.xlim(-500, 500)
    plt.savefig('regression_plot_zoomed.png')
    plt.close()

    print("\nPlots have been saved as:")
    print("1. regression_plot_full.png (full range)")
    print("2. regression_plot_zoomed.png (±500 range)")

    # Calculate and print some basic statistics
    print("\nBasic Statistics:")
    print(f"Number of total measurements: {len(data)}")
    print(f"Number of measurements (negative speeds < -{NEAR_ZERO_THRESHOLD}): {len(negative_data)}")
    print(f"Number of measurements (positive speeds > {NEAR_ZERO_THRESHOLD}): {len(positive_data)}")
    print(f"Number of measurements (near-zero speeds ±{NEAR_ZERO_THRESHOLD}): {len(near_zero_data)}")
    print(f"Speed range: {data['Speed'].min():.1f} to {data['Speed'].max():.1f}")
    print(f"Mean load range: {data['mean_load'].min():.1f} to {data['mean_load'].max():.1f}")
    
    if len(near_zero_data) > 0:
        print(f"\nNear-zero region statistics (speeds between -{NEAR_ZERO_THRESHOLD} and {NEAR_ZERO_THRESHOLD}):")
        print(f"Mean load: {near_zero_data['mean_load'].mean():.2f}")
        print(f"Min load: {near_zero_data['mean_load'].min():.2f}")
        print(f"Max load: {near_zero_data['mean_load'].max():.2f}")

    # ===== NEW ANALYSIS: Mean Load for Each Speed Value =====
    print("\n" + "="*60)
    print("MEAN LOAD FOR EACH SPEED VALUE")
    print("="*60)
    
    # Group by speed and calculate mean load
    speed_mean_load = data.groupby('Speed')['mean_load'].mean().reset_index()
    speed_mean_load.columns = ['Speed', 'MeanLoad']
    
    # Sort by speed for easier reading
    speed_mean_load = speed_mean_load.sort_values('Speed')
    
    print(f"\nTotal unique speed values: {len(speed_mean_load)}")
    print("\nSpeed | Mean Load")
    print("-" * 30)
    for _, row in speed_mean_load.iterrows():
        print(f"{row['Speed']:6.1f} | {row['MeanLoad']:8.2f}")
    
    # Save to CSV for easy use
    speed_mean_load.to_csv('speed_mean_load_table.csv', index=False)
    print(f"\nData saved to 'speed_mean_load_table.csv'")
    
    # Generate C++ lookup table code
    print("\n" + "="*60)
    print("C++ LOOKUP TABLE CODE FOR motor.cpp (NEW VERSION)")
    print("="*60)
    print("\nInstructions:")
    print("1. Copy the code below")
    print("2. Replace the computeTorque() function in motor.cpp")
    print("3. This new version uses the updated experimental data\n")
    print("-" * 60)
    
    print("\nfloat Motor::computeTorque(int speed) {")
    print("    // NEW Lookup table generated from latest experimental data")
    print("    // Speed range: -3000 to 3000 in steps of 50")
    print("    // Array index = (speed + 3000) / 50")
    print("    static const float loadTable[] = {")
    
    # Build the array entries
    entries = []
    for i, row in speed_mean_load.iterrows():
        speed_val = int(row['Speed'])
        load_val = row['MeanLoad']
        entries.append(f"        {load_val:8.2f}f    // {speed_val:5d}")
    
    # Print all entries with commas except the last one
    for i, entry in enumerate(entries):
        if i < len(entries) - 1:
            print(entry.replace("    //", ",   //"))
        else:
            print(entry + "     // Last entry")
    
    print("    };")
    print("")
    print("    // Clamp speed to valid range")
    print("    if (speed < -3000) speed = -3000;")
    print("    if (speed > 3000) speed = 3000;")
    print("")
    print("    // Calculate array index: index = (speed + 3000) / 50")
    print("    int index = (speed + 3000) / 50;")
    print("    ")
    print("    // Direct array lookup - O(1) constant time")
    print("    return loadTable[index];")
    print("}")
    print("\n" + "="*60)
    print("NEW VERSION - Copy the code above to replace computeTorque() in motor.cpp")
    print("="*60)

except Exception as e:
    print(f"An error occurred: {str(e)}")