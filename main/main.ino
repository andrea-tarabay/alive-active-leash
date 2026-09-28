#include "main.h"
#include "motor.h"
#include "setup_helpers.h"
#include "PIDController.h"

enum State : uint8_t {
    STATE_FREE = 0,
    STATE_FREE_REWIND = 1,
    STATE_BRACKING = 2,
    STATE_FORCE_REWIND = 3
};

State currentState = STATE_FREE;
// motors
SMS_STS st;
Motor motorSpool(ID_MOTOR_SPOOL, &st, MOTOR_SPOOL_STATIC_FRICTION, MOTOR_SPOOL_VISCOUS_FRICTION, MOTOR_MIN_SPEED);
Motor motorBrakes(ID_MOTOR_BRAKES, &st);
// setup PID controller for brakes
PIDController brakesPID;
PIDController spoolPID;

// bouton
bool motorRunning = false;
bool lastButtonState = HIGH;

// global variables
int speedBack = 0;

// Speed command filter for spool motor
float spoolSpeedFiltered = 50.0f;
float spoolSpeedFilterAlpha = 0.1f;  // 0 = max smoothing, 1 = no smoothing

// Petite fonction utilitaire pour afficher l'état en texte
const char* stateName(State s) {
    switch(s) {
        case STATE_FREE:         return "STATE_FREE";
        case STATE_FREE_REWIND:  return "STATE_FREE_REWIND";
        case STATE_BRACKING:     return "STATE_BRACKING";
        case STATE_FORCE_REWIND: return "STATE_FORCE_REWIND";
        default:                 return "UNKNOWN";
    }
}

// --- STATE CHANGE HELPERS ---
void goToFree() {
    //Serial.print("\n➡️  Switching state: ");
    //Serial.print(stateName(currentState));
    //Serial.print("  →  ");
    //Serial.println("STATE_FREE");

    currentState = STATE_FREE;
    spoolPID.reset();
    TorqueData torqueCheck = motorSpool.getResidualTorque();
    int speedCmd = (int)spoolPID.update(torqueCheck.residualTorque);
    //motorSpool.enableTorque(true);
    motorSpool.setSpeed(speedCmd);
    spoolSpeedFiltered = speedCmd;

    //Serial.println("   🟢 Free mode: torque OFF, spool floating.");
}

void goToFreeRewind() {
    speedBack = -MOTOR_MIN_SPEED;
    //Serial.print("\n➡️  Switching state: ");
    //Serial.print(stateName(currentState));
    //Serial.print("  →  ");
    //Serial.println("STATE_FREE_REWIND");

    currentState = STATE_FREE_REWIND;
    //motorSpool.enableTorque(true);
    spoolSpeedFiltered = speedBack;
    motorSpool.setSpeed((int)spoolSpeedFiltered);

    //Serial.println("   🔄 Free rewind: torque ON, rewinding spool at speedback=0.");
    //delay(200); //to let the speed stabilize
}

void goToBraking() {
    //Serial.print("\n➡️  Switching state: ");
    //Serial.print(stateName(currentState));
    //Serial.print("  →  ");
    //Serial.println("STATE_BRACKING");

    currentState = STATE_BRACKING;
    //motorSpool.enableTorque(false);
    // reset PID here
    brakesPID.reset();
    float error = motorSpool.getSpeed();
    int   speedCmd = (int)brakesPID.update(error);
    motorBrakes.setSpeed(speedCmd);

    //Serial.println("   🟥 Braking: spool torque OFF, brakes controlled.");
}

void goToForceRewind() {
    speedBack = 0;
    Serial.print("\n➡️  Switching state: ");
    //Serial.print(stateName(currentState));
    //Serial.print("  →  ");
    //Serial.println("STATE_FORCE_REWIND");

    currentState = STATE_FORCE_REWIND;
    //motorSpool.enableTorque(true);
    motorSpool.setSpeed(speedBack);
    spoolSpeedFiltered = speedBack;

    //Serial.println("   🔧 Force rewind: torque ON, pulling at speedback.");
    //delay(200); //to let the speed stabilize
}


