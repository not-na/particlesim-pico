#include <cstdlib>
#include "GameOfLife.h"

// Makes code more readable
#define W DISPLAY_WIDTH
#define H DISPLAY_HEIGHT

GameOfLife::GameOfLife() {
    // Zero out universes
    memset(&universe, 0, sizeof(universe));
    memset(&new_universe, 0, sizeof(new_universe));

    // Pattern loading is done on game start
}


bool GameOfLife::tick() {
    tickcounter--;

    if (tickcounter <= 0) {
        tickcounter = GOL_TICKDIV;
        update();

        return true;
    }

    return false;
}

void GameOfLife::update() {
    // Iterate through every cell
    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            // For each cell, count the number of neighbours that are alive
            int alive = 0;
            for (int x1 = x-1; x1 <= x+1; ++x1) {
                for (int y1 = y-1; y1 <= y+1; ++y1) {
                    if (universe[(y1 + H)%H][(x1 + W)%W]) {
                        alive++;
                    }
                }
            }
            // Note that we may have accidentally counted ourselves, so compensate
            if (universe[y][x]) {
                alive--;
            }

            // Apply rule
            //                   3 always lives | 2 only lives if previously alive
            new_universe[y][x] = alive == 3 || (alive == 2 && universe[y][x]);
            // Overpopulation/Starvation cases are implicitly handled
        }
    }

    // Done iterating, copy over new_universe to universe
    memcpy(&universe, &new_universe, sizeof(universe));
    generation++;

    // Create compressed version for period checking
    // Simply done by using only one bit per cell instead of a full byte
    // This reduces storage needs and speeds up comparison
    // This can be considered to be a very specific perfect hash, since no collisions
    // are possible.
    uint idx = generation % GOL_MAX_PERIOD_TRACK;
    for (int x = 0; x < W; ++x) {
        uint32_t line = 0;
        for (int y = 0; y < H; ++y) {
            line |= universe[y][x]&1;
            line <<= 1;
        }
        prev_universes[idx][x] = line;
    }
}

void GameOfLife::load_stage(const universe_t *universe_def) {
    generation = 0;
    memset(&prev_universes, 0, sizeof(prev_universes));

    periodic_autorestart = universe_def->period_restart;
    if (universe_def->cells == nullptr) {
        // Soup
        int thresh = (int)(universe_def->prob * (float)RAND_MAX);
        printf("Generating soup with prob=%.4f periodic_autorestart=%d\n", universe_def->prob, periodic_autorestart);

        for (int x = 0; x < W; ++x) {
            for (int y = 0; y < H; ++y) {
                universe[y][x] = rand() <= thresh;
            }
        }

        return;
    }

    puts("Loading stage...");
    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            universe[y][x] = universe_def->cells[y*W+x];
        }
    }
}

int GameOfLife::get_period() {
    // Find the most recent universe that matches the latest entry
    // Iterate from second-to-latest entry backwards, looping around where necessary
    int period = -1;

    for (int i = 1; i < GOL_MAX_PERIOD_TRACK - 1; ++i) {
        if (generation-i <= 0) {
            // Don't overrun into prehistoric times
            break;
        }
        int idx = (generation-i) % GOL_MAX_PERIOD_TRACK;
        if (compare_compressed_universes(generation%GOL_MAX_PERIOD_TRACK, idx)) {
            // Found a match, report i as period
            period = i;
            // Exit loop since we only want the shortest period
            break;
        }
    }

    if (period >= generation) {
        // Just in case, to prevent weird edge cases at the start
        period = -1;
    }

    return period;
}
