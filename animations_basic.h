#pragma once

#include "particlesim.h"
#include <math.h>

// Colorcycle

#define ANIM_COLORCYCLE_PERIOD (6)
#define ANIM_COLORCYCLE_STEP (65536/(ANIM_COLORCYCLE_PERIOD*TPS))

void anim_colorcycle_start();
void anim_colorcycle_draw();

// Colorcycle Slow

#define ANIM_COLORCYCLE_SLOW_PERIOD (24)
#define ANIM_COLORCYCLE_SLOW_STEP (65536/(ANIM_COLORCYCLE_SLOW_PERIOD*TPS))

void anim_colorcycleslow_start();
void anim_colorcycleslow_draw();

// Colorcycle UltraSlow

#define ANIM_COLORCYCLE_USLOW_PERIOD (60)
#define ANIM_COLORCYCLE_USLOW_STEP (65536/(ANIM_COLORCYCLE_USLOW_PERIOD*TPS))

void anim_colorcycleuslow_start();
void anim_colorcycleuslow_draw();


// Perlin Noise

void anim_perlinnoise_start();
void anim_perlinnoise_draw();