#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SCServo.h>

// ====== PINS ======
#define PIN_SERVO_RX   19   // MEGA RX1
#define PIN_SERVO_TX   18   // MEGA TX1
#define PIN_BUTTON     2
#define PIN_POT        A0

// ------------------ UART / Servos ------------------
//extern SMS_STS st;  // to control the motors, use pin 18 and 19 (serial 1)
#define ID_MOTOR_SPOOL 1
#define ID_MOTOR_BRAKES 2

#define MOTOR_MAX_SPEED 4000 // max speed for the motors
#define MOTOR_SPOOL_SPEED_MIN 50
#define MOTOR_SPOOL_STATIC_FRICTION 8
#define MOTOR_SPOOL_VISCOUS_FRICTION 0.148

#define BRAKES_PID_KP 400.0f
#define BRAKES_PID_KI 100.0f
#define BRAKES_PID_KD 30.0f
#define SPOOL_PID_KP 180.0f
#define SPOOL_PID_KI 0.0f
#define SPOOL_PID_KD 0.0f
#define CONTROL_PERIOD_MS 20
#define CONTROL_DT ((float)CONTROL_PERIOD_MS / 1000.0f)

#define LOAD_THRESHOLD 8 // minimal load to consider the motor is under load

// ------------------ System parameters ------------------
#define MIN_ROPE_LENGTH 200 //minimal distance in tickes
#define TICKS_PER_REV 4096  // ticks per revolution of the motor
#define SPOOL_RADIUS_M 0.02f    // m
#define MAX_ROTATION 50000 // determined by tests

#endif

