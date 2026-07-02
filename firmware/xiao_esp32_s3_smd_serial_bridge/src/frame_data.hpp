/*====================================================================
<frame_data.hpp>
・シリアル通信のフレームデータ定義ヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <stdint.h>

#define Tx16NUM 24          // 送信するint16データの数
#define Rx16NUM 24          // 受信するint16データの数
#define CAN_IO_SLOT_COUNT 5 // CANモードで扱うIO値の数

extern volatile int16_t Tx_16Data[Tx16NUM];
/*
0: デバッグ用
1~8: ENC1~8
9~16: SW1~8
17~23: 予備
*/

extern volatile int16_t Rx_16Data[Rx16NUM];
/*
0: デバッグ用
1~8: MD1~8
9~16: SERVO1~8
17~23: TR1~7
*/

extern volatile int16_t CanIoRxData[CAN_IO_SLOT_COUNT];
/*
0~3: MD1~4
4: SERVO1
*/

extern volatile int16_t CanIoTxData[CAN_IO_SLOT_COUNT];
/*
0~2: SW1~3
3~4: ENC1~2
*/
