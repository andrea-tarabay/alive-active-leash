#include "Motor.h"
#include <Arduino.h>

Motor::Motor(uint8_t id, SMS_STS* bus, float staticC, float viscousC, int speedLimit)
: motorID(id), servoBus(bus),
  staticCoeff(staticC), viscousCoeff(viscousC), speedThreshold(speedLimit)
{
    // default residual filter: alpha between 0 (max smoothing) and 1 (no smoothing)
    residualFilterAlpha = 0.08f;  // Balanced filtering
    residualFiltered = 0.0f;
}

//only for the spool motor, hardcoded from experimental data
float Motor::computeTorque(int speed)
{
    // Lookup table generated from experimental data
    // Speed range: -3000 to 3000 in steps of 50
    // Array index = (speed + 3000) / 50
    static const float loadTable[] = {
        484.50f,   // -3000
        474.55f,   // -2950
        466.00f,   // -2900
        458.60f,   // -2850
        449.60f,   // -2800
        441.90f,   // -2750
        434.30f,   // -2700
        425.65f,   // -2650
        418.45f,   // -2600
        410.90f,   // -2550
        402.00f,   // -2500
        393.80f,   // -2450
        385.20f,   // -2400
        378.10f,   // -2350
        370.70f,   // -2300
        361.75f,   // -2250
        353.90f,   // -2200
        346.65f,   // -2150
        338.45f,   // -2100
        331.10f,   // -2050
        322.80f,   // -2000
        313.90f,   // -1950
        306.70f,   // -1900
        298.55f,   // -1850
        289.85f,   // -1800
        282.40f,   // -1750
        275.00f,   // -1700
        266.60f,   // -1650
        258.30f,   // -1600
        250.35f,   // -1550
        242.70f,   // -1500
        234.80f,   // -1450
        226.90f,   // -1400
        219.00f,   // -1350
        210.65f,   // -1300
        202.80f,   // -1250
        194.05f,   // -1200
        186.00f,   // -1150
        178.65f,   // -1100
        170.80f,   // -1050
        164.00f,   // -1000
        154.30f,   //  -950
        147.05f,   //  -900
        139.75f,   //  -850
        130.55f,   //  -800
        123.35f,   //  -750
        114.70f,   //  -700
        107.35f,   //  -650
         99.30f,   //  -600
         91.35f,   //  -550
         83.50f,   //  -500
         76.15f,   //  -450
         68.50f,   //  -400
         60.15f,   //  -350
         52.70f,   //  -300
         45.15f,   //  -250
         38.25f,   //  -200
         32.15f,   //  -150
         26.45f,   //  -100
         19.90f,   //   -50
         15.00f,   //     0
        -20.40f,   //    50
        -27.45f,   //   100
        -32.20f,   //   150
        -38.55f,   //   200
        -45.00f,   //   250
        -52.70f,   //   300
        -59.85f,   //   350
        -68.90f,   //   400
        -76.10f,   //   450
        -83.35f,   //   500
        -91.50f,   //   550
        -98.90f,   //   600
       -108.40f,   //   650
       -115.45f,   //   700
       -123.15f,   //   750
       -130.15f,   //   800
       -141.00f,   //   850
       -146.55f,   //   900
       -154.10f,   //   950
       -163.95f,   //  1000
       -170.40f,   //  1050
       -178.45f,   //  1100
       -185.90f,   //  1150
       -193.65f,   //  1200
       -203.35f,   //  1250
       -209.15f,   //  1300
       -219.70f,   //  1350
       -225.35f,   //  1400
       -235.35f,   //  1450
       -241.45f,   //  1500
       -250.35f,   //  1550
       -258.00f,   //  1600
       -264.80f,   //  1650
       -275.05f,   //  1700
       -280.70f,   //  1750
       -289.25f,   //  1800
       -298.05f,   //  1850
       -304.55f,   //  1900
       -312.55f,   //  1950
       -321.90f,   //  2000
       -328.55f,   //  2050
       -335.70f,   //  2100
       -344.45f,   //  2150
       -353.15f,   //  2200
       -360.95f,   //  2250
       -368.35f,   //  2300
       -374.55f,   //  2350
       -382.40f,   //  2400
       -390.95f,   //  2450
       -398.95f,   //  2500
       -407.30f,   //  2550
       -415.00f,   //  2600
       -422.65f,   //  2650
       -430.40f,   //  2700
       -438.50f,   //  2750
       -446.15f,   //  2800
       -454.50f,   //  2850
       -460.85f,   //  2900
       -468.55f,   //  2950
       -476.00f    //  3000
    };

    // Clamp speed to valid range
    if (speed < -3000) speed = -3000;
    if (speed > 3000) speed = 3000;

    // Calculate array index: index = (speed + 3000) / 50
    int index = (speed + 3000) / 50;
    
    // Direct array lookup - O(1) constant time
    return loadTable[index];
}

long Motor::getPos()
{
    // read present pos from servo
    int raw = servoBus->ReadPos(motorID);

    int diff = raw - lastRaw;

    // wrap-around
    if(diff >  2000) accumulated -= 4096; // ex: 10 → 4090
    if(diff < -2000) accumulated += 4096; // ex: 4090 → 10

    lastRaw = raw;

    // absolue position = raw + accumulated
    return raw + accumulated- initialRaw;
}

int Motor::getSpeed()
{
    return servoBus->ReadSpeed(motorID);
}

int Motor::getTorque()
{
    return servoBus->ReadLoad(motorID);
}

TorqueData Motor::getResidualTorque(){
    TorqueData result;
    
    // Use FeedBack to read all values at once from the servo bus
    if (servoBus->FeedBack(motorID) != -1) {
        int speed = servoBus->ReadSpeed(motorID);
        int actualLoad = servoBus->ReadLoad(motorID);
        
        result.actualTorque = actualLoad;
        float predicted;
        if(actualLoad == 0){
            predicted = 0.0f;
        }else{
            predicted = computeTorque(speed);
        }

        // torque résiduel = charge réelle - charge prédite
        result.modeledTorque = predicted;
        float rawResidual = static_cast<float>(actualLoad) - predicted;
        // exponential moving average filter
        residualFiltered = residualFiltered * (1.0f - residualFilterAlpha) + rawResidual * residualFilterAlpha;
        result.residualTorque = residualFiltered;
    } else {
        // If FeedBack fails, return zeros
        result.actualTorque = 0;
        result.modeledTorque = 0.0f;
        result.residualTorque = 0.0f;
    }
    
    return result;
}

void Motor::setResidualFilterAlpha(float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    residualFilterAlpha = alpha;
}

void Motor::setMode(MotorMode mode)
{   
    servoBus->unLockEprom(motorID);
    if(mode == MODE_SPEED){
        // wheel mode = 1
        servoBus->writeByte(motorID, SMS_STS_MODE, 1);
    }else{
        // joint mode = 0
        servoBus->writeByte(motorID, SMS_STS_MODE, 0);
    }
    servoBus->LockEprom(motorID);
}

//it's not suposed to be in the eeprom -> to check
void Motor::enableTorque(bool torque)
{
    servoBus->EnableTorque(motorID, torque);
}

void Motor::setSpeed(int speed)
{
    servoBus->WriteSpe(motorID, speed, 0);
}

void Motor::resetInitialRaw(){
    int raw = servoBus->ReadPos(motorID);
    initialRaw = raw;
    lastRaw = raw;
    accumulated = 0;
}
