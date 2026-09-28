#include <SCServo.h>

#define PIN_SERVO_RX 19
#define PIN_SERVO_TX 18

SMS_STS servo;
const int SERVO_ID = 2;
const int MAX_SPEED = 3000;
const int STEP_SPEED = 50;
const int NUM_MEASURES = 10;

void setup() {
    Serial.begin(115200);
    Serial1.begin(1000000, SERIAL_8N1);
    servo.pSerial = &Serial1;

    servo.WheelMode(SERVO_ID);

    Serial.println("Speed,Load1,Load2,Load3,Load4,Load5,Load6,Load7,Load8,Load9,Load10");
    delay(500);
}

void loop() {
    for(int speed = -MAX_SPEED; speed <= MAX_SPEED; speed += STEP_SPEED){
        servo.WriteSpe(SERVO_ID, speed, 10);

        // wait 2 seconds at starting speed to stabilize
        if(speed == -MAX_SPEED){
            delay(2000);
        } else {
            delay(500); // regular stabilization
        }

        Serial.print(speed); // speed column
        for(int i = 0; i < NUM_MEASURES; i++){
            int load = servo.ReadLoad(SERVO_ID);
            if(load == -1) load = 0; // fallback
            Serial.print(",");
            Serial.print(load);
            delay(100);
        }
        Serial.println(); // end of line
    }
    for(int speed = MAX_SPEED; speed >= -MAX_SPEED; speed -= STEP_SPEED){
        servo.WriteSpe(SERVO_ID, speed, 10);

        // wait 2 seconds at starting speed to stabilize
        if(speed == MAX_SPEED){
            delay(2000);
        } else {
            delay(500); // regular stabilization
        }

        Serial.print(speed); // speed column
        for(int i = 0; i < NUM_MEASURES; i++){
            int load = servo.ReadLoad(SERVO_ID);
            if(load == -1) load = 0; // fallback
            Serial.print(",");
            Serial.print(load);
            delay(100);
        }
        Serial.println(); // end of line
    }

    // stop motor
    servo.WriteSpe(SERVO_ID, 0, 0);
    Serial.println("Measurement complete.");
    while(true); // stop loop
}
