/*====================================================================
<frame_data.hpp>
・シリアル通信のフレームデータ定義ヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <stdint.h>

#define Tx16NUM 17 // 送信するint16データの数
#define Rx16NUM 24 // 受信するint16データの数

extern volatile int16_t Tx_16Data[Tx16NUM];
/*
0: デバッグ用
1~8: ENC1~8
9~16: SW1~8

MODE_IR時は以下のフォーマットで使用
0: protocol
1: address
2: command
3: flags
4: numberOfBits
5: decodedRawData[15:0]
6: decodedRawData[31:16]
7: decodedRawData[47:32]
8: decodedRawData[63:48]

MODE_HC_SR04時は以下のフォーマットで使用
0: valid (1=有効, 0=タイムアウト)
1: distance[mm]
2: pulse_width[us]
*/

extern volatile int16_t Rx_16Data[Rx16NUM];
/*
0: デバッグ用
1~8: MD1~8
9~16: SERVO1~8
17~23: TR1~7
*/
