/*====================================================================
<>
・使用しているMDとswが異なるので注意!!!
Copyright (c) 2025 RRST-NHK-Project. All rights reserved.
====================================================================*/
#include "driver/pcnt.h"
#include "frame_data.hpp"
#include "pin_ctrl_init.hpp"
#include <Arduino.h>
#include "natsu_pid_task.hpp"

#include "config.hpp"

// エンコーダのDIPスイッチをすべてoffにすること
//  ================= TASK =================

// PID制御タスク馬渕735
void PID_Task(void *) {
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        //pid_control();
        pid_vel_control();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CTRL_PERIOD_MS));
    }
}

float pid_calculate(float enc, float last_enc, float &last_err, float &integral,
                    float kp, float ki, float kd, float dt) {
    
    float err = enc - last_enc;
    float P = kp * err;

    integral += (err + last_err * dt);
    float I = ki * integral;
    
    float derivative = (err - last_err) / dt;
    float D = kd * derivative;

    last_err = err;
    return P + I + D;
}

// PID制御関数
void pid_vel_control() {

    float last_angle[4];

    float dt = CTRL_PERIOD_MS / 1000.0f;

    int16_t cnt0, cnt1, cnt2, cnt3;
    static int32_t total_cnt0 = 0;
    static int32_t total_cnt1 = 0;
    static int32_t total_cnt2 = 0;
    static int32_t total_cnt3 = 0;
    static float target_angle_cur[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    static float last_enc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    static float vel_last_error[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    static float vel_integral[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    float vel[4];
    // ===== 目標速度 rpm → deg/s =====
    float target_v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    target_v[0] = Rx_16Data[1] * 6.0f; // rpm → deg/s
    target_v[1] = Rx_16Data[2] * 6.0f;
    target_v[2] = Rx_16Data[3] * 6.0f;
    target_v[3] = Rx_16Data[4] * 6.0f;

    pcnt_get_counter_value(PCNT_UNIT_0, &cnt0);
    pcnt_get_counter_value(PCNT_UNIT_1, &cnt1);
    pcnt_get_counter_value(PCNT_UNIT_2, &cnt2);
    pcnt_get_counter_value(PCNT_UNIT_3, &cnt3);

    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_2);
    pcnt_counter_clear(PCNT_UNIT_3);

    total_cnt0 += cnt0;
    total_cnt1 += cnt1;
    total_cnt2 += cnt2;
    total_cnt3 += cnt3;

    angle[0] = total_cnt0 * DEG_PER_COUNT;
    angle[1] = total_cnt1 * DEG_PER_COUNT;
    angle[2] = total_cnt2 * DEG_PER_COUNT;
    angle[3] = total_cnt3 * DEG_PER_COUNT;

    // 角速度計算
    vel[0] = (angle[0] - last_angle[0]) / dt;
    vel[1] = (angle[1] - last_angle[1]) / dt;
    vel[2] = (angle[2] - last_angle[2]) / dt;
    vel[3] = (angle[3] - last_angle[3]) / dt;

    last_angle[0] = angle[0];
    last_angle[1] = angle[1];
    last_angle[2] = angle[2];
    last_angle[3] = angle[3];

    // オーバーフロー対策が甘いがとりあえずそのまま送る
    // Tx_16Data[1] = static_cast<int16_t>(angle[0]);
    // Tx_16Data[2] = static_cast<int16_t>(angle[1]);
    // Tx_16Data[3] = static_cast<int16_t>(angle[2]);
    // Tx_16Data[4] = static_cast<int16_t>(angle[3]);
    // Tx_16Data[9] = static_cast<int16_t>(vel[0]);
    // Tx_16Data[10] = static_cast<int16_t>(vel[1]);
    // Tx_16Data[11] = static_cast<int16_t>(vel[2]);
    // Tx_16Data[12] = static_cast<int16_t>(vel[3]);

    output[0] = pid_calculate(target_v[0], vel[0], vel_last_error[0], vel_integral[0], Kp, Ki, Kd, dt);

    output[1] = pid_calculate(target_v[1], vel[1], vel_last_error[1], vel_integral[1], Kp, Ki, Kd, dt);

    output[2] = pid_calculate(target_v[2], vel[2], vel_last_error[2], vel_integral[2], Kp, Ki, Kd, dt);

    output[3] = pid_calculate(target_v[3], vel[3], vel_last_error[3], vel_integral[3], Kp, Ki, Kd, dt);

    output[0] = constrain(output[0], -MD_PWM_MAX, MD_PWM_MAX);
    output[1] = constrain(output[1], -MD_PWM_MAX, MD_PWM_MAX);
    output[2] = constrain(output[2], -MD_PWM_MAX, MD_PWM_MAX);
    output[3] = constrain(output[3], -MD_PWM_MAX, MD_PWM_MAX);

    digitalWrite(MD1D, output[0] > 0 ? HIGH : LOW);
    digitalWrite(MD2D, output[1] > 0 ? HIGH : LOW);
    digitalWrite(MD3D, output[2] > 0 ? HIGH : LOW);
    digitalWrite(MD4D, output[3] > 0 ? HIGH : LOW);

    ledcWrite(0, abs(output[0]));
    ledcWrite(1, abs(output[1]));
    ledcWrite(2, abs(output[2]));
    ledcWrite(3, abs(output[3]));
}


   float target_v[4] = {0.0, 0.0, 0.0, 0.0};
   float last_target_v[4] = {0.0, 0.0, 0.0, 0.0};
   float err[4] = {0.0, 0.0, 0.0, 0.0};
   float last_err[4] = {0.0, 0.0, 0.0, 0.0};
   float err_diff[4] = {0.0, 0.0, 0.0, 0.0};
   float err_sum[4] = {0.0, 0.0, 0.0, 0.0};
   float rps[4] = {0.0, 0.0, 0.0, 0.0};
   float FF[4] = {0.0, 0.0, 0.0, 0.0};
   float P[4] = {0.0, 0.0, 0.0, 0.0}, I[4] = {0.0, 0.0, 0.0, 0.0}, D[4] = {0.0, 0.0, 0.0, 0.0};
   float motor_power[4] = {0.0, 0.0, 0.0, 0.0};

/*

// PID制御関数はとりあえず速度制御だけするつもりだからひとまずコメントアウト
void pid_control() {
    //////////////定義
    float kp = 1.0; // 3.0// Rx_16Data[21];
    float ki = 0.0; // Rx_16Data[22];
    float kd = 0.3; // 0.1 Rx_16Data[23];
    float dt = CTRL_PERIOD_MS / 1000.0f;

    int16_t cnt0, cnt1;
    static int32_t total_cnt0 = 0;
    static int32_t total_cnt1 = 0;
    static float target_angle_cur[2] = {0.0f, 0.0f};

    pcnt_get_counter_value(PCNT_UNIT_0, &cnt0);
    pcnt_get_counter_value(PCNT_UNIT_1, &cnt1);

    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);

    total_cnt0 += cnt0;
    total_cnt1 -= cnt1;

    angle[0] = total_cnt0 * DEG_PER_COUNT;
    angle[1] = total_cnt1 * DEG_PER_COUNT;

    ////////////////////
    // 起動時の調整
    static bool first = true;
    if (first) {
        target_angle_cur[0] = angle[0];
        target_angle_cur[1] = angle[1];
        first = false;
    }
    // スイッチでゼロリセット

    if (Rx_16Data[5] == 1) {
        total_cnt0 = 0;
        angle[0] = 0.0f;
        target_angle_cur[0] = 0.0f;
        pos_integral[0] = 0.0f;
        pos_error_prev[0] = 0.0f;
    }
    if (Rx_16Data[6] == 1) {
        total_cnt1 = 0;
        angle[1] = 0.0f;
        target_angle_cur[1] = 0.0f;
        pos_integral[1] = 0.0f;
        pos_error_prev[1] = 0.0f;
    }

    // オーバーフロー対策が甘いがとりあえずそのまま送る
    Tx_16Data[1] = static_cast<int16_t>(angle[0]);
    Tx_16Data[2] = static_cast<int16_t>(angle[1]);
    Tx_16Data[11] = digitalRead(SW3);
    Tx_16Data[12] = digitalRead(SW4);

    // ===== 360度オーバーフロー処理 =====
    // if (Rx_16Data[3] > 300.0f)
    // {
    //     angle[0] -= 360.0f;
    //     target_angle_cur[0] -= 360.0f;
    // }
    // else if (Rx_16Data[4] > 300.0f)
    // {
    //     angle[1] -= 360.0f;
    //     target_angle_cur[1] -= 360.0f;
    // }

    // ===== 目標角ランプ生成 =====
    target_angle[0] = Rx_16Data[1];
    target_angle[1] = Rx_16Data[2];

    // ランプ後の目標角度
    constexpr float MAX_STEP_DEG = 0.2f;

    target_angle_cur[0] += constrain(target_angle[0] - target_angle_cur[0], -MAX_STEP_DEG, +MAX_STEP_DEG);
    target_angle_cur[1] += constrain(target_angle[1] - target_angle_cur[1], -MAX_STEP_DEG, +MAX_STEP_DEG);

    output[0] = pid_calculate(target_angle_cur[0], angle[0], pos_error_prev[0], pos_integral[0],
                              kp, ki, kd, dt);
    output[0] = constrain(output[0], -MD_PWM_MAX, MD_PWM_MAX);

    output[1] = pid_calculate(target_angle_cur[1], angle[1], pos_error_prev[1], pos_integral[1],
                              kp, ki, kd, dt);
    output[1] = constrain(output[1], -MD_PWM_MAX, MD_PWM_MAX);

    digitalWrite(MD1D, output[0] > 0 ? HIGH : LOW);
    digitalWrite(MD2D, output[1] > 0 ? HIGH : LOW);

    ledcWrite(0, abs(output[0]));
    ledcWrite(1, abs(output[1]));
}*/