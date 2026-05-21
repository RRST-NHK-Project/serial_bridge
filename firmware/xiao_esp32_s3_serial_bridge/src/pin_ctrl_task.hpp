/*====================================================================
<pin_ctrl_task.hpp>
・ピン操作関連の関数とタスクのヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once

#include <Arduino.h>

// 関数のプロトタイプ宣言
void SDM15_Task(void *);
void IR_Task(void *);
void HC_SR04_Task(void *);