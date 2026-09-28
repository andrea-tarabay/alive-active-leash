#include "setup_helpers.h"

#define MEAN_LOAD 85 // mean load torque at speed 250 [unit: motor torque units]

void setupPins() {
    // NOTE: with INPUT_PULLUP
    // LOW  = pressed
    // HIGH = released
    pinMode(PIN_BUTTON, INPUT_PULLUP);  // internal pull-up ON
    pinMode(PIN_POT, INPUT);
}

void releaseBrakesInteractive(Motor &motorBrakes) {
    Serial.println("Press button to release brakes, press again to stop...");
    bool motorRunning = false;
    int lastButtonState = digitalRead(PIN_BUTTON);
    while (true) {
        int reading = digitalRead(PIN_BUTTON);
        // detect pressed (transition HIGH -> LOW)
        if (reading == LOW && lastButtonState == HIGH) {
            // simple debounce
            delay(50);
            if (digitalRead(PIN_BUTTON) == LOW) {
                motorRunning = !motorRunning;
                if (motorRunning) {
                    motorBrakes.setSpeed(-2000); // move backwards
                    Serial.println("Motor running (backwards)");
                } else {
                    motorBrakes.setSpeed(0); // stop
                    Serial.println("Motor stopped");
                    // exit after stopping
                    break;
                }
                // wait for button release to avoid multiple toggles
                while (digitalRead(PIN_BUTTON) == LOW) delay(10);
            }
        }
        lastButtonState = reading;
        delay(30);
    }
}

void autoBrakesCalibration(Motor &motorBrakes) {
    Serial.println("Autonomous calibration of brakes motor (M2 only)");
    // start the motor at speed 250
    motorBrakes.setSpeed(250);
    delay(5000); //Let the motor reach speed
    const int N = 20;              // size of rolling window
    int samples[N];                // store last N samples
    int count = 0;                 // number of samples collected so far
    long sum = 0; 
    // make the motor turn until the load difference is over the threshold
    while (true) {
        int load = abs(motorBrakes.getTorque());
        /*
        if (count < N) {
            // still filling buffer
            samples[count] = load;
            sum += load;
            count++;
        } else {
            // remove oldest and add newest
            sum -= samples[0];

            // shift all values left by one
            for (int i = 1; i < N; i++) {
                samples[i - 1] = samples[i];
            }

            // insert new value at the end
            samples[N - 1] = load;
            sum += load;
        }

        // compute rolling mean
        float rollingMean = sum / (float)min(count, N);
        */

        //int loadDiff = abs(load - rollingMean - MEAN_LOAD);
        int loadDiff = abs(load - MEAN_LOAD);
        Serial.print("Current load: "); Serial.print(load);
        Serial.print(" Current load diff: "); Serial.print(loadDiff);
        Serial.println();
        if (loadDiff >= LOAD_DELTA) {
            Serial.println("Load change detected on M2, brake engaged!");
            motorBrakes.setSpeed(0); // stop motor
            motorBrakes.setSpeed(-2000); // reverse brakes slightly to avoid over-tightening
            delay(2500);
            Serial.println("Brake adjusted, set new 0 position.");
            motorBrakes.setSpeed(0);
            motorBrakes.resetInitialRaw();
            break;
        }
        delay(20);
    }
    

}

void manualSpoolCalibration(Motor &motorSpool) {
    Serial.println("Press button to start spool calibration, press again to stop and reset...");
    int lastButtonState = digitalRead(PIN_BUTTON);
    bool spoolRunning = false;
    while (true) {
        int reading = digitalRead(PIN_BUTTON);
        // detect pressed (transition HIGH -> LOW)
        if (reading == LOW && lastButtonState == HIGH) {
            // debounce
            delay(50);
            if (digitalRead(PIN_BUTTON) == LOW) {
                spoolRunning = !spoolRunning;
                if (spoolRunning) {
                    motorSpool.setSpeed(-2000); // move spool backwards
                    Serial.println("Spool running (backwards)");
                } else {
                    motorSpool.setSpeed(0); // stop
                    motorSpool.resetInitialRaw();
                    Serial.println("Spool stopped and resetInitialRaw");
                    break;
                }
                // wait for button release to avoid multiple toggles
                while (digitalRead(PIN_BUTTON) == LOW) delay(10);
            }
        }
        lastButtonState = reading;
        delay(10);
    }
}
