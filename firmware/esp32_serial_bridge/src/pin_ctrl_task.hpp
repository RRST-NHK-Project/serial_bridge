/*====================================================================
<pin_ctrl_task.hpp>
・ピン操作関連の関数とタスクのヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once

#include <Arduino.h>

// 関数のプロトタイプ宣言
void Input_Task(void *);      // 入力タスク
void Output_Task(void *);     // 出力タスク
void IO_Task(void *);         // 入出力タスク
void ROBOMAS_IO_Task(void *); // ロボマス入出力タスク
void Omni_IO_Task(void *);    // 4輪オムニ用入出力タスク
void NATSU_ID2_Task(void *);  // 夏ロボID2用入出力タスク
void NATSU_ID4_Task(void *);  // 夏ロボID4専用タスク(MD1 + WS2812Bテープ)
void MD_Output();
void Servo_Output();
void TR_Output();
void ENC_Input();
void SW_Input();
void IO_MD_Output();
void IO_TR_Output();
void IO_ENC_Input();
void IO_SW_Input();
void ROBOMAS_IO_ENC_Input();
void ROBOMAS_IO_SW_Input();
void OMNI_IO_TR_Output(); //一応ピンが干渉していないTRを使えるようにしておく
void NATSU_ID2_ENC_Input();
void NATSU_ID2_TR_Output();
