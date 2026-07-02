/*====================================================================
<config.h>
書き込み前にここでIDと動作モードを設定してください．MDやサーボの設定もここで行います．
MDは基本的に変更不要ですが，サーボは型番、機構に応じて適切に設定する必要があります．
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <Arduino.h>

// ================= 基本設定 =================

// IDの設定，シリアルフレームのDEVICE_IDとして使用します。
// 例: 101, 102, 103, 104
#define DEVICE_ID 101

// CAN_IDは3桁形式で指定します。
// AはCANバス番号、BCはそのバス内の固有デバイス番号です。
// 例: 101, 102, 103, 104
#define CAN_ID 101

// モードの設定，どれか一つをコメントアウト解除すること
// #define MODE_CAN
#define MODE_CAN_HOST
// #define MODE_IO
//  #define MODE_DEBUG

// CANのノード割り当て設定
// 1つのCANバス上で最大4ノードまで対応し、24スロットを4つのノードに分配する
#define CAN_NODE_COUNT 4
#define CAN_SLOTS_PER_NODE 6

#ifndef DEVICE_ID
#error "DEVICE_ID must be defined in config.hpp"
#endif

#if defined(MODE_CAN) || defined(MODE_CAN_HOST)
#ifndef CAN_ID
#error "CAN_ID must be defined in config.hpp for CAN/MODE_CAN_HOST"
#endif

#ifndef CAN_NODE_INDEX
#define CAN_NODE_INDEX ((CAN_ID % 100) - 1)
#endif

#if CAN_ID < 100 || CAN_ID > 999
#error "CAN_ID must be a 3-digit value, e.g. 101..104"
#endif

#if (CAN_ID % 100) < 1 || (CAN_ID % 100) > CAN_NODE_COUNT
#error "CAN_ID bottom two digits must be between 01 and CAN_NODE_COUNT"
#endif

#if DEVICE_ID < 100 || DEVICE_ID > 999
#error "DEVICE_ID must be a 3-digit value, e.g. 101..104"
#endif
#else
#ifndef CAN_NODE_INDEX
#define CAN_NODE_INDEX 0
#endif
#endif

// ================= MD関連 =================

// MD関連の設定，使用するMDに応じて変更
#define MD_PWM_FREQ 20000   // MDのPWM周波数（Hz）
#define MD_PWM_RESOLUTION 8 // MDのPWM分解能（bit）

// ================= サーボ関連 =================

// サーボ関連の設定、使用するサーボに応じて変更
#define SERVO_PWM_FREQ 50       // サーボPWM周波数（Hz）
#define SERVO_PWM_RESOLUTION 14 // サーボPWM分解能（bit）

// サーボの最小・最大パルス幅、角度範囲、初期角度の設定
#define SERVO1_MIN_US 500
#define SERVO1_MAX_US 2500
#define SERVO1_MIN_DEG 0
#define SERVO1_MAX_DEG 270
#define SERVO1_INIT_DEG 0

#define SERVO2_MIN_US 500
#define SERVO2_MAX_US 2500
#define SERVO2_MIN_DEG 0
#define SERVO2_MAX_DEG 270
#define SERVO2_INIT_DEG 0

#define SERVO3_MIN_US 500
#define SERVO3_MAX_US 2500
#define SERVO3_MIN_DEG 0
#define SERVO3_MAX_DEG 270
#define SERVO3_INIT_DEG 0

#define SERVO4_MIN_US 500
#define SERVO4_MAX_US 2500
#define SERVO4_MIN_DEG 0
#define SERVO4_MAX_DEG 270
#define SERVO4_INIT_DEG 0

// ================= 詳細設定（通常は変更不要） =================

// 以下の設定は必要に応じて変更
#define ENABLE_LED 1 // 状態表示LEDを有効にする場合1に設定

// 汎用（mMULTI）ポートの設定（スイッチ:0, サーボ:1）
#define MULTI1 0
#define MULTI2 0
#define MULTI3 0
