/*====================================================================
<defs.hpp>
Constants and pin definitions.

- Keep configuration (ID / mode selection) in config.hpp
- Keep pins and constant values in this file
====================================================================*/

#pragma once

#include "config.hpp"

#include <Arduino.h>

// ================= Serial =================

#define SERIAL_BAUD 115200

// Periodic TX frame interval (MCU -> PC)
#define TX_PERIOD_MS 20

// ================= Servo =================

#define ENABLE_SERVO1 1
#define ENABLE_SERVO2 1
#define ENABLE_SERVO3 1
#define ENABLE_SERVO4 1
#define ENABLE_SERVO5 1
#define ENABLE_SERVO6 1

// Default pin assignment (change as needed)
// Note: Arduino Uno's Servo library can output on (almost) any digital pin.
#define SERVO1_PIN 9
#define SERVO2_PIN 10
#define SERVO3_PIN 11
#define SERVO4_PIN 6
#define SERVO5_PIN 5
#define SERVO6_PIN 3

// Angle input unit: degrees (from serial_bridge)
#define SERVO1_MIN_DEG 0
#define SERVO1_MAX_DEG 180
#define SERVO1_INIT_DEG 90
#define SERVO1_MIN_US 500
#define SERVO1_MAX_US 2500

#define SERVO2_MIN_DEG 0
#define SERVO2_MAX_DEG 180
#define SERVO2_INIT_DEG 90
#define SERVO2_MIN_US 500
#define SERVO2_MAX_US 2500

#define SERVO3_MIN_DEG 0
#define SERVO3_MAX_DEG 180
#define SERVO3_INIT_DEG 90
#define SERVO3_MIN_US 500
#define SERVO3_MAX_US 2500

#define SERVO4_MIN_DEG 0
#define SERVO4_MAX_DEG 180
#define SERVO4_INIT_DEG 90
#define SERVO4_MIN_US 500
#define SERVO4_MAX_US 2500

#define SERVO5_MIN_DEG 0
#define SERVO5_MAX_DEG 180
#define SERVO5_INIT_DEG 90
#define SERVO5_MIN_US 500
#define SERVO5_MAX_US 2500

#define SERVO6_MIN_DEG 0
#define SERVO6_MAX_DEG 180
#define SERVO6_INIT_DEG 90
#define SERVO6_MIN_US 500
#define SERVO6_MAX_US 2500

// ================= Switch =================

// Using INPUT_PULLUP by default. Active-low means pressed==LOW.
#define SW_ACTIVE_LOW 1

// Default assignment avoids D0/D1 (Serial). Analog pins can be used as digital.
#define SW1_PIN 2
#define SW2_PIN 4
#define SW3_PIN 7
#define SW4_PIN 8
#define SW5_PIN A0
#define SW6_PIN A1
#define SW7_PIN A2
#define SW8_PIN A3
