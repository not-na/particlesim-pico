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
}

void GameOfLife::load_stage(const universe_t *universe_def) {
    generation = 0;

    if (universe_def->cells == nullptr) {
        // Soup
        int thresh = (int)(universe_def->prob * (float)RAND_MAX);
        printf("Generating soup with prob=%.4f\n", universe_def->prob);

        for (int x = 0; x < W; ++x) {
            for (int y = 0; y < H; ++y) {
                universe[y][x] = rand() >= thresh;
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
