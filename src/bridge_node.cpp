#include "serial_bridge/bridge_node.hpp"
#include "serial_bridge/config.hpp"
#include "serial_bridge/graphical_ui.hpp"

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int16_multi_array.hpp>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <sstream>
#include <termios.h>
#include <unistd.h>

namespace {
    constexpr double kSerialBaudRate = 115200.0;
    constexpr double kUartBitsPerByte = 10.0;
}

// EIO/ENODEV/ENXIO はデバイスの物理的な切断を示すエラーコード
static bool is_disconnect_error(int err) {
    return err == EIO || err == ENODEV || err == ENXIO;
}

// コメントそのうち整備します

/*
フレーム構造
[0] START   = 0xAA
[1] ID
[2] LEN     = データ長(byte)
[3..] DATA  = int16 big-endian
[last] CHECKSUM = ID ^ LEN ^ DATA...
*/

SerialBridgeNode::SerialBridgeNode(uint8_t device_id, const std::string &port,
                                   double rx_timeout_sec,
                                   double reconnect_interval_sec)
    : Node("serial_bridge_" + std::to_string(device_id)),
      device_id_(device_id),
      fd_(-1),
      port_(port),
      reconnect_interval_(std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<double>(reconnect_interval_sec))),
      rx_timeout_(std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<double>(rx_timeout_sec))),
      last_reconnect_attempt_(std::chrono::steady_clock::now() - reconnect_interval_),
      last_rx_time_(std::chrono::steady_clock::now()),
      last_status_log_time_(std::chrono::steady_clock::now()) {

    this->declare_parameter(
        serial_bridge::config::kParamVerbosePacketLog,
        serial_bridge::config::kVerbosePacketLogDefault);
    this->declare_parameter(
        serial_bridge::config::kParamStatusLogPeriodMs,
        serial_bridge::config::kStatusLogPeriodMsDefault);
    verbose_packet_log_ = this->get_parameter(
                                  serial_bridge::config::kParamVerbosePacketLog)
                              .as_bool();
    const auto status_log_period_ms = this->get_parameter(
                                              serial_bridge::config::kParamStatusLogPeriodMs)
                                          .as_int();
    status_log_period_ = std::chrono::milliseconds(
        std::max<int64_t>(serial_bridge::config::kStatusLogPeriodMsMin, status_log_period_ms));

    RCLCPP_INFO(this->get_logger(),
                "Device ID %u -> Port %s (verbose_packet_log=%s, status_period=%ldms)",
                static_cast<unsigned>(device_id_), port_.c_str(), verbose_packet_log_ ? "true" : "false",
                static_cast<long>(status_log_period_.count()));

    // ---------- ROS Pub/Sub ----------
    rx_pub_ = this->create_publisher<std_msgs::msg::Int16MultiArray>(
        "serial_rx_" + std::to_string(device_id_), 10);

    tx_sub_ = this->create_subscription<std_msgs::msg::Int16MultiArray>(
        "serial_tx_" + std::to_string(device_id_), 10,
        std::bind(&SerialBridgeNode::tx_callback,
                  this, std::placeholders::_1));

    // ---------- timer ----------
    timer_ = this->create_wall_timer(
        std::chrono::milliseconds(serial_bridge::config::kUpdatePeriodMs),
        std::bind(&SerialBridgeNode::update, this));

    // 初回接続を試みる（失敗してもタイマーで再試行）
    try_open_port();
}

void SerialBridgeNode::close_port() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    connected_ = false;
    rx_buffer_.clear();
    serial_bridge::graphical_ui::erase_node_line(device_id_);
}

