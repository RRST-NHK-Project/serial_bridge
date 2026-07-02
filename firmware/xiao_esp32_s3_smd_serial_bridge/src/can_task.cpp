/*====================================================================
<can_task.cpp>
・CAN通信（TWAI + MCP2561）を用いた制御データ転送

シリアル通信の 24 スロットを CAN バス上の最大 4 ノードへ
分配し、各ノードが自分の担当スロットだけを受信・送信する。
ノードモードでは、自分の担当スロットを GPIO / サーボへ反映し、
エンコーダ / スイッチの状態をそのスロットへ戻す。
ホストモードでは、シリアルで受け取った 24 スロットを各ノードへ配り、
各ノードから返ってきたデータをまとめてシリアルへ返す。
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"
#include <Arduino.h>
#include <cstring>
#include <driver/twai.h>

constexpr uint32_t CAN_FRAME_ID_BASE = 0x100;
constexpr uint8_t CAN_FRAME_DLC = 8;
constexpr uint8_t CAN_VALUES_PER_FRAME = 4;
constexpr uint8_t CAN_CHUNK_COUNT_PER_NODE = (CAN_SLOTS_PER_NODE + CAN_VALUES_PER_FRAME - 1) / CAN_VALUES_PER_FRAME;
constexpr uint32_t CAN_TX_PERIOD_MS = 5;

static twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
static twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

static uint8_t canFrameNodeIndex(uint32_t identifier) {
    return (uint8_t)((identifier - CAN_FRAME_ID_BASE) / 16U);
}

static uint8_t canFrameChunkIndex(uint32_t identifier) {
    return (uint8_t)((identifier - CAN_FRAME_ID_BASE) % 16U);
}

static void canSendNodeSlotBlock(const int16_t *data, uint8_t node_index) {
    for (uint8_t chunk = 0; chunk < CAN_CHUNK_COUNT_PER_NODE; chunk++) {
        twai_message_t message{};
        message.identifier = CAN_FRAME_ID_BASE + (node_index * 16U) + chunk;
        message.data_length_code = CAN_FRAME_DLC;
        message.flags = 0;
        std::memset(message.data, 0, sizeof(message.data));

        for (uint8_t i = 0; i < CAN_VALUES_PER_FRAME; i++) {
            const uint8_t slot_index = node_index * CAN_SLOTS_PER_NODE + chunk * CAN_VALUES_PER_FRAME + i;
            if (slot_index >= Tx16NUM) {
                break;
            }

            const int16_t value = data[slot_index];
            message.data[i * 2] = (uint8_t)(value >> 8);
            message.data[i * 2 + 1] = (uint8_t)(value & 0xFF);
        }

        (void)twai_transmit(&message, pdMS_TO_TICKS(100));
    }
}

static void canRecvNodeSlotBlock(int16_t *buffer, uint8_t node_index) {
    twai_message_t message{};

    while (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        if (message.data_length_code != CAN_FRAME_DLC) {
            continue;
        }

        const uint8_t frame_node = canFrameNodeIndex(message.identifier);
        if (frame_node != node_index) {
            continue;
        }

        const uint8_t chunk = canFrameChunkIndex(message.identifier);
        if (chunk >= CAN_CHUNK_COUNT_PER_NODE) {
            continue;
        }

        const uint8_t slot_offset = node_index * CAN_SLOTS_PER_NODE + chunk * CAN_VALUES_PER_FRAME;
        for (uint8_t i = 0; i < CAN_VALUES_PER_FRAME; i++) {
            const uint8_t slot_index = slot_offset + i;
            if (slot_index >= Rx16NUM) {
                break;
            }

            const uint8_t byte_index = i * 2;
            const int16_t hi = (int16_t)((uint8_t)message.data[byte_index] << 8);
            const int16_t lo = (int16_t)((uint8_t)message.data[byte_index + 1]);
            buffer[slot_index] = (int16_t)(hi | lo);
        }
    }
}

static void canRecvAllNodeSlotBlocks(int16_t *buffer) {
    twai_message_t message{};

    while (twai_receive(&message, pdMS_TO_TICKS(0)) == ESP_OK) {
        if (message.data_length_code != CAN_FRAME_DLC) {
            continue;
        }

        const uint8_t frame_node = canFrameNodeIndex(message.identifier);
        if (frame_node >= CAN_NODE_COUNT) {
            continue;
        }

        const uint8_t chunk = canFrameChunkIndex(message.identifier);
        if (chunk >= CAN_CHUNK_COUNT_PER_NODE) {
            continue;
        }

        const uint8_t slot_offset = frame_node * CAN_SLOTS_PER_NODE + chunk * CAN_VALUES_PER_FRAME;
        for (uint8_t i = 0; i < CAN_VALUES_PER_FRAME; i++) {
            const uint8_t slot_index = slot_offset + i;
            if (slot_index >= Rx16NUM) {
                break;
            }

            const uint8_t byte_index = i * 2;
            const int16_t hi = (int16_t)((uint8_t)message.data[byte_index] << 8);
            const int16_t lo = (int16_t)((uint8_t)message.data[byte_index + 1]);
            buffer[slot_index] = (int16_t)(hi | lo);
        }
    }
}

static void applyNodeSlotBlockToLocalControl(const int16_t *slot_buffer, uint8_t node_index) {
    const uint8_t slot_offset = node_index * CAN_SLOTS_PER_NODE;

    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t slot_index = slot_offset + i;
        if (slot_index < Rx16NUM) {
            Rx_16Data[1 + i] = slot_buffer[slot_index];
        }
    }

    for (uint8_t i = 0; i < 2; i++) {
        const uint8_t slot_index = slot_offset + 4 + i;
        if (slot_index < Rx16NUM) {
            Rx_16Data[9 + i] = slot_buffer[slot_index];
        }
    }
}

static void buildNodeSlotBlockFromLocalFeedback(int16_t *slot_buffer, uint8_t node_index) {
    const uint8_t slot_offset = node_index * CAN_SLOTS_PER_NODE;

    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t slot_index = slot_offset + i;
        if (slot_index < Tx16NUM) {
            slot_buffer[slot_index] = (i < 2) ? Tx_16Data[1 + i] : Tx_16Data[9 + (i - 2)];
        }
    }

    for (uint8_t i = 0; i < 2; i++) {
        const uint8_t slot_index = slot_offset + 4 + i;
        if (slot_index < Tx16NUM) {
            slot_buffer[slot_index] = Tx_16Data[9 + i];
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
    static int16_t node_slot_buffer[Tx16NUM] = {0};

    while (1) {
#if defined(MODE_CAN_HOST)
        std::memset((void *)Tx_16Data, 0, sizeof(Tx_16Data));
        canRecvAllNodeSlotBlocks((int16_t *)Tx_16Data);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(CAN_TX_PERIOD_MS)) {
            for (uint8_t node_index = 0; node_index < CAN_NODE_COUNT; node_index++) {
                canSendNodeSlotBlock((const int16_t *)Rx_16Data, node_index);
            }
            last_tx = xTaskGetTickCount();
        }
#else
        canRecvNodeSlotBlock(node_slot_buffer, CAN_NODE_INDEX);
        applyNodeSlotBlockToLocalControl(node_slot_buffer, CAN_NODE_INDEX);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(CAN_TX_PERIOD_MS)) {
            buildNodeSlotBlockFromLocalFeedback(node_slot_buffer, CAN_NODE_INDEX);
            canSendNodeSlotBlock(node_slot_buffer, CAN_NODE_INDEX);
            last_tx = xTaskGetTickCount();
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
