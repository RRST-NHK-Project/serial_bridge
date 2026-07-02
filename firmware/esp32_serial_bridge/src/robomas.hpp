/*====================================================================
<robomas.hpp>
・ロボマス関連のヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once

#include "driver/gpio.h"
#include "driver/twai.h"
#include "frame_data.hpp"
#include <Arduino.h>

#include "config.hpp"

#if defined(OLD_BOARD)
#include "old_defs.hpp"
#elif defined(NEW_BOARD)
#include "new_defs.hpp"
#endif

#if (defined(OLD_BOARD) + defined(NEW_BOARD)) != 1
#error "Invalid board definition. Please define *one board* in config.hpp."
#endif


void send_cur_all(float cur_array[NUM_MOTOR]);
void send_cur_c610(float cur_array[NUM_MOTOR]);

// 関数のプロトタイプ宣言
void M3508_Task(void *pvParameters);
void M2006_Task(void *pvParameters);
void M3508_RX(void *);
void robomas_init();

void twai_receive_feedback();