bool SerialBridgeNode::try_open_port() {
    close_port();

    fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) {
        RCLCPP_WARN(this->get_logger(),
                    "Cannot open %s: %s — will retry in %.1f s",
                    port_.c_str(), strerror(errno),
                    std::chrono::duration<double>(reconnect_interval_).count());
        return false;
    }

    termios tty{};
    tcgetattr(fd_, &tty);
    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    tcsetattr(fd_, TCSANOW, &tty);

    // カーネルの送受信バッファに残った古いデータを破棄する。
    // スキャナが同じポートを一時的に開いて読み込んだ後に残ったバイト列や、
    // 切断前の途中フレームがバッファに残っていると START_BYTE 同期が取れなくなる。
    if (tcflush(fd_, TCIOFLUSH) < 0) {
        RCLCPP_WARN(this->get_logger(),
                    "tcflush failed on %s: %s — stale bytes may remain",
                    port_.c_str(), strerror(errno));
    }

    RCLCPP_INFO(this->get_logger(), "Connected to %s", port_.c_str());
    connected_ = true;
    last_rx_time_ = std::chrono::steady_clock::now();
    return true;
}

/* ================= RX ================= */

void SerialBridgeNode::update() {
    // 未接続の場合は再接続を試みる（インターバル制限あり）
    if (fd_ < 0) {
        auto now = std::chrono::steady_clock::now();
        if (now - last_reconnect_attempt_ >= reconnect_interval_) {
            last_reconnect_attempt_ = now;
            try_open_port();
        }
        maybe_log_status();
        return;
    }

    constexpr uint8_t START_BYTE = serial_bridge::config::kStartByte;
    // static std::deque<uint8_t> rx_buffer;セグフォ防止のためメンバ変数へ移動

    uint8_t buf[serial_bridge::config::kReadBufferSize];
    int n = read(fd_, buf, sizeof(buf));
    if (n < 0) {
        if (is_disconnect_error(errno)) {
            RCLCPP_WARN(this->get_logger(),
                        "Device disconnected from %s — waiting for reconnection",
                        port_.c_str());
            close_port();
            last_reconnect_attempt_ = std::chrono::steady_clock::now();
        } else {
            RCLCPP_DEBUG(this->get_logger(), "read error on %s: %s", port_.c_str(), strerror(errno));
        }
        maybe_log_status();
        return;
    }
    if (n == 0) {
        // データなし: タイムアウトチェック
        auto now = std::chrono::steady_clock::now();
        if (now - last_rx_time_ >= rx_timeout_) {
            RCLCPP_WARN(this->get_logger(),
                        "RX timeout on %s — no data for %.1f s, closing port",
                        port_.c_str(),
                        std::chrono::duration<double>(rx_timeout_).count());
            close_port();
            last_reconnect_attempt_ = std::chrono::steady_clock::now();
        }
        maybe_log_status();
        return;
    }

    last_rx_time_ = std::chrono::steady_clock::now();
    rx_bytes_since_status_ += static_cast<uint64_t>(n);
    rx_total_bytes_ += static_cast<uint64_t>(n);

    for (int i = 0; i < n; i++)
        rx_buffer_.push_back(buf[i]);

    int processed = 0;
    constexpr int MAX_FRAMES = 64;

    while (rx_buffer_.size() >= 4 && processed < MAX_FRAMES) {
        // START 同期
        if (rx_buffer_.front() != START_BYTE) {
            rx_buffer_.pop_front();
            dropped_bytes_since_status_++;
            continue;
        }

        uint8_t length = rx_buffer_[2];
        size_t frame_size = 1 + 1 + 1 + length + 1;

        if (rx_buffer_.size() < frame_size)
            return; // フレーム未完

        // checksum
        uint8_t checksum = 0;
        for (size_t i = 1; i < 3 + length; i++)
            checksum ^= rx_buffer_[i];

        if (checksum != rx_buffer_[3 + length]) {
            checksum_errors_since_status_++;
            RCLCPP_DEBUG(this->get_logger(), "Checksum mismatch");
            rx_buffer_.pop_front();
            dropped_bytes_since_status_++;
            continue;
        }

        // ID check
        uint8_t rx_id = rx_buffer_[1];
        if (rx_id != device_id_) {
            id_mismatch_since_status_++;
            RCLCPP_DEBUG(this->get_logger(),
                         "ID mismatch rx=%u expected=%u",
                         static_cast<unsigned>(rx_id),
                         static_cast<unsigned>(device_id_));
            for (size_t i = 0; i < frame_size; i++)
                rx_buffer_.pop_front();
            continue;
        }

        // 16bit data decode
        int16_t values[serial_bridge::config::kRx16Num] = {0};
        size_t data16_count = std::min(
            static_cast<size_t>(length / 2),
            serial_bridge::config::kRx16Num);

        for (size_t i = 0; i < data16_count; i++) {
            values[i] = (int16_t)((rx_buffer_[3 + i * 2] << 8) |
                                  rx_buffer_[3 + i * 2 + 1]);
        }

        // publish
        std_msgs::msg::Int16MultiArray msg;
        msg.data.assign(values, values + serial_bridge::config::kRx16Num);
        rx_pub_->publish(msg);
        rx_frames_since_status_++;

        if (verbose_packet_log_) {
            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < serial_bridge::config::kRx16Num; i++) {
                oss << values[i];
                if (i + 1 < serial_bridge::config::kRx16Num)
                    oss << ", ";
            }
            oss << "]";

            RCLCPP_INFO(this->get_logger(),
                        "[ID %u] RX DATA: %s",
                        static_cast<unsigned>(device_id_), oss.str().c_str());
        }

        // consume
        for (size_t i = 0; i < frame_size; i++)
            rx_buffer_.pop_front();
        processed++;
    }

    maybe_log_status();
}

