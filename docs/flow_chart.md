# serial_bridge 処理フローチャート

ROS 2 側（`bridge_node`）とマイコン側（代表例として `firmware/esp32_serial_bridge`）の、シリアルフレーム送受信の処理フローをまとめたもの。
STM32 / Arduino Uno / ESP32 系 / B-G431 等、他ターゲットのファームウェアも同一フレームプロトコルと FreeRTOS タスク構成を踏襲している。

## 共通フレーム構造

ROS 2 側・マイコン側とも同一のバイト列フォーマットを使用する（`bridge_node.cpp` / `serial_task.cpp` 共通仕様）。

| バイト位置 | 名前 | 内容 |
|---|---|---|
| `[0]` | START | `0xAA` 固定 |
| `[1]` | DEVICE ID | 送信元/宛先デバイスID |
| `[2]` | LENGTH | DATA部のバイト数 |
| `[3..]` | DATA | `int16` × N（big-endian、上位byte→下位byte） |
| `[last]` | CHECKSUM | `ID ^ LEN ^ DATA...`（XOR） |

- RX: マイコン → ROS 2（topic: `serial_rx_<id>`）
- TX: ROS 2 → マイコン（topic: `serial_tx_<id>`）

---

## ① ROS 2 側処理（bridge_node パッケージ）

`main.cpp` が起動すると、常駐する「スキャンスレッド」がポートを走査してデバイスを検出し、検出ごとに `SerialBridgeNode`（1デバイス = 1ノード）を動的に生成して `MultiThreadedExecutor` へ登録する。

### 起動〜デバイス検出（`main.cpp` / `port_scanner.cpp`）

```mermaid
flowchart TD
    A["rclcpp::init<br/>パラメータ読込<br/>(excluded_ports, rx_timeout_sec,<br/>reconnect_interval_sec, scan_interval_ms)"] --> B["MultiThreadedExecutor 生成<br/>debug_node 登録"]
    B --> C["executor.spin() 開始（メインスレッド）"]
    B --> D["スキャンスレッド開始（バックグラウンド）"]

    C --> C2["常駐: 登録済み全ノードの<br/>timer / subscription を実行"]

    D --> D1["known_ports を除外リスト化"]
    D1 --> D2["detect_serial_devices()<br/>/dev/ttyUSB*, /dev/ttyACM* を glob<br/>各ポートを開き read_frame() で試読"]
    D2 --> D3{"新規デバイス or<br/>再接続を検出？"}
    D3 -->|"未登録IDを検出"| D4["SerialBridgeNode 新規生成<br/>node_map登録・executor.add_node()"]
    D3 -->|"既知IDが別ポートで復活"| D5["旧ノードをremove_node<br/>新ノード生成・add_node()"]
    D3 -->|"接続中 or 未検出"| D6["何もしない"]
    D4 --> D7["scan_interval_ms 待機"]
    D5 --> D7
    D6 --> D7
    D7 --> D1

    classDef decision fill:#eee9f6,stroke:#a596c7,color:#5b4a86;
    classDef loopback fill:#ffffff,stroke:#4a6a63,color:#4a6a63,stroke-dasharray: 3 3;
    class D3 decision
    class D6 loopback
```

### SerialBridgeNode: 受信処理 RX（`bridge_node.cpp` — `update()`）

ノードごとに wall timer で駆動。1バイト単位で START_BYTE 同期を取り、checksum と ID を検証してから ROS トピックに publish する。

