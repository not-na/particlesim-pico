#pragma once

#include "particlesim.h"

// TODO: add gamma correction to COLOR macro
#define COLOR(R, G, B) ((R)<<0 | (G)<<8 | (B)<<16)
// TODO: add gamma correction to COLOR_HSV macro
#define COLOR_HSV(H, S, V) (gl_color_hsv_nogamma((H), (S), (V)))

#define BLACK COLOR(0, 0, 0)
#define WHITE COLOR(255, 255, 255)
#define GREY COLOR(128, 128, 128)

void gl_fillscreen(uint32_t color);

void gl_fillrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void gl_pixel(uint32_t x, uint32_t y, uint32_t color);

// TODO: add functions for lines, circles etc.

uint32_t gl_color_hsv_nogamma(uint16_t hue, uint8_t sat=255, uint8_t val=255);