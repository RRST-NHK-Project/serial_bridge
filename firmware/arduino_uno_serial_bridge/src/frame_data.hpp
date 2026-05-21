/*====================================================================
<frame_data.hpp>
Frame slot definitions for arduino_uno_serial_bridge.

- RX: PC -> MCU (24 x int16)
- TX: MCU -> PC (17 x int16)
====================================================================*/

#pragma once

#include <stdint.h>

// MCU -> PC
#define Tx16NUM 17
// PC -> MCU
#define Rx16NUM 24

extern volatile int16_t Tx_16Data[Tx16NUM];
/*
0: reserved/debug
1..8: reserved (encoder/sensors)
9..16: SW1..SW8 (this Uno firmware uses 9..12 as SW1..SW4)
*/

extern volatile int16_t Rx_16Data[Rx16NUM];
/*
0: reserved/debug
1..8: reserved (motor commands)
9..16: SERVO1..SERVO8 (this Uno firmware uses slot 9 as SERVO1 angle)
17..23: reserved (TR outputs)
*/
