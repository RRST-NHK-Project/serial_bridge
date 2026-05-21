#include "serial_frame.hpp"

#include "config.hpp"
#include "frame_data.hpp"

#include <Arduino.h>

namespace {
    constexpr uint8_t kStartByte = 0xAA;

    enum class RxState : uint8_t {
        WaitStart,
        WaitId,
        WaitLen,
        WaitData,
        WaitChecksum,
    };

    RxState rx_state = RxState::WaitStart;

    uint8_t rx_id = 0;
    uint8_t rx_len = 0;
    uint8_t rx_buf[Rx16NUM * 2] = {0};
    uint8_t rx_index = 0;
    uint8_t rx_checksum = 0;
}

void send_frame() {
    static uint8_t tx_frame[1 + 1 + 1 + Tx16NUM * 2 + 1];

    tx_frame[0] = kStartByte;
    tx_frame[1] = DEVICE_ID;
    tx_frame[2] = Tx16NUM * 2;

    uint8_t checksum = 0;
    checksum ^= tx_frame[1];
    checksum ^= tx_frame[2];

    for (int i = 0; i < Tx16NUM; i++) {
        const int16_t v = Tx_16Data[i];
        tx_frame[3 + i * 2] = static_cast<uint8_t>((v >> 8) & 0xFF);
        tx_frame[3 + i * 2 + 1] = static_cast<uint8_t>(v & 0xFF);
        checksum ^= tx_frame[3 + i * 2];
        checksum ^= tx_frame[3 + i * 2 + 1];
    }

    tx_frame[3 + Tx16NUM * 2] = checksum;

    Serial.write(tx_frame, sizeof(tx_frame));
}

void receive_frame() {
    while (Serial.available() > 0) {
        const uint8_t b = static_cast<uint8_t>(Serial.read());

        switch (rx_state) {
        case RxState::WaitStart:
            if (b == kStartByte) {
                rx_state = RxState::WaitId;
            }
            break;

        case RxState::WaitId:
            rx_id = b;
            rx_checksum = b;
            rx_state = RxState::WaitLen;
            break;

        case RxState::WaitLen:
            rx_len = b;
            rx_checksum ^= b;

            if (rx_len > Rx16NUM * 2) {
                rx_state = RxState::WaitStart;
            } else {
                rx_index = 0;
                rx_state = RxState::WaitData;
            }
            break;

        case RxState::WaitData:
            rx_buf[rx_index++] = b;
            rx_checksum ^= b;

            if (rx_index >= rx_len) {
                rx_state = RxState::WaitChecksum;
            }
            break;

        case RxState::WaitChecksum:
            if (rx_checksum == b && rx_id == DEVICE_ID) {
                const uint8_t data16_count = static_cast<uint8_t>(rx_len / 2);
                for (uint8_t i = 0; i < data16_count && i < Rx16NUM; i++) {
                    Rx_16Data[i] = static_cast<int16_t>((rx_buf[i * 2] << 8) | rx_buf[i * 2 + 1]);
                }
            }
            rx_state = RxState::WaitStart;
            break;
        }
    }
}
