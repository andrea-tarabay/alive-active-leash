#include <SCServo.h>

// ====== PINS ======
#define PIN_SERVO_RX   19   // MEGA RX1
#define PIN_SERVO_TX   18   // MEGA TX1
#define PIN_BUTTON     2
#define PIN_POT        A0

// ====== SERVO ======
SMS_STS sms_sts;

void setup()
{
  // debug serial to PC (USB)
  Serial.begin(115200);

  // servo bus
  Serial1.begin(1000000, SERIAL_8N1);
  sms_sts.pSerial = &Serial1;

  // button
  pinMode(PIN_BUTTON, INPUT_PULLUP);  // internal pull-up ON

  // potentiometer
  pinMode(PIN_POT, INPUT);

  delay(1000);

  Serial.println("=== SERVO PING SELF-TEST ===");

  int id1 = sms_sts.Ping(1);
  if(id1 != -1){
    Serial.println("Servo ID 1 (Spool motor): OK");
  }else{
    Serial.println("Servo ID 1 (Spool motor): NOT FOUND");
  }

  int id2 = sms_sts.Ping(2);
  if(id2 != -1){
    Serial.println("Servo ID 2 (Brakes motor): OK");
  }else{
    Serial.println("Servo ID 2 (Brakes motor): NOT FOUND");
  }

  Serial.println("=== END OF SELF-TEST ===");
  Serial.println();
}

void loop()
{
  // read button
  bool buttonState = digitalRead(PIN_BUTTON);
  // NOTE: with INPUT_PULLUP
  // LOW  = pressed
  // HIGH = released

  // read potentiometer
  int potVal = analogRead(PIN_POT);

  Serial.print("Button: ");
  Serial.print(buttonState == LOW ? "PRESSED" : "RELEASED");
  Serial.print(" | Pot=");
  Serial.println(potVal);

  delay(1000);
}
