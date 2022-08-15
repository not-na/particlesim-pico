#include "snake.h"


uint32_t snake_colors[] = {
        COLOR_HSV(0, 255, 255),
        COLOR_HSV(4096, 255, 255),
        COLOR_HSV(8192, 255, 255),
        COLOR_HSV(12288, 255, 255),
        COLOR_HSV(16384, 255, 255),
        COLOR_HSV(20480, 255, 255),
        COLOR_HSV(24576, 255, 255),
        COLOR_HSV(28672, 255, 255),
        COLOR_HSV(32768, 255, 255),
        COLOR_HSV(36864, 255, 255),
        COLOR_HSV(40960, 255, 255),
        COLOR_HSV(45056, 255, 255),
        COLOR_HSV(49152, 255, 255),
        COLOR_HSV(53248, 255, 255),
        COLOR_HSV(57344, 255, 255),
        COLOR_HSV(61440, 255, 255),
};

Snake::Snake() {
    head_x = 0;
    head_y = 0;
    prev_dx = 0;
    prev_dy = 0;

    init();
}

void Snake::init() {
    printf("Snake init! tickdiv=%u\n", this->tickdiv);

    // Reset grid
    memset(this->grid, 0, sizeof(this->grid));

    this->score = 0;
    this->length = 1;
    this->game_over = false;

    // Initialize snake
    this->head_x = DISPLAY_SIZE/2;
    this->head_y = DISPLAY_SIZE/2;
    this->prev_dx = 0;
    this->prev_dy = 0;
    this->grid[head_x][head_y].flags = SNAKE_FLAG_BODY | SNAKE_FLAG_TAIL;
    this->grid[head_x][head_y].color = rand()%SNAKE_COLORS_COUNT;  // Random starting color

    // Spawn first fruit
    this->spawn_fruit();
}

void Snake::set_wall_collision(bool collide) {
    this->wall_collision = collide;
}

bool Snake::tick(float ax, float ay, float az) {
    if (game_over) {
        return false;
    }

    this->tickcounter--;

    if (this->tickcounter <= 0) {
        this->tickcounter = this->tickdiv;
        this->update(ax, ay, az);

        return true;
    }

    return true;
}

void Snake::update(float ax, float ay, float az) {
    int dx = 0;
    int dy = 0;

    // Decide in which direction the user wants to move
    if (abs(ax) > abs(ay)) {
        // X acceleration is greater, tilt on x axis if higher than threshold
        if (ax >= SNAKE_TILT_THRESHOLD) {
            dx = 1;
        } else if (-ax >= SNAKE_TILT_THRESHOLD) {
            dx = -1;
        }
    } else {
        // Y acceleration is greater, tilt on y axis if higher than threshold
        if (ay >= SNAKE_TILT_THRESHOLD) {
            dy = 1;
        } else if (-ay >= SNAKE_TILT_THRESHOLD) {
            dy = -1;
        }
    }

    // If user does not indicate direction, either use previous or wait if unknown
    if (dx == 0 && dy == 0) {
        if (prev_dx == 0 && prev_dy == 0) {
            return;
        }

        dx = prev_dx;
        dy = prev_dy;
    }

    // Prevent direction change if user wants to reverse
    if (!(grid[head_x][head_y].flags & SNAKE_FLAG_TAIL)) {
        if (head_x+dx == grid[head_x][head_y].prev_x && head_y+dy == grid[head_x][head_y].prev_y) {
            // Continue in same direction as before
            dx = prev_dx;
            dy = prev_dy;
        }
    }

    this->prev_dx = dx;
    this->prev_dy = dy;

    int new_x = head_x+dx;
    int new_y = head_y+dy;

    // Check if we collide with a wall
    if (wall_collision) {
        // Game over on collision
        if (new_x < 0 || new_y < 0 || new_x >= DISPLAY_SIZE || new_y >= DISPLAY_SIZE) {
            game_over = true;
            return;
        }
    } else {
        // Wrap around
        new_x = new_x % DISPLAY_SIZE;
        new_y = new_y % DISPLAY_SIZE;
    }

    snake_node_t* next = &grid[new_x][new_y];
    if (next->flags & SNAKE_FLAG_BODY || next->flags & SNAKE_FLAG_TAIL) {
        // Collision with itself, game over
        game_over = true;
        return;
    } else if (next->flags & SNAKE_FLAG_FRUIT) {
        // Found fruit
        // Remove it and increase length, color is kept from fruit color
        next->flags = 0;

        uint8_t color = next->color;

        // Advance snake and attach fruit to tail
        snake_node_t* tail = advance_snake(next);
        tail->flags = SNAKE_FLAG_BODY;
        snake_node_t* newtail = &grid[tail->prev_x][tail->prev_y];
        newtail->color = color;
        newtail->flags = SNAKE_FLAG_BODY|SNAKE_FLAG_TAIL;

        length++;

        // Spawn new fruit
        spawn_fruit();
    } else {
        // Empty field, move snake forwards
        advance_snake(next);
    }

    head_x = new_x;
    head_y = new_y;
}

