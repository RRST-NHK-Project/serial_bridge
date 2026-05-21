/*====================================================================
<config.h>
書き込み前にここでIDと動作モードを設定してください．MDやサーボの設定もここで行います．
MDは基本的に変更不要ですが，サーボは型番、機構に応じて適切に設定する必要があります．
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#pragma once
#include <Arduino.h>

// ================= 基本設定 =================

// IDの設定，ROS側からマイコンを識別するために使用，すべてのマイコンで異なる値にすること
#define DEVICE_ID 0x00

// モードの設定，どれか一つをコメントアウト解除する
#define MODE_SDM15
// #define MODE_ENC
// #define MODE_IR
// #define MODE_HC_SR04
// #define MODE_DEBUG

// ================= 高度な設定（通常は変更不要） =================

// 以下の設定は必要に応じて変更
#define ENABLE_LED 1 // 状態表示LEDを有効にする場合1に設定

// SDM15 UART設定
// ボード配線に合わせて変更すること
// XIAO ESP32S3 では GPIO3 はストラップピンのため外部機器接続時に起動/通信不安定の原因になりやすい
// 既定は UART に使いやすい GPIO43(TX) / GPIO44(RX) を使用する
#define SDM15_UART_TX_PIN 4
#define SDM15_UART_RX_PIN 3

// IR受信モード設定
// 使用するIRレシーバの信号線を接続するGPIO番号
#define IR_RECEIVE_PIN D10
// IR受信タスクの実行周期（ミリ秒）
#define IR_TASK_PERIOD_MS 5

// メーカー(=IRプロトコル)のフィルタ設定
// 0: 全プロトコルを受信, 1: 指定プロトコルのみ受信
#define IR_FILTER_ENABLE 0
// IR_FILTER_ENABLE=1 のときに有効
// 例: SONY, NEC, PANASONIC, JVC, SAMSUNG, RC5, RC6
#define IR_FILTER_PROTOCOL LG

// HC-SR04モード設定
// Trig/Echoを接続するGPIO番号
#define HC_SR04_TRIG_PIN D2
#define HC_SR04_ECHO_PIN D1
// 計測周期（ミリ秒）
#define HC_SR04_TASK_PERIOD_MS 60
// pulseIn() タイムアウト（マイクロ秒）
// 30ms は約5m相当の往復時間
#define HC_SR04_TIMEOUT_US 30000UL