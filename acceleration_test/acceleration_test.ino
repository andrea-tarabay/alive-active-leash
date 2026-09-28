#include <SCServo.h>

// Motor ID and serial bus
#define MOTOR_ID 1
SMS_STS st;

unsigned long lastPrintTime = 0;
const unsigned long printInterval = 20;  // Print every 20ms
bool commandSent = false;
bool stopped = false;
unsigned long testStartTime = 0;

void setup() {
  // debug serial to PC (USB)
  Serial.begin(115200);

  // servo bus
  Serial1.begin(1000000, SERIAL_8N1);
  st.pSerial = &Serial1;
  
  delay(1000);
  
  // Set motor to wheel mode (speed control)
  st.unLockEprom(MOTOR_ID);
  st.writeByte(MOTOR_ID, SMS_STS_MODE, 1);  // Wheel mode
  st.LockEprom(MOTOR_ID);
  
  // Start with speed 0
  st.EnableTorque(MOTOR_ID, 1);
  st.WriteSpe(MOTOR_ID, 0, 0);
  
  delay(1000);
  
  // Print CSV header
  Serial.println("Time_ms,Speed,CommandSent");
  
  testStartTime = millis();
  lastPrintTime = testStartTime;
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send speed command after 500ms
  if (!commandSent && (currentTime - testStartTime >= 500)) {
    Serial.println("motor started");
    st.WriteSpe(MOTOR_ID, 3000, 0);  // Set speed to 3000 with 0 acceleration
    commandSent = true;
  }
  
  // Stop motor after 5 seconds
  if (!stopped && (currentTime - testStartTime >= 5000)) {
    st.WriteSpe(MOTOR_ID, 0, 0);  // Stop motor
    stopped = true;
    Serial.println("MOTOR_STOPPED");
  }
  
  // Print data every 20ms
  if (currentTime - lastPrintTime >= printInterval) {
    int currentSpeed = st.ReadSpeed(MOTOR_ID);
    
    Serial.print(currentTime - testStartTime);
    Serial.print(",");
    Serial.print(currentSpeed);
    Serial.print(",");
    Serial.println(commandSent ? 1 : 0);
    
    lastPrintTime = currentTime;
  }
}
