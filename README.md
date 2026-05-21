# **serial_bridge**

> NOTE: Microcontroller projects have been moved into the `firmware/` directory
> (e.g. `firmware/esp32_serial_bridge`, `firmware/stm32_serial_bridge`) to
> clearly separate MCU-side code from ROS-side code.

> 日本語版: [README_ja.md](README_ja.md)

## 1. Overview

`serial_bridge` is a ROS 2 package that provides a bidirectional bridge between ROS 2 nodes and microcontrollers via serial communication.  
It automatically scans available serial ports, identifies connected microcontrollers by their `DEVICE_ID` embedded in each frame, and manages multiple MCU connections simultaneously.

This package enables real-time control of motors, servos, and solenoid valves on the robot, as well as acquisition of sensors such as encoders, microswitches, and distance sensors.

---

## 2. System Requirements

| Item | Description |
|:---|:---|
| OS | Ubuntu 24.04 LTS |
| ROS | ROS 2 Jazzy |
| Baud Rate | 115200 bps |
| Hardware | PC connected to MCU via USB or UART |

> **Note**:  
> Ensure the user belongs to the `dialout` group for `/dev/ttyUSB*` or `/dev/ttyACM*` access.  
> `sudo usermod -aG dialout $USER` (re-login required)

---

## 3. Features

- Automatic serial port scanning — no manual port assignment required.
- Hot-plug support via dynamic node creation: MCUs can be connected or disconnected at any time and are automatically detected and recovered.
- Multi-MCU support: each MCU is identified by `DEVICE_ID` in the frame, independent of USB port order.
- Custom binary frame protocol with `int16` data arrays (24 TX / 24 RX slots).
- XOR checksum-based frame validation ensures communication reliability.
- Automatic reconnection on disconnect or RX timeout.
- Three log output modes: terminal text, graphical bar, or silent. Default is graphical mode.
- MCU firmware included (PlatformIO): ESP32 + Arduino Uno (WIP) + STM32 Nucleo (WIP).

---

## 4. System Configuration

### 4.1 Overview Diagram

```
  [Any ROS 2 node]
       │  serial_tx_[ID]  (Publish)
       │  serial_rx_[ID]  (Subscribe)
       │
  [serial_bridge node]
    ├─ port_scanner  ─── scans /dev/ttyUSB*, /dev/ttyACM*
    │                    identifies MCUs by DEVICE_ID
    │                    spawns a bridge_node on detection
    │
    ├─ bridge_node (ID=6)  ── /dev/ttyUSB0 ──► MCU (ID=6)   Motor / Servo / TR
    ├─ bridge_node (ID=7)  ── /dev/ttyUSB1 ──► MCU (ID=7)   Encoder / Switch
    └─ bridge_node (ID=N)  ── ...          ──► MCU (ID=N)   ...
```

### 4.2 File Roles

| Component | Role |
|:---|:---|
| `main.cpp` | Initializes the ROS executor, reads parameters, starts the scanner thread |
| `port_scanner` | Probes all `/dev/ttyUSB*` and `/dev/ttyACM*` ports (excluding the list), sends a probe frame, and returns a map of `DEVICE_ID → port` |
| `bridge_node` | One instance per MCU — runs a 5 ms timer, reads serial RX, publishes `serial_rx_[ID]`; receives `serial_tx_[ID]` and writes to the serial port |

### 4.3 Hot-Plug and Reconnection Flow

```
[MCU plugged in]
    └─► port_scanner detects valid frame on /dev/ttyUSBx
            └─► SerialBridgeNode created and added to executor
                    └─► serial_rx_[ID] / serial_tx_[ID] topics become active

[MCU unplugged / RX timeout]
    └─► bridge_node detects EIO / ENODEV / ENXIO, or rx_timeout_sec elapsed
            └─► port closed, connected_ = false
                    └─► bridge_node retries open every reconnect_interval_sec
                    └─► port_scanner also re-detects on next scan cycle
                            └─► if found on a different port: node replaced
```

> **Tips**: MCU identity is determined by `DEVICE_ID` in the frame, not by USB port number.  
> Plugging into a different USB port is handled transparently — the same `serial_rx_[ID]` / `serial_tx_[ID]` topics remain valid.

### 4.4 Communication Flow (bidirectional)

