#include "serial_bridge/bridge_node.hpp"
#include "serial_bridge/config.hpp"
#include "serial_bridge/graphical_ui.hpp"
#include "serial_bridge/port_scanner.hpp"
#include <rclcpp/rclcpp.hpp>

#include <rcutils/logging.h>

#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <thread>

int main(int argc, char *argv[]) {
    if (serial_bridge::config::kLogOutputMode ==
            serial_bridge::config::LogOutputMode::kNone ||
        serial_bridge::config::kLogOutputMode ==
            serial_bridge::config::LogOutputMode::kGraphical) {
        rcutils_logging_set_default_logger_level(RCUTILS_LOG_SEVERITY_FATAL);
    }

    rclcpp::init(argc, argv);

    auto debug_node = std::make_shared<rclcpp::Node>("serial_bridge_status");

    // ---------- パラメータ宣言 ----------
    debug_node->declare_parameter("excluded_ports", std::vector<std::string>{});
    debug_node->declare_parameter("rx_timeout_sec", 2.0);
    debug_node->declare_parameter("reconnect_interval_sec", 3.0);
    debug_node->declare_parameter("scan_interval_ms", 5000);

    // ---------- パラメータ読み込み ----------
    auto excluded_ports_vec =
        debug_node->get_parameter("excluded_ports").as_string_array();
    const std::set<std::string> excluded_ports(excluded_ports_vec.begin(),
                                               excluded_ports_vec.end());
    const double rx_timeout_sec =
        debug_node->get_parameter("rx_timeout_sec").as_double();
    const double reconnect_interval_sec =
        debug_node->get_parameter("reconnect_interval_sec").as_double();
    const int scan_interval_ms =
        debug_node->get_parameter("scan_interval_ms").as_int();

    if (!excluded_ports.empty()) {
        for (const auto &p : excluded_ports)
            RCLCPP_INFO(debug_node->get_logger(), "Excluded port: %s", p.c_str());
    }

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(debug_node);

    // デバイスID → ノード の管理マップ（スキャンスレッドと共有）
    std::map<uint8_t, std::shared_ptr<SerialBridgeNode>> node_map;
    std::set<std::string> known_ports;
    std::mutex map_mutex;
    std::atomic<bool> running{true};

    // バックグラウンドスキャンスレッド
    // 未検出のデバイスを定期的に探索し、新規デバイスが見つかれば動的にノードを追加する
    std::thread scanner([&]() {
        while (running && rclcpp::ok()) {
            // 既使用ポートを除外してスキャン（データ競合防止）
            std::set<std::string> skip;
            {
                std::lock_guard<std::mutex> lock(map_mutex);
                skip = known_ports;
            }

            auto devices = detect_serial_devices(skip, excluded_ports);

            {
                std::lock_guard<std::mutex> lock(map_mutex);
                for (auto &[id, port] : devices) {
                    auto it = node_map.find(id);
                    if (it == node_map.end()) {
                        // 未知のデバイス: 新規ノードを作成
                        RCLCPP_INFO(debug_node->get_logger(),
                                    "New device found: ID=%u port=%s",
                                    static_cast<unsigned>(id), port.c_str());
                        auto node = std::make_shared<SerialBridgeNode>(id, port,
                                                                       rx_timeout_sec,
                                                                       reconnect_interval_sec);
                        node_map[id] = node;
                        known_ports.insert(port);
                        executor.add_node(node);
                    } else if (!it->second->is_connected()) {
                        // 既知のデバイスが切断中に別ポートで再検出された: ノードを作り直す
                        // 注: is_connected() は atomic だが map_mutex とは同期しない。
                        // タイミングによって接続直後のノードを置き換える可能性がある（極めて稀）が、
                        // 新ノードが即座に再接続するため実害はない。
                        RCLCPP_INFO(debug_node->get_logger(),
                                    "Device ID=%u reconnected on new port %s (was %s) — replacing node",
                                    static_cast<unsigned>(id), port.c_str(), it->second->get_port().c_str());
                        executor.remove_node(it->second);
                        known_ports.erase(it->second->get_port());
                        auto node = std::make_shared<SerialBridgeNode>(id, port,
                                                                       rx_timeout_sec,
                                                                       reconnect_interval_sec);
                        it->second = node;
                        known_ports.insert(port);
                        executor.add_node(node);
                    }
                    // 接続中かつ既知: 何もしない
                }
            }

            // 次のスキャンまで待機（100ms 刻みで running フラグを確認）
            for (int i = 0; i < scan_interval_ms / 100 && running && rclcpp::ok(); i++)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    executor.spin();

    running = false;
    scanner.join();

    serial_bridge::graphical_ui::shutdown();

    rclcpp::shutdown();
    return 0;
}
