#ifndef MOTOR_H
#define MOTOR_H

#include <SCServo.h>

enum MotorMode : uint8_t {
    MODE_POS = 0,
    MODE_SPEED = 1
};

struct TorqueData {
    int actualTorque;
    float modeledTorque;
    float residualTorque;
};

class Motor {
public:
    Motor(uint8_t id, SMS_STS* bus,
          float staticC = 0.0f,
          float viscousC = 0.0f,
          int speedLimit = 0);

    long getPos();
    int getSpeed();
    int getTorque(); //without motor friction compensation, nul if the torque is desactivated
    float computeTorque(int speed);
    TorqueData getResidualTorque();

    void setMode(MotorMode mode);
    void enableTorque(bool torque);
    void setSpeed(int speed);
    void resetInitialRaw();
    // Residual torque filtering: set smoothing alpha in range [0..1]
    void setResidualFilterAlpha(float alpha);

private:
    uint8_t motorID;
    SMS_STS* servoBus;

    int initialRaw;
    int lastRaw;
    long accumulated;

    // friction model
    float staticCoeff;
    float viscousCoeff;
    int speedThreshold;

    // residual torque filter state
    float residualFiltered;
    float residualFilterAlpha;
};

#endif
