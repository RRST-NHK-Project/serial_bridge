# stm32_serial_bridge

> **WIP**: This firmware is work-in-progress. Pin assignment, timing, and behavior may change.
> Treat it as a reference / development firmware and validate on your hardware before competition use.

## 1. Overview

stm32_serial_bridge is an STM32 firmware compatible with the `serial_bridge` ROS 2 package.
It implements the **maximum slot layout expected by the ROS side**:

- RX (PC → MCU, 24 × int16):
  - MD1..8 (slots 1..8)
  - SERVO1..8 (slots 9..16)
  - TR1..7 (slots 17..23)
- TX (MCU → PC, 17 × int16):
  - ENC1..8 (slots 1..8)
  - SW1..8 (slots 9..16)

This project targets Nucleo boards:
- NUCLEO-F446RE
- NUCLEO-F767ZI (allowed if F446RE pinout is insufficient)

---

## 2. Target Hardware

| Item | Description |
|:---|:---|
| MCU | STM32 Nucleo (F446RE / F767ZI) |
| Framework | Arduino (PlatformIO / STM32duino) |
| Transport | USB Serial (`/dev/ttyACM*`) |
| Baud rate | 115200 bps |

---

## 3. Configuration

- `src/config.hpp`: `DEVICE_ID`, mode selection
- `src/defs.hpp`: pins and constant values (PWM freq/resolution, servo pulse ranges, etc.)

Notes:
- This firmware does not implement motor FOC control. `askuric/Simple FOC` is used only as a convenient quadrature encoder decoder (`Encoder` class).
- Servo output uses `Servo.h` when available (preferred). If `<Servo.h>` is not found in your toolchain, it falls back to generating servo PWM by `analogWrite` duty.
- Some default pins (notably `PD*`) may not be routed to headers on your specific Nucleo variant. If so, reassign pins in `src/defs.hpp`.

---

## 4. Default Pin Assignment

Default pins are defined in `src/defs.hpp`.

### MD (PWM + DIR)

| Channel | PWM | DIR |
|:---:|:---:|:---:|
| MD1 | PB6 | PB0 |
| MD2 | PB7 | PB1 |
| MD3 | PB8 | PB2 |
| MD4 | PB9 | PB3 |
| MD5 | PA8 | PC0 |
| MD6 | PA9 | PC1 |
| MD7 | PA10 | PC2 |
| MD8 | PA11 | PC3 |

### Servo

| Channel | Pin |
|:---:|:---:|
| SERVO1 | PA0 |
| SERVO2 | PA1 |
| SERVO3 | PA2 |
| SERVO4 | PA3 |
| SERVO5 | PB4 |
| SERVO6 | PB5 |
| SERVO7 | PA6 |
| SERVO8 | PA7 |

### TR output

| Channel | Pin |
|:---:|:---:|
| TR1 | PD0 |
| TR2 | PD1 |
| TR3 | PD2 |
| TR4 | PD3 |
| TR5 | PD4 |
| TR6 | PD5 |
| TR7 | PD6 |

### Encoder (quadrature A/B)

| Channel | A | B |
|:---:|:---:|:---:|
| ENC1 | PC4 | PC5 |
| ENC2 | PC6 | PC7 |
| ENC3 | PC8 | PC9 |
| ENC4 | PC10 | PC11 |
| ENC5 | PA4 | PA5 |
| ENC6 | PB12 | PB13 |
| ENC7 | PB14 | PB15 |
| ENC8 | PB10 | PB11 |

### SW input

| Channel | Pin |
|:---:|:---:|
| SW1 | PD7 |
| SW2 | PD8 |
| SW3 | PD9 |
| SW4 | PD10 |
| SW5 | PD11 |
| SW6 | PD12 |
| SW7 | PD13 |
| SW8 | PD14 |

---

## 5. Build & Upload

```bash
cd ~/ros2_ws/src/serial_bridge/stm32_serial_bridge
pio run -e nucleo_f446re -t upload
# or
pio run -e nucleo_f767zi -t upload
```

