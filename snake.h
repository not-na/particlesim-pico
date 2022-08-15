#pragma once

#include "hub75.h"
#include "particlesim.h"
#include "anim_helpers.h"

#define SNAKE_TILT_THRESHOLD (0.1)

#define SNAKE_FLAG_FRUIT  (1 << 0)
#define SNAKE_FLAG_BODY   (1 << 1)
#define SNAKE_FLAG_TAIL   (1 << 2)

#define SNAKE_COLORS_COUNT (16)

extern uint32_t snake_colors[];

typedef struct {
    uint8_t prev_x;
    uint8_t prev_y;
    uint8_t color;
    uint8_t flags;
} snake_node_t;

class Snake {
public:
    Snake();

    void init();
    bool tick(float ax, float ay, float az);

    void set_wall_collision(bool collide);
    void set_tickdiv(int div);

    void draw();
private:
    void update(float ax, float ay, float az);

    void spawn_fruit();

    snake_node_t* advance_snake(snake_node_t* newhead);

    uint8_t head_x, head_y;
    int prev_dx, prev_dy;

    int tickdiv = TPS/2;
    int tickcounter = 0;
    int score = 0;
    int length = 1;

    bool wall_collision = true;
    bool game_over = false;

    snake_node_t grid[DISPLAY_SIZE][DISPLAY_SIZE] = {0};
};