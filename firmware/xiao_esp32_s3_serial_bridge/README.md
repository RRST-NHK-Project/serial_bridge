# xiao_esp32_s3_serial_bridge

## 1. Overview

xiao_esp32_s3_serial_bridge is a sensor-focused firmware for Seeed XIAO ESP32S3 boards used with the serial_bridge ROS 2 package.
It receives and sends binary serial frames, then runs one selected sensor task mode.

Unlike the ESP32 actuator firmware variants, this project is primarily for sensor acquisition and protocol bridging.

---

## 2. Target Hardware

| Item | Description |
|:---|:---|
| MCU | Seeed XIAO ESP32S3 |
| Framework | Arduino (PlatformIO) |
| Transport | USB Serial |
| Typical use | SDM15, IR, ultrasonic, and simple encoder input bridging |

---

## 3. Mode Comparison (Actuators and Sensors)

Select exactly one mode in src/config.hpp.

| Mode | Purpose | DC motor | Servo | TR output | RoboMaster motor | ENC | SW | Other sensors |
|:---|:---|---:|---:|---:|---:|---:|---:|:---|
| MODE_SDM15 | SDM15 distance sensor stream bridge | 0 | 0 | 0 | 0 | 0 | 0 | SDM15: 1 stream |
| MODE_ENC | Encoder mode slot (reserved/implementation-specific) | 0 | 0 | 0 | 0 | 1 | 0 | - |
| MODE_IR | IR receiver protocol bridge | 0 | 0 | 0 | 0 | 0 | 0 | IR receiver: 1 |
| MODE_HC_SR04 | Ultrasonic distance sensor bridge | 0 | 0 | 0 | 0 | 0 | 0 | HC-SR04: 1 |
| MODE_DEBUG | Development/debug mode | - | - | - | - | - | - | project-specific |

Summary:
- Available actuator channels are 0 in all current modes.
- This firmware should be treated as a sensor endpoint, not an actuator controller.

---

## 4. Data Layout (int16)

Base frame arrays are shared with other serial_bridge MCU projects.
Some modes define custom TX payload semantics:

- MODE_SDM15:
  - TX[0]: distance
  - TX[1]: intensity
  - TX[2]: disturb
- MODE_IR:
  - TX[0..8]: protocol, address, command, flags, bit count, and split raw data
- MODE_HC_SR04:
  - TX[0]: valid flag
  - TX[1]: distance [mm]
  - TX[2]: pulse width [us]

---

## 5. Configuration Workflow

1. Open src/config.hpp.
2. Set DEVICE_ID.
3. Enable exactly one sensor mode macro.
4. Configure mode-specific pins and timing constants.
5. Build and flash with PlatformIO.

---

## 6. Compatibility with serial_bridge (ROS 2)

This firmware is compatible with the serial_bridge ROS 2 node when payload meaning is interpreted per active mode.

Keep synchronized:
- DEVICE_ID
- Active mode on firmware side
- TX payload decoding logic on ROS 2 side

---

## 7. Credits

Developed by NHK Project, RRST, Ritsumeikan University, Japan.
- Official Website: https://www.rrst.jp
- X (Twitter): https://x.com/RRST_BKC

![Logo](https://www.rrst.jp/img/logo.png)
