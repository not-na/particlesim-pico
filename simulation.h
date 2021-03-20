#pragma once

#include <stdio.h>
#include "math.h"
#include "pico/stdlib.h"

#define SIM_MAX_PARTICLECOUNT 512

// Bounce formula copied from Adafruit_PixelDust
#define BOUNCE(n) n = ((-n) * elasticity / 256) ///< 1-axis elastic bounce


typedef struct particle {
    int32_t x, y;  // Position in particle space
    int16_t vx, vy;  // Velocity in particle space
    uint32_t color;  // RGB Color of the particle
} particle_t;

class Simulation {
public:
    Simulation(uint32_t w, uint32_t h, uint8_t scale, uint32_t count=SIM_MAX_PARTICLECOUNT, uint8_t e=128, bool sort=false);

    void loadBackground(const uint32_t* bg);
    void loadParticles(const uint32_t* p, uint32_t count);

    inline void setPixel(uint32_t x, uint32_t y);
    inline void clearPixel(uint32_t x, uint32_t y);
    inline bool getPixel(uint32_t x, uint32_t y) const;

    void clearAll();

    void iterate(int32_t ax, int32_t ay, int32_t az);

    uint32_t particlecount;
    particle_t particles[SIM_MAX_PARTICLECOUNT];

private:
    bool sort;
    uint32_t width, height, w32;
    uint32_t xMax, yMax;
    uint8_t scale, elasticity;
    uint32_t bitmap[32];
};