void setup() {
  // debug serial to PC (USB)
  Serial.begin(115200);

  // servo bus
  Serial1.begin(1000000, SERIAL_8N1);
  st.pSerial = &Serial1;

  // === BRAKES PID CONFIGURATION ===
  // error unit: servo speed  ->  output unit: servo speed [0..BRAKE_MAX_SPEED]
  PIDConfig cfg;
  cfg.Kp = BRAKES_PID_KP;    // proportional: main braking strength
  cfg.Ki = BRAKES_PID_KI;     // integral: remove small residual drift
  cfg.Kd = BRAKES_PID_KD;      // derivative: extra damping (less oscillation)
  cfg.dt     = CONTROL_DT;
  cfg.outMin = 0.0f;                // only positive brake strength
  cfg.outMax = MOTOR_MAX_SPEED;
  brakesPID.setConfig(cfg);

  // === SPOOL PID CONFIGURATION ===
  // error unit: residual torque  ->  output unit: servo speed [0..MOTOR_MAX_SPEED]
  PIDConfig spoolCfg;
  spoolCfg.Kp = SPOOL_PID_KP;    // proportional: main control
  spoolCfg.Ki = SPOOL_PID_KI;     // integral: none
  spoolCfg.Kd = SPOOL_PID_KD;      // derivative: none
  spoolCfg.dt     = CONTROL_DT;
  spoolCfg.outMin = -MOTOR_MAX_SPEED;                // only positive speed
  spoolCfg.outMax = MOTOR_MAX_SPEED;
  spoolPID.setConfig(spoolCfg);

  // initialize pins
  setupPins();

  // set motors mode and start with "disconnected" motors
  motorSpool.resetInitialRaw();
  motorSpool.setMode(MODE_SPEED);
  motorBrakes.setMode(MODE_SPEED);
  //Turn on the motors at 0 speed
  motorBrakes.setSpeed(0);
  motorSpool.setSpeed(0);
  motorSpool.enableTorque(true);
  motorBrakes.enableTorque(true);
  delay(50);

  // Interactive release of brakes (press to toggle)
  releaseBrakesInteractive(motorBrakes);

  // Autonomous brakes calibration
  autoBrakesCalibration(motorSpool, motorBrakes);

  // Manual spool calibration via button (press to run -2000, press again to stop and reset)
  manualSpoolCalibration(motorSpool);
  
  //turn off the motors
  motorBrakes.setSpeed(0);
  //motorBrakes.enableTorque(false);
  //motorSpool.enableTorque(true);
  motorSpool.setSpeed(MOTOR_MIN_SPEED);
  delay(50);
}

