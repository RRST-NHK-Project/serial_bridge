/*====================================================================
<can_task.cpp>
・CAN通信（TWAI + MCP2561）を用いた制御データ転送

本実装では、シリアル通信と同じ int16 配列を CAN フレームに分割して
送受信する。CANノードモードでは、受信した制御データを GPIO/サーボへ
反映し、エンコーダ/スイッチの状態を CAN で送信する。
CANホストモードでは、シリアルで PC と通信しつつ CAN で他マイコンと通信し、
双方向に変換する。
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"
#include <Arduino.h>
#include <driver/twai.h>

constexpr uint32_t CAN_FRAME_ID_BASE = 0x100;
constexpr uint8_t CAN_FRAME_DLC = 8;
constexpr uint8_t CAN_VALUES_PER_FRAME = 4;
constexpr uint8_t CAN_CHUNK_COUNT = (Tx16NUM + CAN_VALUES_PER_FRAME - 1) / CAN_VALUES_PER_FRAME;
constexpr uint32_t CAN_TX_PERIOD_MS = 5;

static twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
static twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

static void canSendFrameChunk(const int16_t *data, uint8_t chunk_index) {
    twai_message_t message{};
    message.identifier = CAN_FRAME_ID_BASE + chunk_index;
    message.data_length_code = CAN_FRAME_DLC;
    message.flags = 0;

    for (int i = 0; i < CAN_FRAME_DLC; i++) {
        message.data[i] = 0;
    }

    for (uint8_t i = 0; i < CAN_VALUES_PER_FRAME; i++) {
        const uint8_t index = chunk_index * CAN_VALUES_PER_FRAME + i;
        if (index >= Tx16NUM) {
            break;
        }

        const int16_t value = data[index];
        message.data[i * 2] = (uint8_t)(value >> 8);
        message.data[i * 2 + 1] = (uint8_t)(value & 0xFF);
    }

    (void)twai_transmit(&message, pdMS_TO_TICKS(100));
}

static void canRecvFrame(int16_t *data) {
    twai_message_t message{};

    while (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        if (message.data_length_code != CAN_FRAME_DLC) {
            continue;
        }

        if (message.identifier < CAN_FRAME_ID_BASE ||
            message.identifier >= CAN_FRAME_ID_BASE + CAN_CHUNK_COUNT) {
            continue;
        }

        const uint8_t chunk_index = (uint8_t)(message.identifier - CAN_FRAME_ID_BASE);
        const uint8_t start_index = chunk_index * CAN_VALUES_PER_FRAME;

        for (uint8_t i = 0; i < CAN_VALUES_PER_FRAME; i++) {
            const uint8_t index = start_index + i;
            if (index >= Rx16NUM) {
                break;
            }

            const uint8_t byte_index = i * 2;
            const int16_t hi = (int16_t)((uint8_t)message.data[byte_index] << 8);
            const int16_t lo = (int16_t)((uint8_t)message.data[byte_index + 1]);
            data[index] = (int16_t)(hi | lo);
        }
    }
}

void canInit() {
    twai_stop();
    twai_driver_uninstall();

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());
}

void canTask(void *) {
    TickType_t last_tx = xTaskGetTickCount();

    while (1) {
#if defined(MODE_CAN_HOST)
        canRecvFrame((int16_t *)Tx_16Data);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(CAN_TX_PERIOD_MS)) {
            const int16_t *payload = (const int16_t *)Rx_16Data;
            for (uint8_t chunk = 0; chunk < CAN_CHUNK_COUNT; chunk++) {
                canSendFrameChunk(payload, chunk);
            }
            last_tx = xTaskGetTickCount();
        }
#else
        canRecvFrame((int16_t *)Rx_16Data);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(CAN_TX_PERIOD_MS)) {
            const int16_t *payload = (const int16_t *)Tx_16Data;
            for (uint8_t chunk = 0; chunk < CAN_CHUNK_COUNT; chunk++) {
                canSendFrameChunk(payload, chunk);
            }
            last_tx = xTaskGetTickCount();
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