```mermaid
flowchart TD
    A["timer発火 → update()"] --> B{"fd < 0（未接続）？"}
    B -->|"Yes"| B1["reconnect_interval 経過確認<br/>経過していれば try_open_port()"]
    B1 --> B2["状態ログ更新 → return"]
    B -->|"No（接続中）"| C["read(fd, buf, N)"]
    C --> D{"n の値は？"}
    D -->|"n < 0"| D1["エラー種別確認<br/>切断エラー(EIO/ENODEV/ENXIO)なら<br/>close_port()"]
    D -->|"n == 0"| D2["RXタイムアウト判定<br/>rx_timeout_sec 超過なら close_port()"]
    D -->|"n > 0"| D3["rx_buffer_（deque）へ追加"]

    D3 --> E["while: バッファ≥4 かつ<br/>未処理フレーム数<64"]
    E --> F{"先頭byte == START_BYTE(0xAA)？"}
    F -->|"不一致"| F1["先頭1byte破棄"] --> E
    F -->|"一致"| G{"LENGTHからフレーム全体が<br/>揃っている？"}
    G -->|"未充足"| G1["return（次回受信を待つ）"]
    G -->|"充足"| H{"CHECKSUM 一致？"}
    H -->|"不一致"| H1["1byte破棄"] --> E
    H -->|"一致"| I{"ID == 自ノードの device_id_？"}
    I -->|"不一致"| I1["フレーム全体を破棄"] --> E
    I -->|"一致"| J["int16 デコード（big-endian）"]
    J --> K["Int16MultiArray publish<br/>topic: serial_rx_&lt;id&gt;"]
    K --> L["消費したフレーム分を<br/>バッファから除去"] --> E
    D1 --> M["maybe_log_status()"]
    D2 --> M
    B2 --> M

    classDef decision fill:#eee9f6,stroke:#a596c7,color:#5b4a86;
    classDef loopback fill:#ffffff,stroke:#4a6a63,color:#4a6a63,stroke-dasharray: 3 3;
    classDef term fill:#fbe9e4,stroke:#a8402c,color:#a8402c;
    class B,D,F,G,H,I decision
    class F1,H1,I1 loopback
    class G1 term
```

### SerialBridgeNode: 送信処理 TX（`bridge_node.cpp` — `tx_callback()`）

`serial_tx_<id>` トピックの subscription コールバックとして駆動（timer ではなくイベント駆動）。

```mermaid
flowchart TD
    A["serial_tx_&lt;id&gt; 受信 → tx_callback()"] --> B{"fd < 0 または<br/>データ長不足？"}
    B -->|"該当"| B1["return（送信しない）"]
    B -->|"問題なし"| C["フレーム組み立て<br/>START/ID/LEN + int16→big-endian DATA"]
    C --> D["CHECKSUM 計算（XOR）"]
    D --> E["write(fd, frame)"]
    E --> F{"write() 成功？"}
    F -->|"失敗"| F1["エラー種別確認<br/>切断エラーなら close_port()<br/>それ以外は tx_errors カウント"]
    F -->|"成功"| F2["tx_frames / bytes カウント更新"]
    F1 --> G["maybe_log_status()"]
    F2 --> G
    B1 --> G

    classDef decision fill:#eee9f6,stroke:#a596c7,color:#5b4a86;
    classDef term fill:#fbe9e4,stroke:#a8402c,color:#a8402c;
    class B,F decision
    class B1 term
```

---

## ② マイコン側処理（ESP32 版ファームウェアを代表例）

### setup(): 起動・タスク生成（`main.cpp`）

```mermaid
flowchart TD
    A["Serial.begin(115200)"] --> B["起動待機<br/>delay(200) + delay(100 × DEVICE_ID)<br/>ID分だけ開始タイミングをずらす"]
    B --> C["LED を DEVICE_ID 回点滅<br/>自身のIDを目視確認できるように"]
    C --> D["serialTask を FreeRTOS タスク生成<br/>優先度10・スタック2048語"]
    D --> E{"MODE_* マクロ（config.hpp）は？"}
    E -->|"MODE_OUTPUT"| F1["Output_Task 生成"]
    E -->|"MODE_INPUT"| F2["Input_Task 生成"]
    E -->|"MODE_IO"| F3["IO_Task 生成"]
    E -->|"MODE_ROBOMAS 系"| F4["robomas_init() +<br/>M2006/M3508/gm6020 Task 等"]
    F1 --> G["setup() 完了"]
    F2 --> G
    F3 --> G
    F4 --> G
    G --> H["loop() は vTaskDelay(1000ms) のみ<br/>実処理は全て FreeRTOS タスク側"]

    classDef decision fill:#eee9f6,stroke:#a596c7,color:#5b4a86;
    class E decision
```

> `MODE_*` は 1 つのみ定義する必要があり、複数定義・未定義はコンパイルエラーになる。

### serialTask: フレーム送受信（`serial_task.cpp`）

