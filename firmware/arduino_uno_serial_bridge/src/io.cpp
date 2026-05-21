#include "io.hpp"

#include "config.hpp"
#include "defs.hpp"
#include "frame_data.hpp"

#include <Arduino.h>
#include <Servo.h>

namespace {
#if ENABLE_SERVO1
    Servo servo1;
#endif
#if ENABLE_SERVO2
    Servo servo2;
#endif
#if ENABLE_SERVO3
    Servo servo3;
#endif
#if ENABLE_SERVO4
    Servo servo4;
#endif
#if ENABLE_SERVO5
    Servo servo5;
#endif
#if ENABLE_SERVO6
    Servo servo6;
#endif

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

    int servo_us_from_deg(int deg, int min_deg, int max_deg, int min_us, int max_us) {
        const int a = clamp_int(deg, min_deg, max_deg);
        return map_int(a, min_deg, max_deg, min_us, max_us);
    }

    int read_switch(uint8_t pin) {
        const int v = digitalRead(pin);
#if SW_ACTIVE_LOW
        return (v == LOW) ? 1 : 0;
#else
        return (v == HIGH) ? 1 : 0;
#endif
    }
}

void io_init() {
#if defined(MODE_IO)
#if ENABLE_SERVO1
    servo1.attach(SERVO1_PIN);
    servo1.writeMicroseconds(servo_us_from_deg(SERVO1_INIT_DEG, SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
#endif
#if ENABLE_SERVO2
    servo2.attach(SERVO2_PIN);
    servo2.writeMicroseconds(servo_us_from_deg(SERVO2_INIT_DEG, SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
#endif
#if ENABLE_SERVO3
    servo3.attach(SERVO3_PIN);
    servo3.writeMicroseconds(servo_us_from_deg(SERVO3_INIT_DEG, SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
#endif
#if ENABLE_SERVO4
    servo4.attach(SERVO4_PIN);
    servo4.writeMicroseconds(servo_us_from_deg(SERVO4_INIT_DEG, SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
#endif
#if ENABLE_SERVO5
    servo5.attach(SERVO5_PIN);
    servo5.writeMicroseconds(servo_us_from_deg(SERVO5_INIT_DEG, SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
#endif
#if ENABLE_SERVO6
    servo6.attach(SERVO6_PIN);
    servo6.writeMicroseconds(servo_us_from_deg(SERVO6_INIT_DEG, SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
#endif

    pinMode(SW1_PIN, INPUT_PULLUP);
    pinMode(SW2_PIN, INPUT_PULLUP);
    pinMode(SW3_PIN, INPUT_PULLUP);
    pinMode(SW4_PIN, INPUT_PULLUP);
    pinMode(SW5_PIN, INPUT_PULLUP);
    pinMode(SW6_PIN, INPUT_PULLUP);
    pinMode(SW7_PIN, INPUT_PULLUP);
    pinMode(SW8_PIN, INPUT_PULLUP);
#else
#error "MODE_IO must be defined for this firmware"
#endif
}

void io_update() {
#if ENABLE_SERVO1
    // SERVOx angle commands are slots 9..16.
    servo1.writeMicroseconds(servo_us_from_deg(Rx_16Data[9], SERVO1_MIN_DEG, SERVO1_MAX_DEG, SERVO1_MIN_US, SERVO1_MAX_US));
#endif
#if ENABLE_SERVO2
    servo2.writeMicroseconds(servo_us_from_deg(Rx_16Data[10], SERVO2_MIN_DEG, SERVO2_MAX_DEG, SERVO2_MIN_US, SERVO2_MAX_US));
#endif
#if ENABLE_SERVO3
    servo3.writeMicroseconds(servo_us_from_deg(Rx_16Data[11], SERVO3_MIN_DEG, SERVO3_MAX_DEG, SERVO3_MIN_US, SERVO3_MAX_US));
#endif
#if ENABLE_SERVO4
    servo4.writeMicroseconds(servo_us_from_deg(Rx_16Data[12], SERVO4_MIN_DEG, SERVO4_MAX_DEG, SERVO4_MIN_US, SERVO4_MAX_US));
#endif
#if ENABLE_SERVO5
    servo5.writeMicroseconds(servo_us_from_deg(Rx_16Data[13], SERVO5_MIN_DEG, SERVO5_MAX_DEG, SERVO5_MIN_US, SERVO5_MAX_US));
#endif
#if ENABLE_SERVO6
    servo6.writeMicroseconds(servo_us_from_deg(Rx_16Data[14], SERVO6_MIN_DEG, SERVO6_MAX_DEG, SERVO6_MIN_US, SERVO6_MAX_US));
#endif

    // Switch feedback starts at slot 9.
    Tx_16Data[9] = read_switch(SW1_PIN);
    Tx_16Data[10] = read_switch(SW2_PIN);
    Tx_16Data[11] = read_switch(SW3_PIN);
    Tx_16Data[12] = read_switch(SW4_PIN);
    Tx_16Data[13] = read_switch(SW5_PIN);
    Tx_16Data[14] = read_switch(SW6_PIN);
    Tx_16Data[15] = read_switch(SW7_PIN);
    Tx_16Data[16] = read_switch(SW8_PIN);
}
