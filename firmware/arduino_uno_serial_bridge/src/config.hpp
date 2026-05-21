/*====================================================================
<config.hpp>
Configure DEVICE_ID and mode selection here.
Do not edit src/main.cpp unless you know what you're doing.
====================================================================*/

#pragma once

// ================= Basic =================

// Must be unique among all connected MCUs.
#define DEVICE_ID 0x01

// Mode selection (define exactly one).
// This Uno firmware currently supports only IO mode.
#define MODE_IO
