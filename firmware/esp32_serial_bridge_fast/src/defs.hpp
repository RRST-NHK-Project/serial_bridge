#pragma once

#include "config.hpp"
#include <Arduino.h>

// ================= Pin mapping =================
#define LED 0

// ================= Constants =================
constexpr int kNumMotor = NUM_MOTOR;
constexpr int kEncoderMax = ENCODER_MAX;
constexpr int kHalfEncoder = ENCODER_MAX / 2;
constexpr float kGearM2006 = GEAR_M2006;
constexpr float kControlPeriodSec = 1.0f / CONTROL_RATE_HZ;