```
ROS 2 (serial_bridge)                      MCU

  serial_tx_[ID] received
       │
       ▼
  TX frame assembled
  [0xAA][ID][LEN][DATA...][CHK]
       │
       └─────────────────────────────────► received via serial
                                           └─ commands applied to motor/servo

                                           feedback sent
                                           [0xAA][ID][LEN][DATA...][CHK]
       ◄─────────────────────────────────┘
  RX frame received
       │
       ├─ START_BYTE sync
       ├─ CHECKSUM validation
       └─► published as serial_rx_[ID]
```

---

## 5. Frame Format

```
[0]     START_BYTE  : 0xAA
[1]     DEVICE_ID   : unique ID configured per MCU (uint8)
[2]     LENGTH      : payload length in bytes (= num_int16 × 2)
[3..N]  DATA        : int16 array, big-endian (high byte first)
[N+1]   CHECKSUM    : XOR of bytes [1]..[N]
```

### Default data slot count (configurable in `config.hpp`)

| Direction | Slots | Bytes |
|:---|---:|---:|
| PC → MCU (TX) | 24 × int16 | 48 |
| MCU → PC (RX) | 24 × int16 | 48 |

### Typical TX slot usage (esp32_serial_bridge)

| Slot | Usage |
|---:|:---|
| 1–8 | DC motor commands |
| 9–16 | RC servo commands |
| 17–23 | TR (transistor/solenoid) outputs |

### Typical RX slot usage

| Slot | Usage |
|---:|:---|
| 1–8 | Encoder values |
| 9–16 | Switch / sensor inputs |
| 17–23 | Reserved |

---

## 6. ROS 2 Interfaces

### Subscribed Topics (ROS → MCU)

| Topic | Type | Description |
|:---|:---|:---|
| `serial_tx_[DEVICE_ID]` | `std_msgs/msg/Int16MultiArray` | Control commands sent to the MCU |

Publish to this topic from any ROS 2 node to have serial_bridge forward the commands to the MCU.

### Published Topics (MCU → ROS)

| Topic | Type | Description |
|:---|:---|:---|
| `serial_rx_[DEVICE_ID]` | `std_msgs/msg/Int16MultiArray` | Encoder values, sensor data from the MCU |

Subscribe to this topic from any ROS 2 node to receive and use sensor feedback.

---

## 7. Parameters (`config/serial_bridge.yaml`)

| Parameter | Default | Description |
|:---|:---|:---|
| `excluded_ports` | `["/dev/ttyUSB0"]` | Ports excluded from scanning (e.g. LiDAR, GPS) |
| `rx_timeout_sec` | `2.0` | Seconds without RX before closing the port |
| `reconnect_interval_sec` | `3.0` | Minimum wait between reconnect attempts |
| `scan_interval_ms` | `5000` | Interval between port scan cycles (ms) |

Edit `config/serial_bridge.yaml` to change these values without rebuilding.

---

## 8. Getting Started

### 8.1 Clone

```bash
cd ~/ros2_ws/src
git clone https://github.com/RRST-NHK-Project/serial_bridge.git
```

### 8.2 Build

```bash
cd ~/ros2_ws
colcon build --packages-select serial_bridge
source install/setup.bash
```

### 8.3 Launch

```bash
ros2 launch serial_bridge serial_bridge.launch.py
```

The node starts scanning ports immediately. Connected MCUs appear in the log as they are detected.

---

## 9. Log Output Modes

Configured at compile time in `include/serial_bridge/config.hpp`:

| Mode | Description |
|:---|:---|
| `kTerminal` | Standard ROS 2 logger output |
| `kGraphical` | Bar graph per MCU (RX rate, bandwidth, utilization) |
| `kNone` | Silent — no terminal output |

---

## 10. Directory Structure

| Path | Description |
|:---|:---|
| `src/` | ROS 2 node source (`bridge_node`, `port_scanner`, `main`) |
| `include/serial_bridge/` | Headers (`config.hpp`, `bridge_node.hpp`, `port_scanner.hpp`, `graphical_ui.hpp`) |
| `config/` | ROS 2 parameter file (`serial_bridge.yaml`) |
| `launch/` | Launch file (`serial_bridge.launch.py`) |
| `esp32_serial_bridge/` | MCU firmware (PlatformIO) |
| `arduino_uno_serial_bridge/` | MCU firmware (PlatformIO, Arduino Uno) — **WIP** |
| `stm32_serial_bridge/` | MCU firmware (PlatformIO, STM32 Nucleo) — **WIP** |

---

## 11. Credits

Developed by NHK Project, RRST, Ritsumeikan University, Japan (2024–2026)
- Official Website: https://www.rrst.jp
- X (Twitter): https://x.com/RRST_BKC

![Logo](https://www.rrst.jp/img/logo.png)
