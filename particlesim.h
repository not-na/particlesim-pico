#pragma once

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#include "anim_helpers.h"

#define TPS 120
#define FIFO_TIMEOUT (1000/TPS*1000*2)

#define MPU_SCALE 32
#define MPU_PRESCALE (48.0f)
#define SIM_ELASTICITY 170

#define BTN_SELECT_PIN 2
#define BTN_RESET_PIN 3

// Debounce has to be quite aggressive, since relatively cheap and physically
// large buttons are used
// It would be better to use hardware debouncing with an RC-Filter
#define BTN_DEBOUNCE_MS 150

#define DISPLAY_WIDTH 32
#define DISPLAY_HEIGHT 32

extern uint32_t anim_framebuf[DISPLAY_HEIGHT*DISPLAY_WIDTH];