void loop() {
    // prints ad current values
    long brakesPosition = motorBrakes.getPos();
    int brakesSpeed = motorBrakes.getSpeed();
    long ropeLenght  = motorSpool.getPos();
    int ropeSpeed= motorSpool.getSpeed();
    TorqueData torqueData = motorSpool.getResidualTorque();
    long potValue = (long)((analogRead(PIN_POT) / 1023.0f) * (float)MAX_ROTATION);
   
    switch(currentState){
        case STATE_FREE : {
            static unsigned long lowTorqueStartTime = 0;
            if(brakesPosition > 20){ //brakes not fully open, open them
                motorBrakes.setSpeed(-2000);
            }else{
                motorBrakes.setSpeed(0);
            }
            if(ropeLenght> potValue){
                goToBraking();
            //there is always a small speed applied, no need fo that
            //}else if(ropeLenght< 0){ //too close to the handle go to zero slowly
            //    spoolSpeedFiltered = MOTOR_MIN_SPEED;
            //    motorSpool.setSpeed((int)spoolSpeedFiltered);
            }else{
                TorqueData torqueCheck = motorSpool.getResidualTorque();
                
                // Check if torque has been low for sustained period (500ms)
                if(torqueCheck.residualTorque < LOAD_THRESHOLD_REWIND && abs(ropeSpeed) >= MOTOR_MIN_SPEED){
                    if(lowTorqueStartTime == 0){
                        lowTorqueStartTime = millis();
                    } else if(millis() - lowTorqueStartTime > 500){
                        lowTorqueStartTime = 0;
                        goToFreeRewind();
                    }
                } else {
                    lowTorqueStartTime = 0;  // Reset timer if torque goes back up
                }
                
                // Run PID continuously
                int speedCmd = (int)spoolPID.update(torqueCheck.residualTorque);
                // Apply exponential moving average filter
                spoolSpeedFiltered = spoolSpeedFiltered * (1.0f - spoolSpeedFilterAlpha) + speedCmd * spoolSpeedFilterAlpha;

                if(ropeLenght< 0 && spoolSpeedFiltered < MOTOR_MIN_SPEED){
                    spoolSpeedFiltered = MOTOR_MIN_SPEED;
                }
                motorSpool.setSpeed((int)spoolSpeedFiltered);
            }

        }
        break;
        case STATE_FREE_REWIND: {
            if(brakesPosition > 20){ //brakes not fully open, open them
                motorBrakes.setSpeed(-2000);
            }else{
                motorBrakes.setSpeed(0);
            }
            speedBack += 5; //increase speed slowly
            if(speedBack > 2000) speedBack = 2000;
            spoolSpeedFiltered = -speedBack;
            motorSpool.setSpeed((int)spoolSpeedFiltered);
            if(ropeLenght< MIN_ROPE_LENGTH){
                //Serial.println("fin de course");
                goToFree();
            }
            if(ropeLenght> potValue){
                goToBraking();
            }
            //check torque
            //if the torque is nul, stay in this state
            //if the torque is positive, switch to free state, turn off the motor (avoid values close to zero speed as they are too noisy)
            if(torqueData.residualTorque > LOAD_THRESHOLD_UNWIND && ropeSpeed <= -MOTOR_MIN_SPEED){
                goToFree();
            }
        }
        break;
        case STATE_BRACKING:{
            //get the speed to apply to the brakes motor
            //apply it
            // target speed = 0 -> error = v_mps
            float error = ropeSpeed;
            int speedCmd_brakes     = (int)brakesPID.update(error);
            motorBrakes.setSpeed(speedCmd_brakes);

            //continue with transpaenrt mode on the spool
            TorqueData torqueCheck = motorSpool.getResidualTorque();
            int speedCmd = (int)spoolPID.update(torqueCheck.residualTorque);
            // Apply exponential moving average filter
            spoolSpeedFiltered = spoolSpeedFiltered * (1.0f - spoolSpeedFilterAlpha) + speedCmd * spoolSpeedFilterAlpha;
            motorSpool.setSpeed((int)spoolSpeedFiltered);

            // Serial Plotter: velocity + state flag (1 = BRAKING)
            //Serial.print("error: ");
            //Serial.print(error);
            //Serial.print(" | brake speed cmd: ");
            //Serial.println(speedCmd);
            if(ropeSpeed <= 100){ //speed resolution is 50
                goToForceRewind();
            }
        }
        break;
        case STATE_FORCE_REWIND:{
            if(brakesPosition > 20){ //brakes not fully open
                //hold the torque with the spool motor and release the brakes
                speedBack = 0;
                motorBrakes.setSpeed(-2000);
            }else{
                //rewind the rope
                motorBrakes.setSpeed(0);
                speedBack += 50; //increase speed slowly
                if(speedBack > 2000) speedBack = 2000;
            }
            spoolSpeedFiltered = -speedBack;
            motorSpool.setSpeed((int)spoolSpeedFiltered);

            if (ropeLenght < static_cast<long>(0.8 * potValue)) {
                goToFree();
            }
            //if torque is to high, return to brake state
        }
        break;
        default:
            goToFree();
            break;
    }

    Serial.print("M1_TORQUE: "); Serial.print(torqueData.actualTorque);
    Serial.print(" M1_MODELED_TORQUE: "); Serial.print(torqueData.modeledTorque);
    Serial.print(" M1_SPEED: "); Serial.print(ropeSpeed);
    Serial.print(" M1_POS: "); Serial.print(ropeLenght);
    Serial.print(" M1_RESIDUAL_LOAD: "); Serial.print(torqueData.residualTorque);
    Serial.print(" M1_SPEED_CMD: "); Serial.print((int)spoolSpeedFiltered);
    Serial.print(" M2_SPEED: "); Serial.print(brakesSpeed);
    Serial.print(" M2_POS: "); Serial.print(brakesPosition);
    Serial.print(" POT: "); Serial.print(potValue);
    Serial.print(" STATE: "); Serial.println(currentState);

    delay(CONTROL_PERIOD_MS);
}

