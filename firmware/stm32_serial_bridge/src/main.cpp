/*====================================================================
Project: stm32_serial_bridge
Target board: NUCLEO-F446RE / NUCLEO-F767ZI

Description:
  serial_bridge (ROS 2) compatible microcontroller firmware.
  - Receives binary command frames from PC (24 x int16)
  - Applies MD(8ch), Servo(8ch), TR(7ch)
  - Sends feedback frames back to PC (17 x int16)
    ENC(8ch), SW(8ch)

  Configure DEVICE_ID / mode in config.hpp.
  Configure pins / constants in defs.hpp.
====================================================================*/

#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"
#include "io.hpp"
#include "serial_frame.hpp"

#include <Arduino.h>

namespace {
    uint32_t last_tx_ms = 0;
}

void setup() {
    // Some Nucleo setups benefit from a short stabilization delay.
    delay(2000);

    Serial.begin(SERIAL_BAUD);

    pinMode(BUILTIN_LED, OUTPUT);

    for (int i = 0; i < Tx16NUM; i++)
        Tx_16Data[i] = 0;
    for (int i = 0; i < Rx16NUM; i++)
        Rx_16Data[i] = 0;

    io_init();

    last_tx_ms = millis();
}

void loop() {
    receive_frame();
    io_update();

    const uint32_t now = millis();
    if (now - last_tx_ms >= TX_PERIOD_MS) {
        send_frame();
        last_tx_ms = now;
    }
}
