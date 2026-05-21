#pragma once

#include <cstddef>
#include <cstdint>

namespace serial_bridge::config {

    enum class LogOutputMode {
        kTerminal,
        kGraphical,
        kNone,
    };

    // ログ表示方法を選択する。
    // kTerminal: これまで通りターミナルに表示
    // kGraphical: ターミナルに簡易グラフィカル表示（ASCIIバー）
    // kNone    : ターミナルに表示しない
    inline constexpr LogOutputMode kLogOutputMode = LogOutputMode::kGraphical;
    inline constexpr int kGraphRxBarWidth = 24;
    inline constexpr int kGraphicalUiFrameMs = 100;

    inline constexpr std::size_t kTx16Num = 24;
    inline constexpr std::size_t kRx16Num = 24;
    inline constexpr uint8_t kStartByte = 0xAA;
    inline constexpr int kReadBufferSize = 512;
    inline constexpr int kUpdatePeriodMs = 5;

    inline constexpr const char *kParamVerbosePacketLog = "verbose_packet_log";
    inline constexpr bool kVerbosePacketLogDefault = false;

    inline constexpr const char *kParamStatusLogPeriodMs = "status_log_period_ms";
    inline constexpr int64_t kStatusLogPeriodMsDefault = 100;
    inline constexpr int64_t kStatusLogPeriodMsMin = 100;

    // ================================================================
    // グラフィカル表示の項目選択（デフォルトは1行に収まるように設定）
    // ================================================================
    // 基本項目（常に表示）
    inline constexpr bool kShowGraphicalConnStatus = true;  // [ON/OFF]
    inline constexpr bool kShowGraphicalRxHz = true;        // RX Xfx.xHz [...]
    inline constexpr bool kShowGraphicalTx = true;          // TX(a/b)
    inline constexpr bool kShowGraphicalBandwidth = true;   // BW rx/txBps
    inline constexpr bool kShowGraphicalUtilization = true; // util%

    // 詳細項目（デフォルトで非表示：1行に収まるようにするため）
    inline constexpr bool kShowGraphicalChecksum = true;    // CHK
    inline constexpr bool kShowGraphicalIdMismatch = false; // IDM
    inline constexpr bool kShowGraphicalDropped = false;    // DROP
    inline constexpr bool kShowGraphicalBuffer = true;      // BUF
    inline constexpr bool kShowGraphicalTotal = false;      // TOT

} // namespace serial_bridge::config
