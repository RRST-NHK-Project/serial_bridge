# **serial_bridge**

> English version: [README.md](README.md)

## 1. 概要

`serial_bridge` は、ROS 2 ノードとマイコン間のシリアル通信を双方向にブリッジする ROS 2 パッケージです。  
利用可能なシリアルポートを自動スキャンし、各フレームに埋め込まれた `DEVICE_ID` によってマイコンを識別します。複数のマイコンを同時に管理でき、手動でのポート指定は不要です。

機体のモーター、サーボ、ソレノイドバルブの制御、エンコーダ、マイクロスイッチ、測距センサーなどのセンサ類の取得をリアルタイムに行うことができます。

---

## 2. システム要件

| 項目 | 内容 |
|:---|:---|
| OS | Ubuntu 24.04 LTS |
| ROS | ROS 2 Jazzy |
| ボーレート | 115200 bps |
| ハードウェア | USB または UART で PC に接続されたマイコン |

> **注意**:  
> `/dev/ttyUSB*` や `/dev/ttyACM*` を使用するには `dialout` グループへの追加が必要です。  
> `sudo usermod -aG dialout $USER`（反映には再ログインが必要）

---

## 3. 機能一覧

- シリアルポートの自動スキャン — ポートの手動指定が不要です。
- 動的ノード生成によりホットプラグに対応しており、起動中にマイコンを抜き挿ししても自動検出・自動復帰します。
- 複数マイコンの接続に対応: フレーム内の `DEVICE_ID` で識別するため USB ポート番号に依存しません。
- int16 配列によるカスタムバイナリフレームプロトコル（TX 24スロット / RX 24スロット）
- XOR チェックサムによるフレーム検証を行い、通信の信頼性を担保しています。
- 通信切断・タイムアウト時には自動で再接続を試みます。
- 3 種類のログ出力モード: テキスト表示 / グラフィカル / サイレントを搭載しています。デフォルトはグラフィカルモードです。
- ESP32 / Arduino Uno（WIP） / STM32 Nucleo（WIP） 向けのマイコンファームウェアを同梱（PlatformIO）しているため、IDとモードを設定して書き込むだけで使用可能です。

---

## 4. システム構成

### 4.1 全体構成図

```
  [任意の ROS 2 ノード]
       │  serial_tx_[ID]  (Publish)
       │  serial_rx_[ID]  (Subscribe)
       │
  [serial_bridge ノード]
    ├─ port_scanner  ─── /dev/ttyUSB*, /dev/ttyACM* を探索
    │                    DEVICE_ID でマイコンを識別
    │                    検出時に bridge_node を自動生成
    │
    ├─ bridge_node (ID=6)  ── /dev/ttyUSB0 ──► マイコン (ID=6)   モータ/サーボ/TR
    ├─ bridge_node (ID=7)  ── /dev/ttyUSB1 ──► マイコン (ID=7)   エンコーダ/SW
    └─ bridge_node (ID=N)  ── ...          ──► マイコン (ID=N)   ...
```

### 4.2 各ファイルの役割

| ファイル名 | 役割 |
|:---|:---|
| `main.cpp` | ROS Executorの初期化、パラメータ読み込み、スキャナスレッドの起動 |
| `port_scanner` | 全 `/dev/ttyUSB*`・`/dev/ttyACM*` ポートを探索（除外リスト適用）し、プローブフレームを送受信して `DEVICE_ID → ポート名` のマップを返す |
| `bridge_node` | MCU 1台につき 1 インスタンス。5ms タイマーでシリアル RX を読み取り `serial_rx_[ID]` を Publish。`serial_tx_[ID]` を Subscribe してシリアル TX に書き込む |

### 4.3 ホットプラグ・自動再接続の流れ

```
【マイコン接続時】
    └─► port_scanner が /dev/ttyUSBx で有効なフレームを検出
            └─► SerialBridgeNode を生成し executor に追加
                    └─► serial_rx_[ID] / serial_tx_[ID] トピックが有効化

【マイコン切断 / RX タイムアウト時】
    └─► bridge_node が EIO / ENODEV / ENXIO エラー or rx_timeout_sec を検出
            └─► ポートをクローズ（connected_ = false）
                    └─► reconnect_interval_sec ごとに再オープンを試みる
                    └─► 次のスキャンサイクルで port_scanner も再検出
                            └─► 別ポートで見つかった場合はノードを差し替え
```

> **Tips**: マイコンの識別はフレーム内の `DEVICE_ID` で行われ、USB ポート番号には依存しません。  
> 別の USB ポートに差し直しても同じ `serial_rx_[ID]` / `serial_tx_[ID]` トピックで通信できます。

### 4.4 通信の流れ（双方向通信の場合）

