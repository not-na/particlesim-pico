#include "animations_basic.h"

uint16_t anim_colorcycle_cur = 0;

// Colorcycle

void anim_colorcycle_start() {
    // Reset color to start
    anim_colorcycle_cur = 0;
}

void anim_colorcycle_draw() {
    gl_fillscreen(COLOR_HSV(anim_colorcycle_cur, 255, 255));

    // No need for modulo, overflows at exactly the right point
    anim_colorcycle_cur += ANIM_COLORCYCLE_STEP;
}

// Colorcycle Slow

void anim_colorcycleslow_start() {
    // Reset color to start
    anim_colorcycle_cur = 0;
}

void anim_colorcycleslow_draw() {
    gl_fillscreen(COLOR_HSV(anim_colorcycle_cur, 255, 255));

    // No need for modulo, overflows at exactly the right point
    anim_colorcycle_cur += ANIM_COLORCYCLE_SLOW_STEP;
}

// Colorcycle UltraSlow

void anim_colorcycleuslow_start() {
    // Reset color to start
    anim_colorcycle_cur = 0;
}

void anim_colorcycleuslow_draw() {
    gl_fillscreen(COLOR_HSV(anim_colorcycle_cur, 255, 255));

    // No need for modulo, overflows at exactly the right point
    anim_colorcycle_cur += ANIM_COLORCYCLE_USLOW_STEP;
}

// Perlin Noise

void anim_perlinnoise_start() {

}

void anim_perlinnoise_draw() {
    // TODO: implement this
    gl_fillscreen(COLOR(255, 0, 255));
}