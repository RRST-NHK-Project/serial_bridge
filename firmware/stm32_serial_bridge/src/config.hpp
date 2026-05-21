/*====================================================================
<config.hpp>
Configure DEVICE_ID and mode selection here.
Do not edit src/main.cpp unless you know what you're doing.
====================================================================*/

#pragma once

// Must be unique among all connected MCUs.
#define DEVICE_ID 0x02

// Mode selection (define exactly one)
#define MODE_FULL_IO

#if (defined(MODE_FULL_IO)) != 1
#error "Invalid mode configuration. Please define exactly one mode in config.hpp."
#endif
