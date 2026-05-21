#include "config.hpp"
#include "defs.hpp"
#include "pid_task.hpp"
#include "serial_task.hpp"

#include <Arduino.h>

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);

    pinMode(LED, OUTPUT);

    robomas_init();

    xTaskCreatePinnedToCore(
        serialTask,
        "serialTask",
        2048,
        nullptr,
        10,
        nullptr,
        0);

    xTaskCreatePinnedToCore(
        M2006_Task,
        "M2006_Task",
        4096,
        nullptr,
        20,
        nullptr,
        1);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
