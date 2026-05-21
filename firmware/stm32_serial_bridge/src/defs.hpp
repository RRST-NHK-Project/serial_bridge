/*====================================================================
<defs.hpp>
Constants and pin definitions.

- Keep configuration (ID / mode selection) in config.hpp
- Keep pins and constant values in this file
====================================================================*/

#pragma once

#include "config.hpp"

#include <Arduino.h>

// ================= Board helpers =================

// Nucleo built-in LED/button names are provided by the STM32 Arduino core.
// On other boards, redefine these as needed.
#ifndef BUILTIN_LED
#define BUILTIN_LED LED_BUILTIN
#endif

// ================= Frame / timing =================

#define SERIAL_BAUD 115200
#define TX_PERIOD_MS 20

// ================= MD (DC motor) =================

// MD output maximum (analogWrite scale)
#define MD_PWM_MAX 255

// Optional: if supported by your core
#define MD_PWM_FREQ 20000
#define MD_PWM_RESOLUTION 8

// PWM pins (8ch)
#define MD1P PB_6
#define MD2P PB_7
#define MD3P PB_8
#define MD4P PB_9
#define MD5P PA_8
#define MD6P PA_9
#define MD7P PA_10
#define MD8P PA_11

// DIR pins (8ch)
#define MD1D PB_0
#define MD2D PB_1
#define MD3D PB_2
#define MD4D PB_3
#define MD5D PC_0
#define MD6D PC_1
#define MD7D PC_2
#define MD8D PC_3

// ================= Servo =================

#define SERVO_PWM_FREQ 50
#define SERVO_PWM_RESOLUTION 16

#define SERVO_PWM_PERIOD_US (1000000.0 / SERVO_PWM_FREQ)
#define SERVO_PWM_MAX_DUTY ((1 << SERVO_PWM_RESOLUTION) - 1)
#define SERVO_PWM_SCALE (SERVO_PWM_MAX_DUTY / SERVO_PWM_PERIOD_US)

#define SERVO1 PA_0
#define SERVO2 PA_1
#define SERVO3 PA_2
#define SERVO4 PA_3
#define SERVO5 PB_4
#define SERVO6 PB_5
#define SERVO7 PA_6
#define SERVO8 PA_7

#define SERVO1_MIN_US 500
#define SERVO1_MAX_US 2500
#define SERVO1_MIN_DEG 0
#define SERVO1_MAX_DEG 180
#define SERVO1_INIT_DEG 90

#define SERVO2_MIN_US 500
#define SERVO2_MAX_US 2500
#define SERVO2_MIN_DEG 0
#define SERVO2_MAX_DEG 180
#define SERVO2_INIT_DEG 90

#define SERVO3_MIN_US 500
#define SERVO3_MAX_US 2500
#define SERVO3_MIN_DEG 0
#define SERVO3_MAX_DEG 180
#define SERVO3_INIT_DEG 90

#define SERVO4_MIN_US 500
#define SERVO4_MAX_US 2500
#define SERVO4_MIN_DEG 0
#define SERVO4_MAX_DEG 180
#define SERVO4_INIT_DEG 90

#define SERVO5_MIN_US 500
#define SERVO5_MAX_US 2500
#define SERVO5_MIN_DEG 0
#define SERVO5_MAX_DEG 180
#define SERVO5_INIT_DEG 90

#define SERVO6_MIN_US 500
#define SERVO6_MAX_US 2500
#define SERVO6_MIN_DEG 0
#define SERVO6_MAX_DEG 180
#define SERVO6_INIT_DEG 90

#define SERVO7_MIN_US 500
#define SERVO7_MAX_US 2500
#define SERVO7_MIN_DEG 0
#define SERVO7_MAX_DEG 180
#define SERVO7_INIT_DEG 90

#define SERVO8_MIN_US 500
#define SERVO8_MAX_US 2500
#define SERVO8_MIN_DEG 0
#define SERVO8_MAX_DEG 180
#define SERVO8_INIT_DEG 90

// ================= Encoder =================

// CPR for quadrature encoder (used by SimpleFOC Encoder helper)
#define ENC_CPR 2048

#define ENC1_A PC_4
#define ENC1_B PC_5
#define ENC2_A PC_6
#define ENC2_B PC_7
#define ENC3_A PC_8
#define ENC3_B PC_9
#define ENC4_A PC_10
#define ENC4_B PC_11

// Additional encoders
#define ENC5_A PA_4
#define ENC5_B PA_5
#define ENC6_A PB_12
#define ENC6_B PB_13
#define ENC7_A PB_14
#define ENC7_B PB_15
#define ENC8_A PB_10
#define ENC8_B PB_11

// ================= TR (digital outputs) =================

#define TR1 PD_0
#define TR2 PD_1
#define TR3 PD_2
#define TR4 PD_3
#define TR5 PD_4
#define TR6 PD_5
#define TR7 PD_6

// ================= Switch (digital inputs) =================

// Use INPUT_PULLUP by default; active-low.
#define SW_ACTIVE_LOW 1

// Default assignment avoids D0/D1 (Serial). Adjust for your wiring.
#define SW1 PD_7
#define SW2 PD_8
#define SW3 PD_9
#define SW4 PD_10
#define SW5 PD_11
#define SW6 PD_12
#define SW7 PD_13
#define SW8 PD_14
