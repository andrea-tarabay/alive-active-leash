#ifndef SETUP_HELPERS_H
#define SETUP_HELPERS_H

#include <Arduino.h>
#include "main.h"
#include "motor.h"

#define BASELINE_SAMPLES    40
#define LOAD_DELTA          10       // load spike threshold

// Initialize pins and basic serials (called from setup)
void setupPins();

// Interactive release of brakes: press button to toggle brakes motor
void releaseBrakesInteractive(Motor &motorBrakes);

// Autonomous calibration of brakes motor (existing logic)
void autoBrakesCalibration(Motor &motorSpool, Motor &motorBrakes);

// Manual calibration of spool motor via button: press to run at -2000, press again to stop and resetInitialRaw
void manualSpoolCalibration(Motor &motorSpool);

#endif
