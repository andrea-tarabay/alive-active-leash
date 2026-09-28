#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H

#include <Arduino.h>

struct PIDConfig {
  float Kp;
  float Ki;
  float Kd;
  float dt;      // control period [s]
  float outMin;  // min output
  float outMax;  // max output
};

class PIDController {
public:
  PIDController();
  void setConfig(const PIDConfig &cfg);
  void reset();
  float update(float error);   // returns control output

private:
  PIDConfig cfg_;
  float integ_;
  float prevError_;
  bool  havePrev_;
};

#endif // PIDCONTROLLER_H