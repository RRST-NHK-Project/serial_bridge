/*====================================================================
Project: esp32_serial_bridge
Target board: ESP32 Dev Module

Description:
  ROS 2・マイコン間の通信を行うserial_bridgeパッケージのマイコン側プログラム。
  PCから送られてくるバイナリデータを受信、デコードしマイコンのGPIO出力に反映させる。
  config.hppで各種設定をするだけで使用可能です。このファイル(main.cpp)を直接編集しないこと。

Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "pin_ctrl_task.hpp"
#include "serial_task.hpp"
#include <Arduino.h>
// ================= SETUP =================

void setup() {

    // ボーレートは実機テストしながら調整する予定
    Serial.begin(115200);

    delay(200);
    delay(100 * DEVICE_ID); // 安定待ち, IDごとに開始タイミングをずらす

    pinMode(LED, OUTPUT);

    // ready
    for (int i = 0; i < DEVICE_ID; i++) {
        digitalWrite(LED, HIGH);
        delay(50);
        digitalWrite(LED, LOW);
        delay(50);
    }

    // ledcSetup(1, 20000, 8);
    // ledcAttachPin(LED, 1);

    xTaskCreate(
        serialTask,   // タスク関数
        "serialTask", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        10, // 優先度
        NULL);

// モードに応じた初期化
#if defined(MODE_SDM15)
    // 出力モード初期化
    xTaskCreate(
        SDM15_Task,   // タスク関数
        "SDM15_Task", // タスク名
        2048,         // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);

#elif defined(MODE_IR)
    // 赤外線モード初期化
    ;
    xTaskCreate(
        IR_Task,   // タスク関数
        "IR_Task", // タスク名
        2048,      // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);

#elif defined(MODE_HC_SR04)
    // HC-SR04モード初期化
    xTaskCreate(
        HC_SR04_Task,   // タスク関数
        "HC_SR04_Task", // タスク名
        2048,           // スタックサイズ（words）
        NULL,
        11, // 優先度
        NULL);

#elif defined(MODE_ENC)
    // エンコーダモード初期化
    ;

#elif defined(MODE_DEBUG)
    // デバッグモード初期化
    ;

#else
#error "No mode defined. Please define one mode in config.hpp."
#endif

#if (defined(MODE_SDM15) + defined(MODE_ENC) + defined(MODE_IR) + defined(MODE_HC_SR04) + defined(MODE_DEBUG) != 1)
#error "Invalid mode configuration. Please define exactly *one mode* in config.hpp."
#endif
}

// ================= LOOP =================

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    // メインループはなにもしない、処理はすべてFreeRTOSタスクで行う
}
