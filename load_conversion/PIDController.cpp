// PIDController.cpp
#include "PIDController.h"

PIDController::PIDController()
  : integ_(0.0f), prevError_(0.0f), havePrev_(false)
{
  cfg_.Kp = 0.0f;
  cfg_.Ki = 0.0f;
  cfg_.Kd = 0.0f;
  cfg_.dt = 0.01f;
  cfg_.outMin = -1000.0f;
  cfg_.outMax =  1000.0f;
}

void PIDController::setConfig(const PIDConfig &cfg) {
  cfg_ = cfg;
  reset();
}

void PIDController::reset() {
  integ_     = 0.0f;
  prevError_ = 0.0f;
  havePrev_  = false;
}

float PIDController::update(float error) {
  // integrate
  integ_ += error * cfg_.dt;

  // derivative
  float deriv = 0.0f;
  if (havePrev_) {
    deriv = (error - prevError_) / cfg_.dt;
  }

  float u = cfg_.Kp * error + cfg_.Ki * integ_ + cfg_.Kd * deriv;

  // clamp
  if (u > cfg_.outMax) u = cfg_.outMax;
  if (u < cfg_.outMin) u = cfg_.outMin;

  Serial.print("PID update -> P: "); Serial.print(cfg_.Kp * error);
  Serial.print(" I: "); Serial.print(cfg_.Ki * integ_); 
  Serial.print(" D: "); Serial.print(cfg_.Kd * deriv);
  Serial.print(" | output: "); Serial.println(u);

  prevError_ = error;
  havePrev_  = true;
  return u;
}
