/*====================================================================
<pid_task.hpp>
・PID制御タスク関連のヘッダーファイル
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once

#include <Arduino.h>

// 関数のプロトタイプ宣言
void PID_Task(void *); // PID制御タスク
//void pid_control();
void pid_vel_control();
void md_enc_init();

constexpr uint32_t CTRL_PERIOD_MS = 5; // ピン更新周期（ミリ秒）

//PIDのモード設定
#define Mode_normal
//#define Mode_custom

// 　よく調整する定数集(For Mabuchi 775 motor))
const float max_target_move_cps = 12.5; // 1秒あたりの最大回転数(移動方向)
const float max_target_yaw_cps = 15.0;  // 1秒あたりの最大回転数(回転方向)
const float Kff = 0.0;                  // フィードフォワード（必要に応じて調整するつもりだったけどいらんかッた）
const float Kp = 7.0;                   // P制御//無負荷なら7.5あたり？負荷がかかると8,5でもいいかも
const float Ki = 1.35;                   // I制御
const float Kd = 0.1;                   // D制御(ただしめっちゃ振動するから封印中)
const float filter = 0.2;               // フィルタ係数（小さいほどスムーズらしい）
const float Imax = 45.0;                // I制御の蓄積の上限（必要に応じて調整）
const float motor_limit = 75.0;         // モーターの出力の上限（0~100で）
const int delta_power_limit = 20;       // 出力変化の上限
const float timeout = 1.0;

#if defined(Mode_custom)
//ホイールごとの個別設定（customモード有効時）
const float Kff_[4] = {0.0,0.0,0.0,0.0};
const float Kp_[4] = {0.0,0.0,0.0,0.0};
const float Ki_[4] = {0.0,0.0,0.0,0.0};
const float Kd_[4] = {0.0,0.0,0.0,0.0};
const float filter_[4] = {0.0,0.0,0.0,0.0};
const float Imax_[4] = {0.0,0.0,0.0,0.0};
#endif

extern float target_v[4];
extern float last_target_v[4];
extern float err[4];
extern float last_err[4];
extern float err_diff[4];
extern float err_sum[4];
extern float rps[4];
extern float FF[4];
extern float P[4];
extern float I[4];
extern float D[4];
extern float motor_power[4];

#if (defined(Mode_custom) + defined(Mode_normal)) !=1
#error "Please choose "ONE" mode!!!"
#endif