/* ================= TX ================= */

void SerialBridgeNode::tx_callback(
    const std_msgs::msg::Int16MultiArray::SharedPtr msg) {

    if (fd_ < 0)
        return; // 未接続時は送信しない

    if (msg->data.size() < serial_bridge::config::kTx16Num)
        return;

    constexpr uint8_t START_BYTE = serial_bridge::config::kStartByte;
    constexpr uint8_t LEN = serial_bridge::config::kTx16Num * 2;

    uint8_t frame[1 + 1 + 1 + LEN + 1];

    frame[0] = START_BYTE;
    frame[1] = device_id_;
    frame[2] = LEN;

    for (size_t i = 0; i < serial_bridge::config::kTx16Num; i++) {
        frame[3 + i * 2] = (msg->data[i] >> 8) & 0xFF;
        frame[3 + i * 2 + 1] = msg->data[i] & 0xFF;
    }

    uint8_t checksum = 0;
    for (size_t i = 1; i < 3 + LEN; i++)
        checksum ^= frame[i];

    frame[3 + LEN] = checksum;

    if (write(fd_, frame, sizeof(frame)) < 0) {
        if (is_disconnect_error(errno)) {
            RCLCPP_WARN(this->get_logger(),
                        "Device disconnected from %s (write error) — waiting for reconnection",
                        port_.c_str());
            close_port();
            last_reconnect_attempt_ = std::chrono::steady_clock::now();
        } else {
            tx_errors_since_status_++;
            RCLCPP_DEBUG(this->get_logger(), "write error on %s: %s", port_.c_str(), strerror(errno));
        }
    } else {
        tx_frames_since_status_++;
        tx_bytes_since_status_ += static_cast<uint64_t>(sizeof(frame));
        tx_total_bytes_ += static_cast<uint64_t>(sizeof(frame));
    }

    maybe_log_status();
}

