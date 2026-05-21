/*====================================================================
<frame_data.cpp>
・シリアル通信のフレームデータ定義ファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "frame_data.hpp"

volatile int16_t Tx_16Data[Tx16NUM] = {0}; // 送信用データ配列

volatile int16_t Rx_16Data[Rx16NUM] = {0}; // 受信用データ配列