1つの FreeRTOS タスク内で、RX（状態機械によるバイト単位パース）と TX（周期送信）を毎ループ両方処理する。

```mermaid
flowchart TD
    subgraph RX["RX: receive_frame()（毎ループ呼出し・状態機械）"]
    RA["WAIT_START<br/>1byte読む: 0xAAを待つ"] --> RB["WAIT_ID<br/>rx_id格納・checksum初期化"]
    RB --> RC["WAIT_LEN<br/>rx_len格納"]
    RC --> RD{"LEN > Rx16NUM×2？"}
    RD -->|"不正"| RA
    RD -->|"正常"| RE["WAIT_DATA<br/>1byteずつrx_bufへ格納<br/>checksumを逐次XOR更新"]
    RE --> RF["WAIT_CHECKSUM"]
    RF --> RG{"checksum一致 かつ<br/>ID==DEVICE_ID？"}
    RG -->|"不一致"| RA
    RG -->|"一致"| RH["Rx_16Data[]へデコード反映<br/>int16, big-endian→ネイティブ"]
    RH --> RI["受信LEDトグル<br/>ENABLE_LED時、500ms間隔"]
    RI --> RA
    end

    subgraph TX["TX: 周期送信（同ループ内で時間判定）"]
    TA{"前回送信からTX_PERIOD_MS経過？<br/>(100ms、入力系モードは20ms)"}
    TA -->|"未経過"| TA
    TA -->|"経過"| TB["send_frame()"]
    TB --> TC["Tx_16Data[]→フレーム化<br/>START/DEVICE_ID/LEN + big-endian DATA"]
    TC --> TD2["CHECKSUM計算（XOR）"]
    TD2 --> TE["Serial.write(Tx_8Data)"]
    TE --> TA
    end

    classDef decision fill:#eee9f6,stroke:#a596c7,color:#5b4a86;
    class RD,RG,TA decision
```

> `receive_frame()` は毎ループ呼び出され、送信は `TX_PERIOD_MS` 経過判定のうえ `send_frame()` が呼ばれる。両者は同一タスク内で `vTaskDelay(1ms)` を挟みながら繰り返される。

### 制御タスク例: Output_Task / Input_Task（`pin_ctrl_task.cpp`、5ms周期）

serialTask とは独立した FreeRTOS タスクとして動作し、共有配列 `Rx_16Data[]` / `Tx_16Data[]` を介して GPIO と受け渡しする。`MODE_IO` はこの2系統を1タスクにまとめたもの。

```mermaid
flowchart TD
    subgraph OUT["Output_Task（Rx_16Data → 出力）"]
    OA["Output_init()<br/>サーボ初期角セット"] --> OB["MD_Output()<br/>Rx_16Data[1-4]をPWM上限でconstrain<br/>→方向pin + ledcWrite（モータ駆動）"]
    OB --> OC["Servo_Output()<br/>角度[deg]→パルス幅[us]→duty変換<br/>ledcWrite ×4ch"]
    OC --> OD["TR_Output()<br/>Rx_16Data[17-23]→digitalWrite<br/>（トランジスタON/OFF）"]
    OD --> OE["vTaskDelayUntil(5ms)"]
    OE --> OB
    end

    subgraph IN["Input_Task（入力 → Tx_16Data）"]
    IA["Input_init()"] --> IB["ENC_Input()<br/>pcnt_get_counter_value ×4<br/>→Tx_16Data[1-4]"]
    IB --> IC["SW_Input()<br/>digitalReadを反転して<br/>Tx_16Data[9-16]へ格納"]
    IC --> ID2["vTaskDelayUntil(5ms)"]
    ID2 --> IB
    end
```

`Tx_16Data[]` は serialTask の TX 側がそのまま ROS 2 へ送信し、ROS 2 側が publish する `Rx_16Data[]`（＝ROS 2 から見た TX フレーム）を serialTask の RX 側が受け取って `Output_Task` が参照する — という形で、2つのタスクが共有配列を介して疎結合に連携する。

---

参照ソース: `bridge_node.cpp` / `main.cpp` / `port_scanner.cpp`（ROS 2側）, `firmware/esp32_serial_bridge/src/{main,serial_task,pin_ctrl_task}.cpp`（マイコン側）