void SerialBridgeNode::maybe_log_status() {
    const auto now = std::chrono::steady_clock::now();
    if (now - last_status_log_time_ < status_log_period_) {
        return;
    }

    const auto period_sec = std::chrono::duration<double>(now - last_status_log_time_).count();
    const auto rx_hz = period_sec > 0.0 ? (static_cast<double>(rx_frames_since_status_) / period_sec) : 0.0;
    const auto rx_byte_per_sec = period_sec > 0.0
                                     ? (static_cast<double>(rx_bytes_since_status_) / period_sec)
                                     : 0.0;
    const auto tx_byte_per_sec = period_sec > 0.0
                                     ? (static_cast<double>(tx_bytes_since_status_) / period_sec)
                                     : 0.0;
    const auto total_line_util_percent =
        std::clamp(((rx_byte_per_sec + tx_byte_per_sec) * kUartBitsPerByte / kSerialBaudRate) * 100.0,
                   0.0, 999.9);

    if (serial_bridge::graphical_ui::enabled()) {
        const int width = std::max(8, serial_bridge::config::kGraphRxBarWidth);
        const int filled = std::clamp(static_cast<int>(rx_hz), 0, width);
        const std::string bar(static_cast<size_t>(filled), '#');
        const std::string remain(static_cast<size_t>(width - filled), '.');

        std::ostringstream oss;

        // 接続状態（常に表示）
        if (serial_bridge::config::kShowGraphicalConnStatus) {
            oss << (connected_.load() ? "[ON ] " : "[OFF] ");
        }

        // RX Hz と プログレスバー
        if (serial_bridge::config::kShowGraphicalRxHz) {
            oss << "RX " << rx_hz << "Hz [" << bar << remain << "] ";
        }

        // TX フレーム カウント
        if (serial_bridge::config::kShowGraphicalTx) {
            oss << "TX(" << tx_frames_since_status_ << "/" << tx_errors_since_status_ << ") ";
        }

        // 帯域幅
        if (serial_bridge::config::kShowGraphicalBandwidth) {
            oss << "BW " << static_cast<int>(rx_byte_per_sec) << "/" << static_cast<int>(tx_byte_per_sec)
                << "Bps ";
        }

        // ライン利用率
        if (serial_bridge::config::kShowGraphicalUtilization) {
            oss << static_cast<int>(total_line_util_percent) << "% ";
        }

        // チェックサムエラー
        if (serial_bridge::config::kShowGraphicalChecksum) {
            oss << "CHK " << checksum_errors_since_status_ << " ";
        }

        // IDミスマッチ
        if (serial_bridge::config::kShowGraphicalIdMismatch) {
            oss << "IDM " << id_mismatch_since_status_ << " ";
        }

        // ドロップバイト
        if (serial_bridge::config::kShowGraphicalDropped) {
            oss << "DROP " << dropped_bytes_since_status_ << " ";
        }

        // バッファサイズ
        if (serial_bridge::config::kShowGraphicalBuffer) {
            oss << "BUF " << rx_buffer_.size() << " ";
        }

        // 累積通信量
        if (serial_bridge::config::kShowGraphicalTotal) {
            oss << "TOT " << rx_total_bytes_ << "/" << tx_total_bytes_ << "B";
        }

        serial_bridge::graphical_ui::set_node_line(device_id_, oss.str());
    } else {
        RCLCPP_INFO(
            this->get_logger(),
            "[ID %u] status connected=%s rx_hz=%.1f rx_frames=%llu rx_bytes=%llu tx_ok=%llu tx_err=%llu chk_err=%llu id_mismatch=%llu dropped_bytes=%llu rx_buffer=%zu",
            static_cast<unsigned>(device_id_),
            connected_.load() ? "true" : "false",
            rx_hz,
            static_cast<unsigned long long>(rx_frames_since_status_),
            static_cast<unsigned long long>(rx_bytes_since_status_),
            static_cast<unsigned long long>(tx_frames_since_status_),
            static_cast<unsigned long long>(tx_errors_since_status_),
            static_cast<unsigned long long>(checksum_errors_since_status_),
            static_cast<unsigned long long>(id_mismatch_since_status_),
            static_cast<unsigned long long>(dropped_bytes_since_status_),
            rx_buffer_.size());

        RCLCPP_INFO(
            this->get_logger(),
            "[ID %u] bandwidth rx=%.1fB/s tx=%.1fB/s util=%.1f%%",
            static_cast<unsigned>(device_id_),
            rx_byte_per_sec,
            tx_byte_per_sec,
            total_line_util_percent);
    }

    rx_frames_since_status_ = 0;
    rx_bytes_since_status_ = 0;
    tx_bytes_since_status_ = 0;
    dropped_bytes_since_status_ = 0;
    checksum_errors_since_status_ = 0;
    id_mismatch_since_status_ = 0;
    tx_frames_since_status_ = 0;
    tx_errors_since_status_ = 0;
    last_status_log_time_ = now;
}
