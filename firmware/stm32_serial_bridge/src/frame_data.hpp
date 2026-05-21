/*====================================================================
<frame_data.hpp>
Frame slot definitions for stm32_serial_bridge.

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
1..8: ENC1..ENC8
9..16: SW1..SW8
*/

extern volatile int16_t Rx_16Data[Rx16NUM];
/*
0: reserved/debug
1..8: MD1..MD8
9..16: SERVO1..SERVO8
17..23: TR1..TR7
*/
