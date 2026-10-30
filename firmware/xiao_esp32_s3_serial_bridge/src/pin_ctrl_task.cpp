/*====================================================================
<pin_ctrl_task.cpp>
・ピン操作関連の関数とタスクの実装ファイル
Copyright (c) 2025 RRST-NHK-Project
SPDX-License-Identifier: MIT
====================================================================*/

#include "SDM15.h"
#include "defs.hpp"
#include "driver/pcnt.h"
#include "frame_data.hpp"
#include "pin_ctrl_init.hpp"
#include <Arduino.h>

constexpr uint32_t CTRL_PERIOD_MS = 1; // ピン更新周期（ミリ秒）

void SDM15_Task(void *);
void HC_SR04_Task(void *);

HardwareSerial SerialSDM(1);
SDM15 sdm15(SerialSDM);

#if defined(MODE_HC_SR04)
namespace {
    constexpr float SOUND_SPEED_CM_PER_US = 0.0343f;
}
#endif

// ================= TASK =================

void SDM15_Task(void *pvParameters) {

    (void)pvParameters;

    // HardwareSerial.begin() は (baud, config, rxPin, txPin) の順序
    SerialSDM.begin(460800, SERIAL_8N1, SDM15_RX, SDM15_TX);
    SerialSDM.setTimeout(20);

    // 高速通信対策
    SerialSDM.setRxBufferSize(2048);

    vTaskDelay(pdMS_TO_TICKS(1000)); // 安定待ち

    sdm15.StartScan();

    while (1) {
        // 溜まっているフレームを読み切り、最後の有効値だけを反映する
        bool has_latest = false;
        ScanData latest_data = {};

        while (SerialSDM.available() >= 9) {
            ScanData data = sdm15.GetScanData();
            if (!data.checksum_error) {
                latest_data = data;
                has_latest = true;
            }
        }

        if (has_latest) {
            // 受信データを送信用配列に格納（常に最新値のみ）
            Tx_16Data[0] = int16_t(latest_data.distance);
            Tx_16Data[1] = int16_t(latest_data.intensity);
            Tx_16Data[2] = int16_t(latest_data.disturb);
        }

        vTaskDelay(pdMS_TO_TICKS(CTRL_PERIOD_MS));
    }
}

void HC_SR04_Task(void *pvParameters) {
#if defined(MODE_HC_SR04)
    (void)pvParameters;

    pinMode(HC_SR04_TRIG, OUTPUT);
    pinMode(HC_SR04_ECHO, INPUT);
    digitalWrite(HC_SR04_TRIG, LOW);

    while (1) {
        // Trigに10usパルスを与えて計測開始
        digitalWrite(HC_SR04_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(HC_SR04_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(HC_SR04_TRIG, LOW);

        const uint32_t duration_us =
            pulseIn(HC_SR04_ECHO, HIGH, HC_SR04_TIMEOUT_US);

        if (duration_us == 0) {
            // タイムアウト時は有効フラグを0にする
            Tx_16Data[0] = 0;
            Tx_16Data[1] = 0;
            Tx_16Data[2] = 0;
        } else {
            const float distance_cm =
                (duration_us * SOUND_SPEED_CM_PER_US) * 0.5f;
            const int16_t distance_mm = (int16_t)(distance_cm * 10.0f);

            // HC-SR04フォーマット
            // 0: 有効フラグ(1)
            // 1: 距離[mm]
            // 2: エコーパルス幅[us]
            Tx_16Data[0] = 1;
            Tx_16Data[1] = distance_mm;
            Tx_16Data[2] = (int16_t)duration_us;
        }

        vTaskDelay(pdMS_TO_TICKS(HC_SR04_TASK_PERIOD_MS));
    }
#else
    (void)pvParameters;
    vTaskDelete(NULL);
#endif
}