snake_node_t *Snake::advance_snake(snake_node_t *newhead) {
    snake_node_t* next = newhead;

    next->flags = SNAKE_FLAG_BODY;

    if (length == 1) {
        // Special case, length == 1
        next->flags |= SNAKE_FLAG_TAIL;
        next->color = grid[head_x][head_y].color;
        next->prev_x = head_x;
        next->prev_y = head_y;
        grid[head_x][head_y].flags = 0;

        return next;
    } else {
        // Standard case, length >= 2
        next->prev_x = head_x;
        next->prev_y = head_y;

        snake_node_t *cur = next;
        while (true) {
            // Travel along the snake from head to tail
            next = &grid[cur->prev_x][cur->prev_y];

            // Shift colors from back to front
            cur->color = next->color;

            // If we find the tail, remove it and stop
            if (next->flags & SNAKE_FLAG_TAIL) {
                next->flags = 0;
                cur->flags |= SNAKE_FLAG_TAIL;
                break;
            }

            cur = next;
        }

        return cur;
    }

}

void Snake::spawn_fruit() {
    printf("Spawning new fruit...\n");

    if (this->length >= DISPLAY_SIZE*DISPLAY_SIZE) {
        // No more room for fruits, don't spawn any
        // TODO: maybe add some kind of game won screen here
        return;
    }

    uint8_t pos_x=0, pos_y=0;

    // Find an empty spot by picking an index of remaining empty tiles
    int idx = rand()%(DISPLAY_SIZE*DISPLAY_SIZE-this->length);

    int i = 0;
    for (int x = 0; x < DISPLAY_SIZE; ++x) {
        for (int y = 0; y < DISPLAY_SIZE; ++y) {
            if (this->grid[x][y].flags == 0) {
                // Empty spot, check if the index is right
                if (i == idx) {
                    // Found it!
                    pos_x = x;
                    pos_y = y;
                    break;
                }
                i++;
            }
        }

        if (i == idx) {
            break;
        }
    }

    printf("Found fruit location: x=%u y=%u (idx=%u, max=%u)\n", pos_x, pos_y, idx, DISPLAY_SIZE*DISPLAY_SIZE-this->length);

    this->grid[pos_x][pos_y].flags |= SNAKE_FLAG_FRUIT;
    this->grid[pos_x][pos_y].color = rand()%SNAKE_COLORS_COUNT;
}

void Snake::draw() {
    for (int x = 0; x < DISPLAY_SIZE; ++x) {
        for (int y = 0; y < DISPLAY_SIZE; ++y) {
            snake_node_t* node = &grid[x][y];

            // Draw node if flags are non-empty
            // Since fruits are currently visually identical to body parts,
            // we don't need to differentiate between them
            if (node->flags != 0) {
                gl_pixel(x, y, snake_colors[node->color]);
            } else {
                gl_pixel(x, y, COLOR(0, 0, 0));
            }
        }
    }
}

void Snake::set_tickdiv(int div) {
    this->tickdiv = div;
}
