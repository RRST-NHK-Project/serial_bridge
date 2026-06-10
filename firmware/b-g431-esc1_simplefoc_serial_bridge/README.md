# SimpleFOC_SERIAL_WIP

B-G431B-ESC1 用の SimpleFOC ベース新規 PIO プロジェクトです。

目的は、既存の `serial_bridge` (ROS2) と同じフレーム仕様で、USB シリアル経由に
FOC の目標値(速度/位置)を送り、角度/速度を返す簡易サーボを作ることです。

## ビルド

```bash
pio run
```

## 書き込み

ボードは `disco_b_g431b_esc1` を使います。必要に応じて PlatformIO の upload 方法を選んでください。

## serial_bridge 互換フレーム

フレーム形式は以下です。

`[0xAA][DEVICE_ID][LEN][DATA...][CHECKSUM]`

- `DATA` は `int16` 配列 (big-endian: 上位バイト→下位バイト)
- `CHECKSUM` は `DEVICE_ID ^ LEN ^ DATA...` の XOR

この仕様は [serial_bridge/src/bridge_node.cpp](../../../serial_bridge/src/bridge_node.cpp) と同じです。

### PC -> MCU (command)

`serial_bridge` 側の送信スロット数は 24 (`kTx16Num=24`) です。

本ファームの割り当て:

- `Rx_16Data[RX_ENABLE]`  (=1): enable (0=stop, 1=run)
- `Rx_16Data[RX_MODE]`    (=2): mode (0=velocity, 1=angle)
- `Rx_16Data[RX_TARGET_VELOCITY]` (=3): target_velocity (0.1 rad/s)
- `Rx_16Data[RX_TARGET_ANGLE]`    (=4): target_angle (0.1 deg)
- `Rx_16Data[RX_VOLTAGE_LIMIT]`   (=5): voltage_limit (0.1 V)

### MCU -> PC (telemetry)

`serial_bridge` 側の受信スロット数は 24 です。

本ファームの割り当て:

- `Tx_16Data[TX_ANGLE]`   (=1): angle (0.1 deg)
- `Tx_16Data[TX_VELOCITY]` (=2): velocity (0.1 rad/s)
- `Tx_16Data[TX_TARGET]`  (=3): target (mode に応じて単位が変化)
- `Tx_16Data[TX_MODE]`    (=4): mode (0=velocity, 1=angle)
- `Tx_16Data[TX_VOLTAGE_LIMIT]` (=5): voltage_limit (0.1 V)
- `Tx_16Data[TX_RPM]` (=6): rpm (1 rpm, signed)

## 動作

- `enable=0` または RX が一定時間来ない場合、速度ターゲットを 0 に落とします。
- テレメトリは `TX_PERIOD_MS` 間隔で定期送信します。

## 補足

- 既存の `SimpleFOC_WIP` をベースに、シリアルからターゲットを入れられるようにした構成です。
- 速度と位置は実行中に切り替えられます。
- ホスト側はこのフレーム仕様に合わせて送受信してください。