```
ROS 2 (serial_bridge)                      マイコン

  serial_tx_[ID] 受信
       │
       ▼
  TX フレーム組み立て
  [0xAA][ID][LEN][DATA...][CHK]
       │
       └─────────────────────────────────► シリアル受信
                                           └─ 指令値をモータ/サーボに適用

                                           フィードバック送信
                                           [0xAA][ID][LEN][DATA...][CHK]
       ◄─────────────────────────────────┘
  RX フレーム受信
       │
       ├─ START_BYTE 同期
       ├─ CHECKSUM 検証
       └─► serial_rx_[ID] として Publish
```

---

## 5. フレームフォーマット

```
[0]     START_BYTE  : 0xAA
[1]     DEVICE_ID   : マイコンごとに設定した固有ID（uint8）
[2]     LENGTH      : ペイロードのバイト数（= int16 個数 × 2）
[3..N]  DATA        : int16 配列、ビッグエンディアン（上位バイト先行）
[N+1]   CHECKSUM    : バイト [1]〜[N] の XOR
```

### デフォルトデータスロット数（`config.hpp` で変更可能）

| 方向 | スロット数 | バイト数 |
|:---|---:|---:|
| PC → マイコン（TX） | 24 × int16 | 48 |
| マイコン → PC（RX） | 24 × int16 | 48 |

### TX スロットの標準割り当て（esp32_serial_bridge）

| スロット | 用途 |
|---:|:---|
| 1〜8 | DC モータ指令値 |
| 9〜16 | RC サーボ指令値 |
| 17〜23 | TR（トランジスタ/ソレノイド）出力 |

### RX スロットの標準割り当て

| スロット | 用途 |
|---:|:---|
| 1〜8 | エンコーダ値 |
| 9〜16 | スイッチ / センサ入力 |
| 17〜23 | 予約 |

---

## 6. ROS 2 インターフェース

### Subscribe トピック（ROS → マイコン）

| トピック | 型 | 説明 |
|:---|:---|:---|
| `serial_tx_[DEVICE_ID]` | `std_msgs/msg/Int16MultiArray` | マイコンへの制御指令 |

このトピックに他ノードからPublishすることでserial_bridgeがSubscribeを行い、マイコンへ送信します。

### Publish トピック（マイコン → ROS）

| トピック | 型 | 説明 |
|:---|:---|:---|
| `serial_rx_[DEVICE_ID]` | `std_msgs/msg/Int16MultiArray` | エンコーダ値・センサデータ |

このトピックを他ノードからSubscribeすることでセンサ値を取得・利用することができます。

---

## 7. パラメータ（`config/serial_bridge.yaml`）

| パラメータ | デフォルト | 説明 |
|:---|:---|:---|
| `excluded_ports` | `["/dev/ttyUSB0"]` | スキャン対象から除外するポート（例: LiDAR、GPS） |
| `rx_timeout_sec` | `2.0` | この秒数 RX がなければポートを閉じる |
| `reconnect_interval_sec` | `3.0` | 切断後の再接続までの最小待機時間（秒） |
| `scan_interval_ms` | `5000` | ポートスキャンの間隔（ミリ秒） |

ビルドし直さずに `config/serial_bridge.yaml` を編集するだけで変更が反映されます。

---

## 8. 使い方

### 8.1 クローン

```bash
cd ~/ros2_ws/src
git clone https://github.com/RRST-NHK-Project/serial_bridge.git
```

### 8.2 ビルド

```bash
cd ~/ros2_ws
colcon build --packages-select serial_bridge
source install/setup.bash
```

### 8.3 起動

```bash
ros2 launch serial_bridge serial_bridge.launch.py
```

ノード起動後すぐにポートのスキャンが始まり、マイコンが検出されるとログに表示されます。

---

## 9. ログ出力モード

`include/serial_bridge/config.hpp` でコンパイル時に設定します。

| モード | 説明 |
|:---|:---|
| `kTerminal` | 標準 ROS 2 ロガー出力 |
| `kGraphical` | マイコンごとのバーグラフ（RX レート・帯域・使用率） |
| `kNone` | サイレント — ターミナル出力なし |

---

## 10. ディレクトリ構成

| パス | 説明 |
|:---|:---|
| `src/` | ROS 2 ノードソース（`bridge_node`、`port_scanner`、`main`） |
| `include/serial_bridge/` | ヘッダファイル（`config.hpp`、`bridge_node.hpp`、`port_scanner.hpp`、`graphical_ui.hpp`） |
| `config/` | ROS 2 パラメータファイル（`serial_bridge.yaml`） |
| `launch/` | ランチファイル（`serial_bridge.launch.py`） |
| `esp32_serial_bridge/` | マイコン向けファームウェア（PlatformIO） |
| `arduino_uno_serial_bridge/` | マイコン向けファームウェア（PlatformIO, Arduino Uno）— **WIP** |
| `stm32_serial_bridge/` | マイコン向けファームウェア（PlatformIO, STM32 Nucleo）— **WIP** |

---

## 11. クレジット

立命館大学 RRST NHK プロジェクト（2024–2026）
- 公式 Web サイト: https://www.rrst.jp
- X (Twitter): https://x.com/RRST_BKC

![Logo](https://www.rrst.jp/img/logo.png)
