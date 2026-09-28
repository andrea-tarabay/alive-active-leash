#include "main.h"
#include "motor.h"
#include "setup_helpers.h"
#include "PIDController.h"

// =====================================
// KY-040 Rotary Encoder + Brakes PID Test
// Arduino Mega
// =====================================

// ------- Encoder pins -------
#define ENCODER_CLK   3    // interrupt-capable pin on Mega
#define ENCODER_DT    5
#define ENCODER_SW    6    // optional button (not used here)

volatile long encoderTicks = 0;   // global encoder counter
volatile int  lastClkState = HIGH;

// ------- Motors & PID -------
SMS_STS st;
Motor motorBrakes(ID_MOTOR_BRAKES, &st);
PIDController brakesPID;

// ========== ENCODER ISR ==========
void encoderISR() {
    int clkState = digitalRead(ENCODER_CLK);
    int dtState  = digitalRead(ENCODER_DT);

    if (clkState != lastClkState) {
        if (dtState != clkState) {
            encoderTicks++;   // one direction (e.g. leash going out)
        } else {
            encoderTicks--;   // opposite direction (e.g. leash going in)
        }
        lastClkState = clkState;
    }
}

// ========== SETUP ==========
void setup() {


    // // Debug serial
    Serial.begin(115200);

    // // Servo bus
    Serial1.begin(1000000, SERIAL_8N1);
    st.pSerial = &Serial1;

    // initialize pins
    setupPins();
        // set motors mode and start with "disconnected" motors
    //motorSpool.resetInitialRaw();
    //motorSpool.setMode(MODE_SPEED);
    motorBrakes.setMode(MODE_SPEED);
    //Turn on the motors at 0 speed
    motorBrakes.setSpeed(0);
    //motorSpool.setSpeed(0);
    //motorSpool.enableTorque(true);
    motorBrakes.enableTorque(true);
    delay(50);

    // Interactive release of brakes (press to toggle)
    releaseBrakesInteractive(motorBrakes);
    // Autonomous brakes calibration
    autoBrakesCalibration(motorBrakes);
    Serial.println("Calibration done – you can reset or flash main code.");


    // ---------- BRAKES PID CONFIG ----------
    // error = speedTicks (we want 0)  ->  output = brake speed command
    PIDConfig cfg;
    cfg.Kp = BRAKES_PID_KP;     // tune these!
    cfg.Ki = BRAKES_PID_KI;
    cfg.Kd = BRAKES_PID_KD;
    cfg.dt     = CONTROL_DT;    // must match loop timing in seconds
    cfg.outMin = -MOTOR_MAX_SPEED;   // allow opening (negative) and closing (positive)
    cfg.outMax =  MOTOR_MAX_SPEED;
    brakesPID.setConfig(cfg);

    // ---------- PINS & MOTORS ----------
    setupPins();    // your existing helper to init pins

    motorBrakes.setMode(MODE_SPEED);
    motorBrakes.setSpeed(0);
    motorBrakes.enableTorque(true);
    delay(50);

    // ---------- ENCODER SETUP ----------
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT,  INPUT_PULLUP);
    pinMode(ENCODER_SW,  INPUT_PULLUP);  // not used here

    lastClkState = digitalRead(ENCODER_CLK);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);

    // Start from zero ticks
    noInterrupts();
    encoderTicks = 0;
    interrupts();

    Serial.println("=== KY-040 + Brakes PID test (no spool motor) ===");
}

// ========== LOOP ==========
//
// Behavior:
//  - If encoder ticks INCREASE (dTicks > 0):
//        -> leash going out, dog pulling
//        -> use PID to CLOSE brakes (positive speed)
//  - If encoder ticks STOP increasing or go negative (dTicks <= 0):
//        -> leash not pulling / coming back
//        -> use PID to OPEN brakes back (negative speed)
//
void loop() {
    // 1) Read encoder ticks safely and compute delta
    static long lastEncTicks = 0;

    long encTicks;
    noInterrupts();
    encTicks = encoderTicks;
    interrupts();

    long dTicks = encTicks - lastEncTicks;
    lastEncTicks = encTicks;

    // 2) Estimate speed in ticks per second (assume fixed loop time = CONTROL_DT)
    float speedTicks = dTicks / CONTROL_DT;   // ticks / second

    // 3) Decide behavior based on sign of dTicks
    float error;
    int   brakeCmd;

    // Threshold: ignore tiny jitter/noise
    const long dTicksThreshold = 0;  // you can set e.g. 1 if needed

    if (dTicks > dTicksThreshold) {
        // -----------------------------
        // LEASH GOING OUT (pulling)
        // -> close brakes using PID
        // -----------------------------
        // We want speed -> 0 => error = measured speed
        error    = speedTicks;               // target is 0
        brakeCmd = (int)brakesPID.update(error);

    

        motorBrakes.setSpeed(brakeCmd);
    } else {
        // ----------------------------------------
        // NOT INCREASING (stopped or going back)
        // -> open brakes back
        // ----------------------------------------
        // Reset PID to avoid integrator wind-up
        brakesPID.reset();

        // Simple "go back" logic: open until a safe open position
        long brakePos = motorBrakes.getPos();
        const long OPEN_POS_TARGET = 0;     // adjust to your mechanical zero
        const int  OPEN_SPEED      = -1000; // speed for opening (negative)

        if (brakePos > OPEN_POS_TARGET) {
            // Still closed -> open
            motorBrakes.setSpeed(OPEN_SPEED);
        } else {
            // Fully open -> stop motor
            motorBrakes.setSpeed(0);
        }
    }

    // 4) Debug prints
    /*
    Serial.print("ENC_TICKS: ");    Serial.print(encTicks);
    Serial.print("  dTICKS: ");     Serial.print(dTicks);
    Serial.print("  SPEED_TICKS: ");Serial.print(speedTicks);
    Serial.print("  M2_POS: ");     Serial.print(motorBrakes.getPos());
    Serial.print("  M2_SPEED_CMD: ");Serial.println(motorBrakes.getSpeed());
    */

    // 5) Keep loop period consistent with CONTROL_DT
    delay(CONTROL_PERIOD_MS);
}
