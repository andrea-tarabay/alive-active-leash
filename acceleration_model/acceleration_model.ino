#include <SCServo.h>

// Motor ID and serial bus
#define MOTOR_ID 1
SMS_STS st;

unsigned long testStartTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 20;  // Print every 20ms
const unsigned long offDuration = 500;   // Motor off for 0.5 seconds
const unsigned long onDuration = 5000;   // Motor on for 5 seconds
const unsigned long pauseDuration = 2000; // 2 seconds pause between tests
const int numTests = 5;
int currentTest = 0;
bool motorStarted = false;
bool testComplete = false;
bool allTestsComplete = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize servo bus
  Serial1.begin(1000000, SERIAL_8N1);
  st.pSerial = &Serial1;
  
  delay(1000);
  
  // Set motor to wheel mode (speed control)
  st.writeByte(MOTOR_ID, SMS_STS_MODE, 1);  // Wheel mode
  
  // Start with motor off
  st.EnableTorque(MOTOR_ID, 1);
  st.WriteSpe(MOTOR_ID, 0, 0);
  
  delay(500);
  
  // Print CSV header
  Serial.println("TestNum,Time_ms,Speed");
  
  testStartTime = millis();
  lastPrintTime = testStartTime;
}

void loop() {
  if (allTestsComplete) {
    return;  // Stop after all tests are complete
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - testStartTime;
  
  // Turn on motor after 0.5 seconds
  if (!motorStarted && elapsed >= offDuration) {
    st.WriteSpe(MOTOR_ID, 3000, 0);  // Speed 3000, acceleration 0
    motorStarted = true;
  }
  
  // Turn off motor after 5.5 seconds (0.5s off + 5s on)
  if (motorStarted && elapsed >= (offDuration + onDuration)) {
    st.WriteSpe(MOTOR_ID, 0, 0);
    testComplete = true;
  }
  
  // Wait for pause between tests
  if (testComplete && elapsed >= (offDuration + onDuration + pauseDuration)) {
    currentTest++;
    
    if (currentTest >= numTests) {
      allTestsComplete = true;
      Serial.println("ALL_TESTS_COMPLETE");
      return;
    }
    
    // Reset for next test
    motorStarted = false;
    testComplete = false;
    testStartTime = millis();
    lastPrintTime = testStartTime;
    return;
  }
  
  // Print speed every 20ms (only during active test, not during pause)
  if (!testComplete && currentTime - lastPrintTime >= printInterval) {
    int currentSpeed = st.ReadSpeed(MOTOR_ID);
    
    Serial.print(currentTest);
    Serial.print(",");
    Serial.print(elapsed);
    Serial.print(",");
    Serial.println(currentSpeed);
    
    lastPrintTime = currentTime;
  }
}
