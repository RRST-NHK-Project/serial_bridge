# esp32_s3_serial_bridge

## 1. Overview

This firmware targets the ESP32-S3 serial bridge stack and supports local GPIO control, serial transport, and CAN transport over TWAI with MCP2561.

The CAN implementation distributes the existing 24-slot int16 payload across up to 4 nodes on a single bus.
Each node receives only the slot block assigned to it and uses that data for its local outputs and feedback.

---

## 2. Current Transport Modes

Select exactly one mode in src/config.hpp.

- MODE_IO: local GPIO, servo, encoder, and switch handling over serial
- MODE_CAN: CAN node mode. This board acts as one node on the CAN bus.
- MODE_CAN_HOST: CAN host mode. This board bridges serial data from the PC to CAN nodes and merges node feedback back to serial.
- MODE_DEBUG: development/debug mode

---

## 3. CAN Slot Mapping

The firmware uses the existing 24-slot int16 layout and divides it into 4 node blocks.

| Node | Slot range | Purpose |
|:---|---:|:---|
| Node 0 | 0-5 | First CAN node block |
| Node 1 | 6-11 | Second CAN node block |
| Node 2 | 12-17 | Third CAN node block |
| Node 3 | 18-23 | Fourth CAN node block |

Each node block contains 6 slots and is transmitted as two CAN frames with 4 int16 values each.

### Default config

The following macros are defined in src/config.hpp:

- CAN_NODE_COUNT = 4
- CAN_SLOTS_PER_NODE = 6
- CAN_NODE_INDEX = 0

For a real multi-node bus, set CAN_NODE_INDEX differently on each node and keep CAN_NODE_COUNT consistent across the bus.

---

## 4. How Data Flows

### Host mode

1. The host receives a full 24-slot payload from serial.
2. The host splits that payload into four node blocks.
3. Each block is sent over CAN to the corresponding node.
4. The host receives feedback blocks from the nodes and writes them back into the 24-slot payload.
5. The host sends the merged payload back to the PC over serial.

### Node mode

1. The node receives only its assigned slot block over CAN.
2. The first four slots are applied to local motor-like outputs.
3. The next two slots are applied to local servo-like outputs.
4. Local encoder/switch feedback is packed back into the same node block and sent to the host.

---

## 5. Configuration Workflow

1. Open src/config.hpp.
2. Set DEVICE_ID so each board has a unique ID.
3. Choose one mode macro.
4. For CAN mode, assign CAN_NODE_INDEX on each board:
   - 0, 1, 2, or 3
5. Adjust PWM, servo, and pin settings if required.
6. Build and flash with PlatformIO.

---

## 6. Notes

- This implementation uses the existing serial frame layout as the common data model.
- The CAN transport is intentionally simple and uses a shared slot map rather than a fully separate protocol.
- The slot mapping rule is now implemented and can be extended later for richer per-channel semantics.

---

## 7. Credits

Developed by NHK Project, RRST, Ritsumeikan University, Japan.
- Official Website: https://www.rrst.jp
- X (Twitter): https://x.com/RRST_BKC
