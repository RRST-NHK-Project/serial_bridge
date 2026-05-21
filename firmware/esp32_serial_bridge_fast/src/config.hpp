#pragma once

#include <Arduino.h>

// ================= Basic settings =================
#define DEVICE_ID 0x03
#define SERIAL_BAUD 115200

// ================= Control timing =================
#define CONTROL_RATE_HZ 1000
#define CONTROL_PERIOD_US (1000000 / CONTROL_RATE_HZ)

// ================= RoboMaster M2006 (CAN) =================
#define NUM_MOTOR 4
#define CAN_TX 4
#define CAN_RX 2

#define ENCODER_MAX 8192
#define GEAR_M2006 36.0f

// ================= PID gains (velocity) =================
#define KP_VEL_M2006 0.8f
#define KI_VEL_M2006 0.0f
#define KD_VEL_M2006 0.02f

// ================= Current limit =================
#define M2006_MAX_CUR_A 1.0f

// ================= Misc =================
#define TX_PERIOD_MS 10
#define ENABLE_LED 1
