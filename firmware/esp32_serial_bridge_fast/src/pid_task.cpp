#include "pid_task.hpp"

#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"

#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_timer.h"

#include <Arduino.h>

namespace {
    TaskHandle_t m2006_task_handle = nullptr;
    esp_timer_handle_t m2006_timer = nullptr;

    int encoder_count[kNumMotor] = {0};
    int rpm[kNumMotor] = {0};
    int current[kNumMotor] = {0};
    int last_encoder[kNumMotor] = {0};
    int rotation_count[kNumMotor] = {0};
    long total_encoder[kNumMotor] = {0};

    float target_rpm[kNumMotor] = {0.0f};
    float vel_m2006[kNumMotor] = {0.0f};
    float angle_m2006[kNumMotor] = {0.0f};
    float c_m2006[kNumMotor] = {0.0f};

    float vel_error_prev[kNumMotor] = {0.0f};
    float vel_prop_prev[kNumMotor] = {0.0f};
    float vel_output[kNumMotor] = {0.0f};
    float motor_output_current[kNumMotor] = {0.0f};

    float constrain_double(float val, float min_val, float max_val) {
        if (val < min_val)
            return min_val;
        if (val > max_val)
            return max_val;
        return val;
    }

    float pid_vel(float setpoint, float input, float &error_prev, float &prop_prev, float &output,
                  float kp, float ki, float kd, float dt) {
        const float error = setpoint - input;
        const float prop = error - error_prev;
        const float deriv = prop - prop_prev;
        const float du = kp * prop + ki * error * dt + kd * deriv;
        output += du;

        prop_prev = prop;
        error_prev = error;

        return output;
    }

    void send_cur_c610(float cur_array[kNumMotor]) {
        twai_message_t tx;
        tx.identifier = 0x200;
        tx.extd = 0;
        tx.rtr = 0;
        tx.data_length_code = 8;

        constexpr float max_cur = M2006_MAX_CUR_A;
        constexpr int16_t max_cur_val = 10000;

        for (int i = 0; i < kNumMotor; i++) {
            float amp = constrain_double(cur_array[i], -max_cur, max_cur);
            int16_t val = static_cast<int16_t>(amp * (max_cur_val / max_cur));

            tx.data[i * 2] = (val >> 8) & 0xFF;
            tx.data[i * 2 + 1] = val & 0xFF;
        }

        if (twai_transmit(&tx, pdMS_TO_TICKS(20)) != ESP_OK) {
            Serial.println("[ERR] twai_transmit failed");
        }
    }

    void twai_receive_feedback() {
        twai_message_t rx_msg;

        while (twai_receive(&rx_msg, 0) == ESP_OK) {
            if (rx_msg.data_length_code != 8) {
                continue;
            }
            if (rx_msg.identifier < 0x201 || rx_msg.identifier > 0x204) {
                continue;
            }

            const int m = rx_msg.identifier - 0x201;
            if (m < 0 || m >= kNumMotor) {
                continue;
            }

            encoder_count[m] = static_cast<int16_t>(rx_msg.data[0] << 8 | rx_msg.data[1]);
            rpm[m] = static_cast<int16_t>(rx_msg.data[2] << 8 | rx_msg.data[3]);
            current[m] = static_cast<int16_t>(rx_msg.data[4] << 8 | rx_msg.data[5]);

            const int diff = encoder_count[m] - last_encoder[m];
            if (diff > kHalfEncoder) {
                rotation_count[m]--;
            } else if (diff < -kHalfEncoder) {
                rotation_count[m]++;
            }

            last_encoder[m] = encoder_count[m];

            total_encoder[m] = rotation_count[m] * kEncoderMax + encoder_count[m];
            angle_m2006[m] = total_encoder[m] * (360.0f / (kEncoderMax * kGearM2006));
            vel_m2006[m] = rpm[m] / kGearM2006;
            c_m2006[m] = current[m] * 10.0f / 10000.0f;
        }
    }

    void control_step() {
        for (int i = 0; i < kNumMotor; i++) {
            target_rpm[i] = static_cast<float>(Rx_16Data[i + 5]);
        }

        twai_receive_feedback();

        for (int i = 0; i < kNumMotor; i++) {
            c_m2006[i] = pid_vel(target_rpm[i], vel_m2006[i], vel_error_prev[i],
                                 vel_prop_prev[i], vel_output[i],
                                 KP_VEL_M2006, KI_VEL_M2006, KD_VEL_M2006,
                                 kControlPeriodSec);

            motor_output_current[i] = constrain_double(c_m2006[i], -M2006_MAX_CUR_A, M2006_MAX_CUR_A);
        }

        send_cur_c610(motor_output_current);

        for (int i = 0; i < kNumMotor; i++) {
            Tx_16Data[i + 5] = static_cast<int16_t>(vel_m2006[i]);
            Tx_16Data[i + 13] = static_cast<int16_t>(angle_m2006[i]);
        }
    }

    void m2006_timer_cb(void *) {
        if (m2006_task_handle != nullptr) {
            xTaskNotifyGive(m2006_task_handle);
        }
    }
}

void robomas_init() {
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        while (1) {
        }
    }
    if (twai_start() != ESP_OK) {
        while (1) {
        }
    }
}

void M2006_Task(void *) {
    m2006_task_handle = xTaskGetCurrentTaskHandle();

    const esp_timer_create_args_t timer_args = {
        .callback = &m2006_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "m2006_1khz"};

    if (esp_timer_create(&timer_args, &m2006_timer) == ESP_OK) {
        esp_timer_start_periodic(m2006_timer, CONTROL_PERIOD_US);
    }

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        control_step();
    }
}
