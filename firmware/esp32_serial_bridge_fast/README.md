# esp32_serial_bridge_fast

Fast M2006 velocity-control firmware for serial_bridge (ESP32 Dev Module).

## Slots (int16)

- RX (PC -> MCU):
  - [5..8] target rpm motor1..4

- TX (MCU -> PC):
  - [5..8] measured rpm motor1..4
  - [13..16] measured angle [deg] motor1..4

## Control

- Control loop: 1kHz (esp_timer)
- CAN: TWAI 1Mbps (pins TX=4, RX=2)

## Build

```bash
cd /home/dev/ros2_ws/src/serial_bridge/firmware/esp32_serial_bridge_fast
pio run -e esp32dev -t upload
```
