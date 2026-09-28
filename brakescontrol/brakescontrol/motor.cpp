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

float Motor::computeTorque(int speed)
{
    if(speed >= 0) {
        if(speed < speedThreshold)
            //return -staticCoeff;
            return 0;
        else
            return -staticCoeff - viscousCoeff * speed;
    } else {
        if(speed > -speedThreshold)
            //return staticCoeff;
            return 0;
        else
            return staticCoeff - viscousCoeff * speed;
    }
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
