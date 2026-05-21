/*====================================================================
Project: arduino_uno_serial_bridge
Target board: Arduino Uno

Description:
  serial_bridge (ROS 2) compatible microcontroller firmware.
  - Receives binary command frames from PC (24 x int16)
  - Applies Servo output
  - Sends feedback frames back to PC (17 x int16)

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
    Serial.begin(SERIAL_BAUD);

    // USB CDC stabilization wait
    delay(200);

    // Clear buffers (optional)
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
