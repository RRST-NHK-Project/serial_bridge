# arduino_uno_serial_bridge

> **WIP**: This firmware is work-in-progress. Pin assignment, timing, and behavior may change.
> Treat it as a reference / development firmware and validate on your hardware before competition use.

## 1. Overview

arduino_uno_serial_bridge is an Arduino Uno firmware compatible with the `serial_bridge` ROS 2 package.
It periodically sends a TX frame (MCU → PC) so that `serial_bridge` can auto-detect the device by `DEVICE_ID`,
and it receives RX frames (PC → MCU) to apply actuator commands.

This Uno version currently implements a minimal **IO mode**:
- **Servo output** (6 channels)
- **Switch input** (up to 8 channels by default)

---

## 2. Target Hardware

| Item | Description |
|:---|:---|
| MCU | Arduino Uno (ATmega328P) |
| Framework | Arduino (PlatformIO) |
| Transport | USB Serial (`/dev/ttyACM*`) |
| Baud rate | 115200 bps |

---

## 3. Pin Assignment (default)

| Signal | Arduino Pin | Notes |
|:---|:---:|:---|
| SERVO1 | D9 | `Servo.h` output |
| SERVO2 | D10 | `Servo.h` output |
| SERVO3 | D11 | `Servo.h` output |
| SERVO4 | D6 | `Servo.h` output |
| SERVO5 | D5 | `Servo.h` output |
| SERVO6 | D3 | `Servo.h` output |
| SW1 | D2 | `INPUT_PULLUP` (active-low) |
| SW2 | D4 | `INPUT_PULLUP` (active-low) |
| SW3 | D7 | `INPUT_PULLUP` (active-low) |
| SW4 | D8 | `INPUT_PULLUP` (active-low) |
| SW5 | A0 | `INPUT_PULLUP` (active-low) |
| SW6 | A1 | `INPUT_PULLUP` (active-low) |
| SW7 | A2 | `INPUT_PULLUP` (active-low) |
| SW8 | A3 | `INPUT_PULLUP` (active-low) |

Change pins in `src/defs.hpp`.

---

## 4. Data Layout (int16 slots)

This firmware uses the same frame format as `serial_bridge`.

### PC → MCU (24 × int16)
- Slot 9..14 (`Rx_16Data[9..14]`): SERVO1..SERVO6 angle in degrees
- Other slots: currently ignored

### MCU → PC (17 × int16)
- Slot 9..16 (`Tx_16Data[9..16]`): SW1..SW8 (0 or 1)
- Other slots: set to 0

---

## 5. Configuration

Edit `src/config.hpp`:
- `DEVICE_ID` (must be unique among connected MCUs)
- Mode macro (currently `MODE_IO`)

Edit `src/defs.hpp`:
- Pins (SERVO/SW)
- Period constants (`TX_PERIOD_MS`, `SERIAL_BAUD`)
- Servo range (`SERVO1_MIN_DEG`, `SERVO1_MAX_DEG`, `*_US`)
- Switch polarity

---

## 6. Build & Upload (PlatformIO)

```bash
cd ~/ros2_ws/src/serial_bridge/arduino_uno_serial_bridge
pio run -e uno -t upload
```

Serial monitor:

```bash
pio device monitor -b 115200
```
