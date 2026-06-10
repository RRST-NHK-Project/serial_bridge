/*====================================================================
<serial_task.cpp>
・シリアル通信まわりの実装

Frame Structure:
[START_BYTE][DEVICE_ID][LENGTH][DATA...][CHECKSUM]
- START_BYTE: 0xAA
- DEVICE_ID : DEVICE_ID
- LENGTH    : Number of data bytes (Tx16NUM * 2)
- DATA      : int16 data (big-endian)
- CHECKSUM  : XOR of bytes except START_BYTE

START_BYTE（1バイト）
   - フレームの開始を示す固定値（ROS側とマイコンで揃える）
   - 0xAA

ID（1バイト）
   - 宛先デバイスID（マイコンごとに異なる値を設定する）
   - 例: DEVICE_ID

LEN（1バイト）
   - データ部分（DATA）の長さ（バイト単位）
   - 例: Tx16NUM * 2（16bitデータをバイト数に換算）

DATA（可変長）
   - 16bit単位のデータ配列
   - Tx16NUM / Rx16NUM の数だけ送信
   - 1つの16bitデータは
        上位バイト: (data >> 8)
        下位バイト: (data & 0xFF)

CHECKSUM（1バイト）
   - 計算方法: ID ^ LEN ^ DATA[0] ^ DATA[1] ^ ... ^ DATA[n]

Copyright (c) 2026.
====================================================================*/

#include "serial_task.hpp"
#include "config.hpp"
#include "frame_data.hpp"

#include <Arduino.h>

#define START_BYTE 0xAA
static const uint8_t kExpectedRxLen = Rx16NUM * 2;

// ================= TX =================

uint8_t Tx_8Data[1 + 1 + 1 + Tx16NUM * 2 + 1];
// START + ID + LEN + DATA + CHECKSUM

// ================= RX =================

enum RxState {
    WAIT_START,
    WAIT_ID,
    WAIT_LEN,
    WAIT_DATA,
    WAIT_CHECKSUM
};

enum RxState rx_state = WAIT_START;

uint8_t rx_id = 0;
uint8_t rx_len = 0;
uint8_t rx_buf[Rx16NUM * 2];
uint8_t rx_index = 0;
uint8_t rx_checksum = 0;

uint32_t last_rx_ms = 0;
uint32_t last_tx_ms = 0;

// ================= Internal =================

static void send_frame();
static void receive_frame(uint8_t max_bytes);

// ================= Public =================

void serial_task_init() {
    last_tx_ms = millis();
    last_rx_ms = 0;
}

void serial_task_update() {

    // RXは毎ループ呼び出す
    receive_frame((uint8_t)(Rx16NUM * 2 + 4));

    // TXのみ周期管理
    const uint32_t now = millis();
    if (now - last_tx_ms >= TX_PERIOD_MS) {
        send_frame();
        last_tx_ms = now;
    }
}

void serial_task_send_now() {
    send_frame();
    last_tx_ms = millis();
}

uint32_t serial_last_rx_ms() {
    return last_rx_ms;
}

// ================= TX =================

static void send_frame() {

    if (Serial.availableForWrite() < (int)sizeof(Tx_8Data)) {
        return;
    }

    Tx_8Data[0] = START_BYTE;
    Tx_8Data[1] = DEVICE_ID;
    Tx_8Data[2] = Tx16NUM * 2;

    uint8_t checksum = 0;
    checksum ^= Tx_8Data[1];
    checksum ^= Tx_8Data[2];

    for (int i = 0; i < Tx16NUM; i++) {
        Tx_8Data[3 + i * 2] = (uint8_t)((Tx_16Data[i] >> 8) & 0xFF);
        Tx_8Data[3 + i * 2 + 1] = (uint8_t)(Tx_16Data[i] & 0xFF);
        checksum ^= Tx_8Data[3 + i * 2];
        checksum ^= Tx_8Data[3 + i * 2 + 1];
    }

    Tx_8Data[3 + Tx16NUM * 2] = checksum;

    Serial.write(Tx_8Data, sizeof(Tx_8Data));
}

// ================= RX =================

static void receive_frame(uint8_t max_bytes) {

    uint8_t bytes_processed = 0;

    while (bytes_processed < max_bytes && Serial.available()) {

        uint8_t b = (uint8_t)Serial.read();
        bytes_processed++;

        switch (rx_state) {

        case WAIT_START:
            if (b == START_BYTE) {
                rx_state = WAIT_ID;
            }
            break;

        case WAIT_ID:
            if (b == DEVICE_ID) {
                rx_id = b;
                rx_checksum = b;
                rx_state = WAIT_LEN;
            } else {
                rx_state = WAIT_START;
            }
            break;

        case WAIT_LEN:
            rx_len = b;
            rx_checksum ^= b;

            if (rx_len != kExpectedRxLen) {
                rx_state = WAIT_START;
            } else {
                rx_index = 0;
                rx_state = WAIT_DATA;
            }
            break;

        case WAIT_DATA:
            rx_buf[rx_index++] = b;
            rx_checksum ^= b;

            if (rx_index >= rx_len) {
                rx_state = WAIT_CHECKSUM;
            }
            break;

        case WAIT_CHECKSUM:
            if (rx_checksum == b && rx_id == DEVICE_ID) {
                // ===== int16 デコード (big-endian) =====
                for (int i = 0; i < (rx_len / 2) && i < Rx16NUM; i++) {
                    Rx_16Data[i] = (int16_t)((rx_buf[i * 2] << 8) | rx_buf[i * 2 + 1]);
                }

                last_rx_ms = millis();
            }

            rx_state = WAIT_START;
            break;
        }
    }
}
