/*====================================================================
<serial_task.hpp>
・シリアル通信タスクAPI

Copyright (c) 2026.
====================================================================*/

#pragma once

#include <stdint.h>

void serial_task_init();
void serial_task_update();
void serial_task_send_now();

uint32_t serial_last_rx_ms();
