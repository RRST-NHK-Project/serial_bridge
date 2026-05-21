#include "io.hpp"

#include "defs.hpp"
#include "frame_data.hpp"

#include <Arduino.h>
#include <SimpleFOC.h>

#ifdef __has_include
#if __has_include(<Servo.h>)
#include <Servo.h>
#define STM32_SB_HAVE_SERVO_LIB 1
#else
#define STM32_SB_HAVE_SERVO_LIB 0
#endif
#else
#define STM32_SB_HAVE_SERVO_LIB 0
#endif

namespace {
    int clamp_int(int v, int lo, int hi) {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

    int map_int(int x, int in_min, int in_max, int out_min, int out_max) {
        if (in_max == in_min)
            return out_min;
        const long num = static_cast<long>(x - in_min) * static_cast<long>(out_max - out_min);
        const long den = static_cast<long>(in_max - in_min);
        return static_cast<int>(num / den + out_min);
    }

    int servo_duty_from_deg(int deg, int min_deg, int max_deg, int min_us, int max_us) {
        const int a = clamp_int(deg, min_deg, max_deg);
        const int us = map_int(a, min_deg, max_deg, min_us, max_us);
        return static_cast<int>(us * SERVO_PWM_SCALE);
    }

    int read_switch(uint8_t pin) {
        const int v = digitalRead(pin);
#if SW_ACTIVE_LOW
        return (v == LOW) ? 1 : 0;
#else
        return (v == HIGH) ? 1 : 0;
#endif
    }

    // ================= Encoders (SimpleFOC) =================

    Encoder enc1(ENC1_A, ENC1_B, ENC_CPR);
    Encoder enc2(ENC2_A, ENC2_B, ENC_CPR);
    Encoder enc3(ENC3_A, ENC3_B, ENC_CPR);
    Encoder enc4(ENC4_A, ENC4_B, ENC_CPR);
    Encoder enc5(ENC5_A, ENC5_B, ENC_CPR);
    Encoder enc6(ENC6_A, ENC6_B, ENC_CPR);
    Encoder enc7(ENC7_A, ENC7_B, ENC_CPR);
    Encoder enc8(ENC8_A, ENC8_B, ENC_CPR);

    void enc1A() { enc1.handleA(); }
    void enc1B() { enc1.handleB(); }
    void enc2A() { enc2.handleA(); }
    void enc2B() { enc2.handleB(); }
    void enc3A() { enc3.handleA(); }
    void enc3B() { enc3.handleB(); }
    void enc4A() { enc4.handleA(); }
    void enc4B() { enc4.handleB(); }
    void enc5A() { enc5.handleA(); }
    void enc5B() { enc5.handleB(); }
    void enc6A() { enc6.handleA(); }
    void enc6B() { enc6.handleB(); }
    void enc7A() { enc7.handleA(); }
    void enc7B() { enc7.handleB(); }
    void enc8A() { enc8.handleA(); }
    void enc8B() { enc8.handleB(); }

    // ================= Servos =================

#if STM32_SB_HAVE_SERVO_LIB
    Servo servo1;
    Servo servo2;
    Servo servo3;
    Servo servo4;
    Servo servo5;
    Servo servo6;
    Servo servo7;
    Servo servo8;

    int servo_us_from_deg(int deg, int min_deg, int max_deg, int min_us, int max_us) {
        const int a = clamp_int(deg, min_deg, max_deg);
        return map_int(a, min_deg, max_deg, min_us, max_us);
    }
#endif

    void output_init() {
        pinMode(MD1D, OUTPUT);
        pinMode(MD2D, OUTPUT);
        pinMode(MD3D, OUTPUT);
        pinMode(MD4D, OUTPUT);
        pinMode(MD5D, OUTPUT);
        pinMode(MD6D, OUTPUT);
        pinMode(MD7D, OUTPUT);
        pinMode(MD8D, OUTPUT);

        pinMode(TR1, OUTPUT);
        pinMode(TR2, OUTPUT);
        pinMode(TR3, OUTPUT);
        pinMode(TR4, OUTPUT);
        pinMode(TR5, OUTPUT);
        pinMode(TR6, OUTPUT);
        pinMode(TR7, OUTPUT);

#if STM32_SB_HAVE_SERVO_LIB
        servo1.attach(SERVO1);
        servo2.attach(SERVO2);
        servo3.attach(SERVO3);
        servo4.attach(SERVO4);
        servo5.attach(SERVO5);
        servo6.attach(SERVO6);
        servo7.attach(SERVO7);
        servo8.attach(SERVO8);

        servo1.writeMicroseconds(servo_us_from_deg(SERVO1_INIT_DEG, SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
        servo2.writeMicroseconds(servo_us_from_deg(SERVO2_INIT_DEG, SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
        servo3.writeMicroseconds(servo_us_from_deg(SERVO3_INIT_DEG, SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
        servo4.writeMicroseconds(servo_us_from_deg(SERVO4_INIT_DEG, SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
        servo5.writeMicroseconds(servo_us_from_deg(SERVO5_INIT_DEG, SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
        servo6.writeMicroseconds(servo_us_from_deg(SERVO6_INIT_DEG, SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
        servo7.writeMicroseconds(servo_us_from_deg(SERVO7_INIT_DEG, SERVO7_MIN_DEG, SERVO7_MAX_DEG, SERVO7_MIN_US, SERVO7_MAX_US));
        servo8.writeMicroseconds(servo_us_from_deg(SERVO8_INIT_DEG, SERVO8_MIN_DEG, SERVO8_MAX_DEG, SERVO8_MIN_US, SERVO8_MAX_US));
#else
        // Fallback: generate servo PWM by analogWrite duty.
        analogWriteFrequency(SERVO_PWM_FREQ);
        analogWriteResolution(SERVO_PWM_RESOLUTION);

        analogWrite(SERVO1, servo_duty_from_deg(SERVO1_INIT_DEG, SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
        analogWrite(SERVO2, servo_duty_from_deg(SERVO2_INIT_DEG, SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
        analogWrite(SERVO3, servo_duty_from_deg(SERVO3_INIT_DEG, SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
        analogWrite(SERVO4, servo_duty_from_deg(SERVO4_INIT_DEG, SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
        analogWrite(SERVO5, servo_duty_from_deg(SERVO5_INIT_DEG, SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
        analogWrite(SERVO6, servo_duty_from_deg(SERVO6_INIT_DEG, SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
        analogWrite(SERVO7, servo_duty_from_deg(SERVO7_INIT_DEG, SERVO7_MIN_DEG, SERVO7_MAX_DEG, SERVO7_MIN_US, SERVO7_MAX_US));
        analogWrite(SERVO8, servo_duty_from_deg(SERVO8_INIT_DEG, SERVO8_MIN_DEG, SERVO8_MAX_DEG, SERVO8_MIN_US, SERVO8_MAX_US));
#endif
    }

    void input_init() {
        enc1.init();
        enc1.enableInterrupts(enc1A, enc1B);
        enc2.init();
        enc2.enableInterrupts(enc2A, enc2B);
        enc3.init();
        enc3.enableInterrupts(enc3A, enc3B);
        enc4.init();
        enc4.enableInterrupts(enc4A, enc4B);
        enc5.init();
        enc5.enableInterrupts(enc5A, enc5B);
        enc6.init();
        enc6.enableInterrupts(enc6A, enc6B);
        enc7.init();
        enc7.enableInterrupts(enc7A, enc7B);
        enc8.init();
        enc8.enableInterrupts(enc8A, enc8B);

        pinMode(SW1, INPUT_PULLUP);
        pinMode(SW2, INPUT_PULLUP);
        pinMode(SW3, INPUT_PULLUP);
        pinMode(SW4, INPUT_PULLUP);
        pinMode(SW5, INPUT_PULLUP);
        pinMode(SW6, INPUT_PULLUP);
        pinMode(SW7, INPUT_PULLUP);
        pinMode(SW8, INPUT_PULLUP);
    }

    void md_output() {
        const int v1 = clamp_int(Rx_16Data[1], -MD_PWM_MAX, MD_PWM_MAX);
        const int v2 = clamp_int(Rx_16Data[2], -MD_PWM_MAX, MD_PWM_MAX);
        const int v3 = clamp_int(Rx_16Data[3], -MD_PWM_MAX, MD_PWM_MAX);
        const int v4 = clamp_int(Rx_16Data[4], -MD_PWM_MAX, MD_PWM_MAX);
        const int v5 = clamp_int(Rx_16Data[5], -MD_PWM_MAX, MD_PWM_MAX);
        const int v6 = clamp_int(Rx_16Data[6], -MD_PWM_MAX, MD_PWM_MAX);
        const int v7 = clamp_int(Rx_16Data[7], -MD_PWM_MAX, MD_PWM_MAX);
        const int v8 = clamp_int(Rx_16Data[8], -MD_PWM_MAX, MD_PWM_MAX);

        digitalWrite(MD1D, v1 >= 0 ? HIGH : LOW);
        digitalWrite(MD2D, v2 >= 0 ? HIGH : LOW);
        digitalWrite(MD3D, v3 >= 0 ? HIGH : LOW);
        digitalWrite(MD4D, v4 >= 0 ? HIGH : LOW);
        digitalWrite(MD5D, v5 >= 0 ? HIGH : LOW);
        digitalWrite(MD6D, v6 >= 0 ? HIGH : LOW);
        digitalWrite(MD7D, v7 >= 0 ? HIGH : LOW);
        digitalWrite(MD8D, v8 >= 0 ? HIGH : LOW);

        analogWrite(MD1P, abs(v1));
        analogWrite(MD2P, abs(v2));
        analogWrite(MD3P, abs(v3));
        analogWrite(MD4P, abs(v4));
        analogWrite(MD5P, abs(v5));
        analogWrite(MD6P, abs(v6));
        analogWrite(MD7P, abs(v7));
        analogWrite(MD8P, abs(v8));
    }

    void servo_output() {
#if STM32_SB_HAVE_SERVO_LIB
        servo1.writeMicroseconds(servo_us_from_deg(Rx_16Data[9], SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
        servo2.writeMicroseconds(servo_us_from_deg(Rx_16Data[10], SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
        servo3.writeMicroseconds(servo_us_from_deg(Rx_16Data[11], SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
        servo4.writeMicroseconds(servo_us_from_deg(Rx_16Data[12], SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
        servo5.writeMicroseconds(servo_us_from_deg(Rx_16Data[13], SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
        servo6.writeMicroseconds(servo_us_from_deg(Rx_16Data[14], SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
        servo7.writeMicroseconds(servo_us_from_deg(Rx_16Data[15], SERVO7_MIN_DEG, SERVO7_MAX_DEG, SERVO7_MIN_US, SERVO7_MAX_US));
        servo8.writeMicroseconds(servo_us_from_deg(Rx_16Data[16], SERVO8_MIN_DEG, SERVO8_MAX_DEG, SERVO8_MIN_US, SERVO8_MAX_US));
#else
        analogWriteFrequency(SERVO_PWM_FREQ);
        analogWriteResolution(SERVO_PWM_RESOLUTION);

        analogWrite(SERVO1, servo_duty_from_deg(Rx_16Data[9], SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
        analogWrite(SERVO2, servo_duty_from_deg(Rx_16Data[10], SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
        analogWrite(SERVO3, servo_duty_from_deg(Rx_16Data[11], SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
        analogWrite(SERVO4, servo_duty_from_deg(Rx_16Data[12], SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
        analogWrite(SERVO5, servo_duty_from_deg(Rx_16Data[13], SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
        analogWrite(SERVO6, servo_duty_from_deg(Rx_16Data[14], SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
        analogWrite(SERVO7, servo_duty_from_deg(Rx_16Data[15], SERVO7_MIN_DEG, SERVO7_MAX_DEG, SERVO7_MIN_US, SERVO7_MAX_US));
        analogWrite(SERVO8, servo_duty_from_deg(Rx_16Data[16], SERVO8_MIN_DEG, SERVO8_MAX_DEG, SERVO8_MIN_US, SERVO8_MAX_US));
#endif
    }

    void tr_output() {
        digitalWrite(TR1, Rx_16Data[17] ? HIGH : LOW);
        digitalWrite(TR2, Rx_16Data[18] ? HIGH : LOW);
        digitalWrite(TR3, Rx_16Data[19] ? HIGH : LOW);
        digitalWrite(TR4, Rx_16Data[20] ? HIGH : LOW);
        digitalWrite(TR5, Rx_16Data[21] ? HIGH : LOW);
        digitalWrite(TR6, Rx_16Data[22] ? HIGH : LOW);
        digitalWrite(TR7, Rx_16Data[23] ? HIGH : LOW);
    }

    void enc_input() {
        // Angle(rad) * 1000 -> int16 (same style as existing stm32_serial_bridge WIP)
        Tx_16Data[1] = static_cast<int16_t>(enc1.getAngle() * 1000.0f);
        Tx_16Data[2] = static_cast<int16_t>(enc2.getAngle() * 1000.0f);
        Tx_16Data[3] = static_cast<int16_t>(enc3.getAngle() * 1000.0f);
        Tx_16Data[4] = static_cast<int16_t>(enc4.getAngle() * 1000.0f);
        Tx_16Data[5] = static_cast<int16_t>(enc5.getAngle() * 1000.0f);
        Tx_16Data[6] = static_cast<int16_t>(enc6.getAngle() * 1000.0f);
        Tx_16Data[7] = static_cast<int16_t>(enc7.getAngle() * 1000.0f);
        Tx_16Data[8] = static_cast<int16_t>(enc8.getAngle() * 1000.0f);
    }

    void sw_input() {
        Tx_16Data[9] = read_switch(SW1);
        Tx_16Data[10] = read_switch(SW2);
        Tx_16Data[11] = read_switch(SW3);
        Tx_16Data[12] = read_switch(SW4);
        Tx_16Data[13] = read_switch(SW5);
        Tx_16Data[14] = read_switch(SW6);
        Tx_16Data[15] = read_switch(SW7);
        Tx_16Data[16] = read_switch(SW8);
    }
}

void io_init() {
#if defined(MODE_FULL_IO)
    output_init();
    input_init();
#else
#error "MODE_FULL_IO must be defined for this firmware"
#endif
}

void io_update() {
#if defined(MODE_FULL_IO)
    md_output();
    servo_output();
    tr_output();
    enc_input();
    sw_input();
#endif
}
