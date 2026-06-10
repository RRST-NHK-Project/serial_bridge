#pragma once

#include "serial_bridge/config.hpp"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace serial_bridge::graphical_ui {

    inline constexpr int kPanelWidth = 100;

    inline constexpr const char *kReset = "\033[0m";
    inline constexpr const char *kFgMuted = "\033[38;5;245m";
    inline constexpr const char *kFgTitle = "\033[38;5;45m";
    inline constexpr const char *kFgAccent = "\033[38;5;81m";
    inline constexpr const char *kFgGood = "\033[38;5;48m";        // 緑：良好
    inline constexpr const char *kFgGoodMid = "\033[38;5;120m";    // 緑系：やや良好
    inline constexpr const char *kFgWarn = "\033[38;5;214m";       // 黄：注意
    inline constexpr const char *kFgWarnOrange = "\033[38;5;208m"; // オレンジ：警告
    inline constexpr const char *kFgBad = "\033[38;5;203m";        // 赤：異常
    inline constexpr const char *kFgText = "\033[38;5;252m";

    inline std::string pad_or_trim(const std::string &text, int width) {
        if (width <= 0)
            return "";
        if (static_cast<int>(text.size()) >= width)
            return text.substr(0, static_cast<size_t>(width));
        return text + std::string(static_cast<size_t>(width - text.size()), ' ');
    }

    inline bool is_utf8_continuation(unsigned char c) {
        return (c & 0xC0u) == 0x80u;
    }

    // ANSIエスケープを可視幅に含めず、必要に応じて切り詰め/右側パディングする。
    inline std::string fit_ansi_text(const std::string &text, int width) {
        if (width <= 0)
            return "";

        std::string out;
        out.reserve(text.size() + 8);

        int visible = 0;
        size_t i = 0;
        while (i < text.size()) {
            // ANSI CSI sequence: ESC [ ... letter
            if (text[i] == '\033' && i + 1 < text.size() && text[i + 1] == '[') {
                const size_t start = i;
                i += 2;
                while (i < text.size()) {
                    const unsigned char ch = static_cast<unsigned char>(text[i]);
                    i++;
                    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                        break;
                    }
                }
                out.append(text, start, i - start);
                continue;
            }

            const unsigned char c = static_cast<unsigned char>(text[i]);
            size_t char_len = 1;
            if ((c & 0x80u) != 0u) {
                // UTF-8先頭バイトから連続バイトを取り込む
                size_t j = i + 1;
                while (j < text.size() &&
                       is_utf8_continuation(static_cast<unsigned char>(text[j]))) {
                    j++;
                }
                char_len = j - i;
            }

            if (visible >= width)
                break;

            out.append(text, i, char_len);
            visible++;
            i += char_len;
        }

        if (visible < width) {
            out += std::string(static_cast<size_t>(width - visible), ' ');
        }
        return out;
    }

    inline std::string panel_line(const std::string &content,
                                  const char *content_color = kFgText,
                                  const char *border_color = kFgMuted) {
        const int inner_width = kPanelWidth - 4;
        const std::string body = fit_ansi_text(content, inner_width);
        return std::string(border_color) + "| " + std::string(content_color) +
               body + std::string(border_color) + " |" + kReset;
    }

    inline std::string panel_border(const char *border_color = kFgMuted) {
        return std::string(border_color) + "+" +
               std::string(static_cast<size_t>(kPanelWidth - 2), '-') + "+" + kReset;
    }

    inline std::string scanner_row_color(const std::string &line) {
        if (line.find("detected") != std::string::npos || line.find("done") != std::string::npos)
            return kFgGood;
        if (line.find("failed") != std::string::npos || line.find("no_frame") != std::string::npos)
            return kFgWarn;
        if (line.find("skip") != std::string::npos)
            return kFgMuted;
        return kFgAccent;
    }

    inline const char *device_row_color(const std::string &line) {
        // オフライン状態は赤
        if (line.find("[OFF]") != std::string::npos)
            return kFgBad;

        // オンライン状態：複合的に色分け
        if (line.find("[ON ]") != std::string::npos) {
            double util = 0.0;
            double rx_hz = 0.0;
            int chk_err = 0, idm_err = 0, drop_err = 0;

            // 利用率をパース：最後の % の前の数字
            size_t util_pos = line.rfind('%');
            if (util_pos != std::string::npos && util_pos >= 2) {
                std::string util_str;
                size_t i = util_pos - 1;
                while (i > 0 && (std::isdigit(line[i]) || line[i] == '.')) {
                    util_str = line[i] + util_str;
                    i--;
                }
                try {
                    util = std::stod(util_str);
                } catch (...) {
                }
            }

            // RX Hz をパース：RX の後の数字
            size_t rx_pos = line.find("RX ");
            if (rx_pos != std::string::npos) {
                rx_pos += 3;
                std::string rx_str;
                while (rx_pos < line.size() && (std::isdigit(line[rx_pos]) || line[rx_pos] == '.')) {
                    rx_str += line[rx_pos];
                    rx_pos++;
                }
                try {
                    rx_hz = std::stod(rx_str);
                } catch (...) {
                }
            }

            // CHK エラーをパース（有効な場合）
            size_t chk_pos = line.find("CHK ");
            if (chk_pos != std::string::npos) {
                chk_pos += 4;
                std::string chk_str;
                while (chk_pos < line.size() && std::isdigit(line[chk_pos])) {
                    chk_str += line[chk_pos];
                    chk_pos++;
                }
                try {
                    chk_err = std::stoi(chk_str);
                } catch (...) {
                }
            }

            // IDM エラーをパース（有効な場合）
            size_t idm_pos = line.find("IDM ");
            if (idm_pos != std::string::npos) {
                idm_pos += 4;
                std::string idm_str;
                while (idm_pos < line.size() && std::isdigit(line[idm_pos])) {
                    idm_str += line[idm_pos];
                    idm_pos++;
                }
                try {
                    idm_err = std::stoi(idm_str);
                } catch (...) {
                }
            }

            // DROP エラーをパース（有効な場合）
            size_t drop_pos = line.find("DROP ");
            if (drop_pos != std::string::npos) {
                drop_pos += 5;
                std::string drop_str;
                while (drop_pos < line.size() && std::isdigit(line[drop_pos])) {
                    drop_str += line[drop_pos];
                    drop_pos++;
                }
                try {
                    drop_err = std::stoi(drop_str);
                } catch (...) {
                }
            }

            const int total_errors = chk_err + idm_err + drop_err;

            // 複合的に色分け
            // 赤：高負荷 OR エラーが相当ある
            if (util >= 80.0 || total_errors > 10)
                return kFgBad;

            // オレンジ：中程度負荷 OR エラーがある
            if (util >= 50.0 || total_errors > 0)
                return kFgWarnOrange;

            // 黄：低-中程度負荷 OR RX周波数低い
            if (util >= 20.0 || rx_hz < 1.0)
                return kFgWarn;

            // 緑系：低負荷
            if (util >= 5.0)
                return kFgGoodMid;

            // 直（很好）：ほぼ無負荷 AND エラーなし
            return kFgGood;
        }

        return kFgText;
    }

    inline std::vector<std::string> wrap_plain_text(const std::string &text, int width) {
        std::vector<std::string> lines;
        if (width <= 0) {
            lines.emplace_back("");
            return lines;
        }

        size_t i = 0;
        while (i < text.size()) {
            std::string part;
            int visible = 0;
            while (i < text.size() && visible < width) {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                size_t char_len = 1;
                if ((c & 0x80u) != 0u) {
                    size_t j = i + 1;
                    while (j < text.size() &&
                           is_utf8_continuation(static_cast<unsigned char>(text[j]))) {
                        j++;
                    }
                    char_len = j - i;
                }

                part.append(text, i, char_len);
                i += char_len;
                visible++;
            }

            if (visible < width) {
                part += std::string(static_cast<size_t>(width - visible), ' ');
            }
            lines.push_back(part);
        }

        if (lines.empty()) {
            lines.emplace_back(std::string(static_cast<size_t>(width), ' '));
        }
        return lines;
    }

    inline std::string colorize_scanner_line(const std::string &line) {
        if (line.find("detected") != std::string::npos || line.find("done") != std::string::npos)
            return std::string(kFgGood) + line + kReset;
        if (line.find("failed") != std::string::npos || line.find("no_frame") != std::string::npos)
            return std::string(kFgWarn) + line + kReset;
        if (line.find("skip") != std::string::npos)
            return std::string(kFgMuted) + line + kReset;
        return std::string(kFgAccent) + line + kReset;
    }

    inline std::string colorize_device_line(const std::string &line) {
        if (line.find("[OFF]") != std::string::npos)
            return std::string(kFgBad) + line + kReset;
        if (line.find("CHK 0") != std::string::npos &&
            line.find("TX(") != std::string::npos &&
            line.find("/0)") != std::string::npos)
            return std::string(kFgGood) + line + kReset;
        return std::string(kFgText) + line + kReset;
    }

    inline std::string spinner_frame(size_t tick) {
        static const char *frames[] = {
            "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
        return frames[tick % (sizeof(frames) / sizeof(frames[0]))];
    }

    inline std::string header_pulse(size_t tick) {
        std::ostringstream oss;
        oss << spinner_frame(tick) << " syncing";
        return oss.str();
    }

    inline std::string node_anim_icon(bool is_on, size_t tick) {
        if (!is_on)
            return std::string(kFgBad) + "◼" + kReset;

        return std::string(kFgAccent) + spinner_frame(tick) + kReset;
    }

    struct DashboardState {
        std::string scanner_line{"idle"};
        std::string detected_line{"(none)"};
        std::map<uint8_t, std::string> node_lines;
        std::map<uint8_t, size_t> node_ticks;
        size_t render_tick{0};
        bool initialized{false};
    };

    inline DashboardState g_state;
    inline std::mutex g_mutex;
    inline std::atomic<bool> g_anim_running{false};
    inline std::thread g_anim_thread;

    inline void render_locked();

    inline void start_animation_thread_locked() {
        if (g_anim_running.load())
            return;

        g_anim_running = true;
        g_anim_thread = std::thread([]() {
            while (g_anim_running.load()) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(std::max(16, serial_bridge::config::kGraphicalUiFrameMs)));

                std::lock_guard<std::mutex> lock(g_mutex);
                if (!g_state.initialized)
                    continue;
                render_locked();
            }
        });
    }

    inline bool enabled() {
        return serial_bridge::config::kLogOutputMode ==
               serial_bridge::config::LogOutputMode::kGraphical;
    }

    inline std::string format_device_id(uint8_t id) {
        return std::to_string(static_cast<unsigned>(id));
    }

    inline void render_locked() {
        if (!enabled())
            return;

        if (!g_state.initialized) {
            std::cout << "\033[?25l";
            g_state.initialized = true;
            start_animation_thread_locked();
        }

        std::cout << "\033[H\033[2J";
        g_state.render_tick++;
        std::cout << panel_border(kFgTitle) << "\n";
        std::cout << panel_line("serial_bridge live dashboard  ◉", kFgTitle, kFgTitle) << "\n";
        std::ostringstream mode_line;
        mode_line << "mode: graphical  |  style: modern-color  |  activity "
                  << std::string(kFgGood) << header_pulse(g_state.render_tick) << kReset;
        std::cout << panel_line(mode_line.str(), kFgMuted, kFgTitle) << "\n";
        std::cout << panel_border(kFgTitle) << "\n";
        std::cout << panel_line("SCAN", kFgAccent) << "\n";
        for (const auto &line : wrap_plain_text(g_state.scanner_line, kPanelWidth - 4)) {
            std::cout << panel_line(line, scanner_row_color(g_state.scanner_line).c_str()) << "\n";
        }
        std::cout << panel_line("DETECTED", kFgAccent) << "\n";
        for (const auto &line : wrap_plain_text(g_state.detected_line, kPanelWidth - 4)) {
            std::cout << panel_line(line, kFgGood) << "\n";
        }
        std::cout << panel_border() << "\n";
        std::cout << panel_line("DEVICES", kFgAccent) << "\n";

        if (g_state.node_lines.empty()) {
            std::cout << panel_line("(no active devices)", kFgMuted) << "\n";
        } else {
            for (const auto &[id, line] : g_state.node_lines) {
                const bool is_on = line.find("[ON ]") != std::string::npos;
                const size_t tick = g_state.node_ticks[id];
                std::ostringstream row;
                row << spinner_frame(tick)
                    << "  " << format_device_id(id) << "  " << line;
                const auto wrapped = wrap_plain_text(row.str(), kPanelWidth - 4);
                for (const auto &w : wrapped) {
                    std::cout << panel_line(w, device_row_color(line)) << "\n";
                }
            }
        }

        std::cout << panel_border() << "\n";
        std::cout << panel_line("tip: set LogOutputMode::kTerminal for classic logs", kFgMuted) << "\n";

        std::cout.flush();
    }

    inline void set_scanner_line(const std::string &line) {
        if (!enabled())
            return;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state.scanner_line = line;
        render_locked();
    }

    inline void set_detected_line(const std::string &line) {
        if (!enabled())
            return;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state.detected_line = line;
        render_locked();
    }

    inline void set_node_line(uint8_t id, const std::string &line) {
        if (!enabled())
            return;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state.node_lines[id] = line;
        const bool is_on = line.find("[ON ]") != std::string::npos;
        if (is_on) {
            g_state.node_ticks[id]++;
        } else {
            g_state.node_ticks[id] = 0;
        }
        render_locked();
    }

    inline void erase_node_line(uint8_t id) {
        if (!enabled())
            return;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state.node_lines.erase(id);
        g_state.node_ticks.erase(id);
        render_locked();
    }

    inline void shutdown() {
        if (!enabled())
            return;
        bool need_join = false;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_anim_running = false;
            need_join = g_anim_thread.joinable();

            if (!g_state.initialized) {
                if (need_join) {
                    // join はロック外で実行
                } else {
                    return;
                }
            } else {
                std::cout << "\033[?25h" << std::flush;
                g_state.initialized = false;
            }
        }

        if (need_join)
            g_anim_thread.join();
    }

} // namespace serial_bridge::graphical_ui
