/*====================================================================
<pin_ctrl_init.cpp>
・ピン初期化関連の関数実装ファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "driver/pcnt.h"
#include "frame_data.hpp"
#include <Arduino.h>

constexpr uint32_t CTRL_PERIOD_MS = 5; // ピン更新周期（ミリ秒）
