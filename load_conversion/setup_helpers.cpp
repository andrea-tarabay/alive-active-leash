#include "setup_helpers.h"

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
        delay(10);
    }
}

void autoBrakesCalibration(Motor &motorSpool, Motor &motorBrakes) {
    Serial.println("Autonomus calibration of brakes motor");
    // set the spool motor to a known speed
    motorSpool.setSpeed(1000);
    delay(1000); //Let the motor reach speed

    // calibrate load
    long sumLoad = 0;
    for (int i = 0; i < BASELINE_SAMPLES; ++i) {
        int L = motorSpool.getTorque();
        sumLoad += L;
        delay(10);
    }
    int baseline  = sumLoad / BASELINE_SAMPLES;
    int threshold = baseline - LOAD_DELTA;

    Serial.print("Baseline load: "); Serial.println(baseline);
    Serial.print("Threshold load: "); Serial.println(threshold);

    // Engage brakes until load on M1 exceeds threshold
    Serial.println("Engaging brakes...");
    motorBrakes.setSpeed(500); // engage brakes
    while (true) {
        int currentLoad = motorSpool.getTorque();
        //Serial.print("Current load: "); Serial.println(currentLoad);
        if (currentLoad <= threshold) {
            Serial.println("Increase in load detected, Brake engaged!");
            motorSpool.setSpeed(0);  // stop spool motor
            motorBrakes.setSpeed(-2000); // reverse brakes slightly to avoid over-tightening
            delay(2000);
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
