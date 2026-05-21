#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"

#include <Arduino.h>

#define START_BYTE 0xAA
#define ENABLE_LOOPBACK 0

// ================= TX =================
static uint8_t Tx_8Data[1 + 1 + 1 + Tx16NUM * 2 + 1];

// ================= RX =================
static uint8_t Rx_raw_frame[1 + 1 + 1 + Rx16NUM * 2 + 1];
static uint8_t Rx_raw_len = 0;

enum RxState {
    WAIT_START,
    WAIT_ID,
    WAIT_LEN,
    WAIT_DATA,
    WAIT_CHECKSUM
};

static RxState rx_state = WAIT_START;
static uint8_t rx_id = 0;
static uint8_t rx_len = 0;
static uint8_t rx_buf[Rx16NUM * 2];
static uint8_t rx_index = 0;
static uint8_t rx_checksum = 0;

void serialTask(void *) {
    TickType_t last_tx = xTaskGetTickCount();

    while (1) {
        receive_frame();

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(TX_PERIOD_MS)) {
            send_frame();
            last_tx = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void send_frame() {
    Tx_8Data[0] = START_BYTE;
    Tx_8Data[1] = DEVICE_ID;
    Tx_8Data[2] = Tx16NUM * 2;

    uint8_t checksum = 0;
    checksum ^= Tx_8Data[1];
    checksum ^= Tx_8Data[2];

    for (int i = 0; i < Tx16NUM; i++) {
        Tx_8Data[3 + i * 2] = static_cast<uint8_t>(Tx_16Data[i] >> 8);
        Tx_8Data[3 + i * 2 + 1] = static_cast<uint8_t>(Tx_16Data[i] & 0xFF);
        checksum ^= Tx_8Data[3 + i * 2];
        checksum ^= Tx_8Data[3 + i * 2 + 1];
    }

    Tx_8Data[3 + Tx16NUM * 2] = checksum;

    Serial.write(Tx_8Data, sizeof(Tx_8Data));
}

void receive_frame() {
    while (Serial.available()) {
        uint8_t b = static_cast<uint8_t>(Serial.read());

        switch (rx_state) {
        case WAIT_START:
            if (b == START_BYTE) {
                rx_state = WAIT_ID;
            }
            break;

        case WAIT_ID:
            rx_id = b;
            rx_checksum = b;
            rx_state = WAIT_LEN;
            break;

        case WAIT_LEN:
            rx_len = b;
            rx_checksum ^= b;

            if (rx_len > Rx16NUM * 2) {
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
                if (ENABLE_LED) {
                    static uint32_t last_led_toggle_ms = 0;
                    const uint32_t now = millis();
                    if (now - last_led_toggle_ms >= 500) {
                        digitalWrite(LED, !digitalRead(LED));
                        last_led_toggle_ms = now;
                    }
                }

                Rx_raw_frame[0] = START_BYTE;
                Rx_raw_frame[1] = rx_id;
                Rx_raw_frame[2] = rx_len;
                for (int i = 0; i < rx_len; i++) {
                    Rx_raw_frame[3 + i] = rx_buf[i];
                }
                Rx_raw_frame[3 + rx_len] = b;
                Rx_raw_len = 1 + 1 + 1 + rx_len + 1;

                for (int i = 0; i < rx_len / 2; i++) {
                    Rx_16Data[i] = static_cast<int16_t>((rx_buf[i * 2] << 8) | rx_buf[i * 2 + 1]);
                }

#if ENABLE_LOOPBACK
                Serial.write(Rx_raw_frame, Rx_raw_len);
#endif
            }

            rx_state = WAIT_START;
            break;
        }
    }
}
