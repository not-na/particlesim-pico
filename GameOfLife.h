#pragma once

#include "particlesim.h"

#define GOL_TICKDIV (TPS/5)

class GameOfLife {
public:
    GameOfLife();

    void load_stage(const universe_t* universe_def);

    bool tick();

    uint32_t alive_color = WHITE, dead_color = BLACK;
private:
    void update();

    uint8_t universe[DISPLAY_HEIGHT][DISPLAY_WIDTH]{};
    uint8_t new_universe[DISPLAY_HEIGHT][DISPLAY_WIDTH]{};

    uint generation{};
    int tickcounter = 0;

    friend void gol_draw(uint32_t frame);
};
