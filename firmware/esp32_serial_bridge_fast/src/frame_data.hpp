#pragma once

#include <stdint.h>

#define Tx16NUM 24
#define Rx16NUM 24

extern volatile int16_t Tx_16Data[Tx16NUM];
/*
0: status/debug
5..8: measured velocity (rpm)
13..16: measured angle (deg)
1..4, 9..12, 17..23: reserved
*/

extern volatile int16_t Rx_16Data[Rx16NUM];
/*
0: reserved/debug
5..8: target velocity (rpm)
1..4, 9..23: reserved
*/
