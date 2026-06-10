/*====================================================================
<config.hpp>
書き込み前にここでIDと通信周期、スロット割り当てを設定してください。
serial_bridge (ROS2) 側は DEVICE_ID と int16配列のスロット数(PC->MCU=24, MCU->PC=17)
に合わせて動作します。

※ 現在は PC->MCU / MCU->PC ともに 24 slot に統一しています。

Copyright (c) 2026.
====================================================================*/

#pragma once

#include <stdint.h>

// ================= 基本設定 =================

// IDの設定（ROS側からマイコンを識別するために使用）
// すべてのマイコンで異なる値にすること
#define DEVICE_ID 0x99

// MCU -> PC 送信周期 (ms)
#define TX_PERIOD_MS 20

// PC -> MCU コマンドタイムアウト (ms)
// この時間を超えて受信が無い場合はフェイルセーフ(stop)
#define CMD_TIMEOUT_MS 500

// ================= RX enable behavior =================
// RX_ENABLE が 0 のままでも、ターゲットが非ゼロなら「暗黙的に enable」とみなす。
// 別ノードが enable スロットを書かない/常に 0 の場合の互換用。
// 明示的な enable/disable を厳密に運用したい場合は 0。
#define ALLOW_IMPLICIT_ENABLE 1

// ================= デフォルト値 =================

// C3542-920KV 想定の初期値
// - 一般的な 14 極ロータを想定して pole pairs = 7
// - まずは保守的なゲインと速度上限から開始する
#define MOTOR_POLE_PAIRS 7

// ================= Sensor (Encoder) =================
// SimpleFOC の Encoder は "PPR(1chあたりのパルス/回転)" を指定し、
// quadrature(4x) はライブラリ側で自動換算されます。
// 例: 2048PPR のAB相インクリメンタル → 2048
#define ENCODER_PPR 48

// Index(Z相) を使う場合のみ 1 にしてください。
// 未配線/ノイズのある Index を有効化すると、高速回転時に角度補正が入り振動の原因になります。
#define ENCODER_ENABLE_INDEX 0

// 速度制御の切り分け用。
// `1` にすると RX_MODE を無視して常に速度制御を使う。
#define FORCE_VELOCITY_MODE 1

#define DEFAULT_VOLTAGE_SUPPLY 24.0f
#define DEFAULT_VOLTAGE_LIMIT 6.0f
#define DEFAULT_VELOCITY_LIMIT 1500.0f
// 電流制限 [A]
#define DEFAULT_CURRENT_LIMIT 10.0f

// ================= Command slew-rate limits =================
// 急激な指令変化で電流制限に入りにくくするための内部ランプ制限。
// `0` にすると内部ランプを無効化して、受信した目標値をそのまま適用する。
#define ENABLE_TARGET_RAMP 0
// - velocity: rad/s^2
// - angle: deg/s
#define TARGET_VELOCITY_SLEW_RATE 120.0f
#define TARGET_ANGLE_SLEW_RATE_DEG 360.0f

// ================= FOC gains =================
// Current loop
#define CURRENT_PID_P 0.1f
#define CURRENT_PID_I 10.0f
#define CURRENT_PID_D 0.0f

// Velocity loop
#define VELOCITY_PID_P 0.02f
#define VELOCITY_PID_I 0.0f
#define VELOCITY_PID_D 0.0f

#define VELOCITY_PID_OUTPUT_RAMP 1000.0f
#define VELOCITY_LPF_TF 0.02f

// Angle loop
#define ANGLE_P_GAIN 8.0f

// ================= Slot Index (PC -> MCU) =================
// Rx_16Data[0..23]
#define RX_DEBUG 0
#define RX_ENABLE 1
#define RX_MODE 2
#define RX_TARGET_VELOCITY 3
#define RX_TARGET_ANGLE 4
#define RX_VOLTAGE_LIMIT 5

// ================= Slot Index (MCU -> PC) =================
// Tx_16Data[0..16]
#define TX_DEBUG 0
#define TX_ANGLE 1
#define TX_VELOCITY 2
#define TX_TARGET 3
#define TX_MODE 4
#define TX_VOLTAGE_LIMIT 5
// 追加テレメトリ: 実測RPM (signed, 1rpm)
#define TX_RPM 6

// ================= Unit scaling =================
// 受信側: int16 -> float
// - velocity: 0.1 rad/s
// - angle: 0.1 deg
// - voltage_limit: 0.1 V
#define TARGET_VELOCITY_SCALE 0.1f
#define TARGET_ANGLE_SCALE 0.1f
#define VOLTAGE_LIMIT_SCALE 0.1f
