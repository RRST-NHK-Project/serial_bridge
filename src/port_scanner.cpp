/*====================================================================
シリアルポートを走査し、Arduino/STM32/ESP32 などマイコンから送信されるフレームを受信。
各フレームにはIDを含めているため、接続順に関わらず各マイコンを識別可能。
フレームは START_BYTE で同期し、CHECKSUM による破損検出を行う。
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/

/*====================================================================
 * --- フレーム構造 ---
 * [0] START_BYTE : 0xAA （フレーム先頭）
 * [1] DEVICE_ID  : デバイスID (1バイト)
 * [2] LENGTH     : DATA 部のバイト数 (Tx16NUM*2)
 * [3..N] DATA    : 16bitデータ配列、上位バイト→下位バイト
 * [N+1] CHECKSUM : ID, LENGTH, DATA に対する XOR チェックサム
 * ====================================================================*/

#include "serial_bridge/config.hpp"
#include "serial_bridge/graphical_ui.hpp"
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define START_BYTE 0xAA
#define MAX_FRAME_SIZE 256

struct Frame {
    uint8_t id;
    std::vector<uint8_t> data;
};

static inline bool scanner_log_enabled() {
    return serial_bridge::config::kLogOutputMode ==
           serial_bridge::config::LogOutputMode::kTerminal;
}

static inline bool scanner_graphical_enabled() {
    return serial_bridge::graphical_ui::enabled();
}

static void scanner_graphical_line(const std::string &state,
                                   const std::string &port,
                                   size_t checked,
                                   size_t total,
                                   size_t detected,
                                   size_t skipped_in_use,
                                   size_t skipped_excluded) {
    if (!scanner_graphical_enabled())
        return;

    std::ostringstream oss;
    oss << state
        << " checked=" << checked << "/" << total
        << " detected=" << detected
        << " in_use_skip=" << skipped_in_use
        << " excluded_skip=" << skipped_excluded;
    if (!port.empty()) {
        oss << " port=" << port;
    }
    serial_bridge::graphical_ui::set_scanner_line(oss.str());
}

static void scanner_graphical_detected_line(const std::map<uint8_t, std::string> &result) {
    if (!scanner_graphical_enabled())
        return;

    std::ostringstream oss;
    oss << "[SCAN] detected_map ";
    if (result.empty()) {
        oss << "(none)";
    } else {
        bool first = true;
        for (const auto &[id, port] : result) {
            if (!first)
                oss << " | ";
            first = false;
            oss << static_cast<unsigned>(id) << "->" << port;
        }
    }
    serial_bridge::graphical_ui::set_detected_line(oss.str());
}

//
// /dev/ttyACMx(Arduino, STM32) または /dev/ttyUSBx(ESP32) を列挙する
//
static std::vector<std::string> list_serial_ports() {
    std::vector<std::string> ports;

    // 探索パターン
    std::vector<std::string> patterns = {"/dev/ttyUSB*", "/dev/ttyACM*"};
    for (const auto &pattern : patterns) {
        if (scanner_log_enabled())
            std::cout << "[SCAN] pattern: " << pattern << std::endl;

        glob_t g;
        memset(&g, 0, sizeof(g));
        int ret = glob(pattern.c_str(), 0, NULL, &g);

        if (ret == 0) {
            for (size_t i = 0; i < g.gl_pathc; ++i) {
                if (scanner_log_enabled())
                    std::cout << "[SCAN] found: " << g.gl_pathv[i] << std::endl;
                ports.emplace_back(g.gl_pathv[i]);
            }
        } else {
            if (scanner_log_enabled())
                std::cout << "[SCAN] none matched for pattern " << pattern << std::endl;
        }
        globfree(&g);
    }

    return ports;
}

//
// フレームを受信する関数
// 成功 -> true, frame にデータ格納
// 失敗 -> false
//
static bool read_frame(int fd, Frame &frame, int timeout_ms = 2000) {
    uint8_t buf[MAX_FRAME_SIZE];
    int idx = 0;
    int total_wait = 0;

    while (total_wait < timeout_ms) {
        uint8_t byte;
        int n = read(fd, &byte, 1);
        if (n == 1) {
            if (idx == 0) {
                // START_BYTE までスキップ
                if (byte != START_BYTE)
                    continue;
            }
            buf[idx++] = byte;

            if (idx == 3) {
                // 3バイト目で LENGTH がわかる
                int frame_len = 3 + buf[2] + 1; // START+ID+LEN + DATA + CHECKSUM
                if (frame_len > MAX_FRAME_SIZE)
                    return false;
            }

            if (idx >= 3 && idx == 3 + buf[2] + 1) {
                // フレーム読み込み完了
                uint8_t checksum = 0;
                for (int i = 1; i < idx - 1; i++)
                    checksum ^= buf[i]; // ID+LEN+DATA
                if (checksum != buf[idx - 1]) {
                    if (scanner_log_enabled())
                        std::cout << "[ERROR] CHECKSUM mismatch\n";
                    return false;
                }

                frame.id = buf[1];
                frame.data.assign(buf + 3, buf + idx - 1);
                return true;
            }
        } else {
            usleep(1000);
            total_wait += 1;
        }
    }
    return false; // タイムアウト
}

