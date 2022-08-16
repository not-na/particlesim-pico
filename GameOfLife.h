#pragma once

#include "particlesim.h"

#define GOL_TICKDIV (TPS/5)

#define GOL_MAX_PERIOD_TRACK 8
#define GOL_RESTART_PERIOD 4

class GameOfLife {
public:
    GameOfLife();

    void load_stage(const universe_t* universe_def);

    bool tick();

    int get_period();

    uint32_t alive_color = WHITE, dead_color = BLACK;
private:
    void update();

    bool compare_compressed_universes(int a, int b) {
        for (int i = 0; i < DISPLAY_WIDTH; ++i) {
            if (prev_universes[a][i] != prev_universes[b][i]) {
                return false;
            }
        }
        return true;
    }

    uint8_t universe[DISPLAY_HEIGHT][DISPLAY_WIDTH]{};
    uint8_t new_universe[DISPLAY_HEIGHT][DISPLAY_WIDTH]{};

    uint generation{};
    int tickcounter = 0;

    bool periodic_autorestart{};
    uint32_t prev_universes[GOL_MAX_PERIOD_TRACK][DISPLAY_WIDTH]{};

    friend void gol_draw(uint32_t frame);
};