//
// 指定ポートを開き、フレーム受信可能状態にする
//
static int open_serial_port(const std::string &port) {
    int fd = open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0)
        return -1;

    termios tty{};
    tcgetattr(fd, &tty);
    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // 0.1秒
    tcsetattr(fd, TCSANOW, &tty);

    // USB CDC 安定待ち
    usleep(500000);

    return fd;
}

//
// 全ポートを走査して、ID → ポート名 の map を返す
// skip_ports に含まれるポートはスキャン対象外（既に使用中のポートを避けるため）
//
std::map<uint8_t, std::string> detect_serial_devices(const std::set<std::string> &skip_ports,
                                                     const std::set<std::string> &excluded_ports) {
    std::map<uint8_t, std::string> result;
    auto ports = list_serial_ports();
    size_t checked = 0;
    size_t detected = 0;
    size_t skipped_in_use = 0;
    size_t skipped_excluded = 0;

    scanner_graphical_line("list", "", checked, ports.size(), detected,
                           skipped_in_use, skipped_excluded);

    if (scanner_log_enabled())
        std::cout << "[SCAN] Total ports found: " << ports.size() << std::endl;

    for (auto &p : ports) {
        if (skip_ports.count(p)) {
            checked++;
            skipped_in_use++;
            scanner_graphical_line("skip(in_use)", p, checked, ports.size(), detected,
                                   skipped_in_use, skipped_excluded);
            if (scanner_log_enabled())
                std::cout << "[SCAN] Skipping " << p << " (already in use)" << std::endl;
            continue;
        }
        if (excluded_ports.count(p)) {
            checked++;
            skipped_excluded++;
            scanner_graphical_line("skip(excluded)", p, checked, ports.size(), detected,
                                   skipped_in_use, skipped_excluded);
            if (scanner_log_enabled())
                std::cout << "[SCAN] Skipping " << p << " (excluded by config)" << std::endl;
            continue;
        }

        scanner_graphical_line("open", p, checked, ports.size(), detected,
                               skipped_in_use, skipped_excluded);
        if (scanner_log_enabled())
            std::cout << "[CHECK] Opening port: " << p << std::endl;
        int fd = open_serial_port(p);
        if (fd < 0) {
            checked++;
            scanner_graphical_line("open_failed", p, checked, ports.size(), detected,
                                   skipped_in_use, skipped_excluded);
            if (scanner_log_enabled())
                std::cout << "[NG] Failed to open " << p << std::endl;
            continue;
        }

        Frame frame;
        scanner_graphical_line("read", p, checked, ports.size(), detected,
                               skipped_in_use, skipped_excluded);
        if (read_frame(fd, frame)) {
            result[frame.id] = p;
            detected = result.size();
            checked++;
            scanner_graphical_line("detected", p, checked, ports.size(), detected,
                                   skipped_in_use, skipped_excluded);
            if (scanner_log_enabled())
                std::cout << "[OK] Detected ID=" << (int)frame.id << " on " << p << std::endl;
        } else {
            checked++;
            scanner_graphical_line("no_frame", p, checked, ports.size(), detected,
                                   skipped_in_use, skipped_excluded);
            if (scanner_log_enabled())
                std::cout << "[NG] No valid frame received from " << p << std::endl;
        }

        close(fd);
    }

    if (result.empty()) {
        if (scanner_log_enabled())
            std::cout << "[RESULT] No serial devices detected." << std::endl;
    } else {
        if (scanner_log_enabled())
            std::cout << "[RESULT] Total detected devices: " << result.size() << std::endl;
    }

    if (scanner_graphical_enabled()) {
        scanner_graphical_line("done", "", checked, ports.size(), result.size(),
                               skipped_in_use, skipped_excluded);
        scanner_graphical_detected_line(result);
    }

    return result;
}
