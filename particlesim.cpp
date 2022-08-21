#include "particlesim.h"

#include "MPU6050.h"
#include "hub75.h"
#include "simulation.h"

#include "images/img_all.h"
#include "gol/gol_all.h"

#include "anim_helpers.h"
#include "animations_basic.h"

#include "snake.h"
#include "GameOfLife.h"



/*
 * Background image format
 *
 * Backgrounds are represented as a flat array of DISPLAY_SIZE**2 uint32_t
 *
 * Each uint32_t represents a pixel, with components 0xAABBGGRR.
 *
 * R, G and B are used for rendering. A is currently don't care, except for collision detection.
 *
 * A pixel is considered occupied if it is non-zero.
 * */

#include "stages.cpp"

// Number of available stages/universes/animations
#define STAGE_COUNT count_of(stages)
#define UNIVERSE_COUNT count_of(universes)
#define ANIMATION_COUNT 10

#define MODE_COUNT (STAGE_COUNT+UNIVERSE_COUNT+ANIMATION_COUNT)

const char* animation_names[ANIMATION_COUNT] = {
        "Color Cycle",
        "Color Cycle - Slow",
        "Color Cycle - Ultra Slow",
        "Perlin Noise",
        "Snake, classic, 3tps",
        "Snake, classic, 4tps",
        "Snake, classic, 5tps",
        "Snake, no wall collisions, 3tps",
        "Snake, no wall collisions, 4tps",
        "Snake, no wall collisions, 5tps",
};

MPU6050 mpu;

bool btn_select_pressed = false;
bool btn_reset_pressed = false;

int cur_stage = 0;

Simulation sim(DISPLAY_SIZE, DISPLAY_SIZE,
               MPU_SCALE, SIM_MAX_PARTICLECOUNT, SIM_ELASTICITY, true
               );

Snake snake;
GameOfLife gol;

uint32_t anim_framebuf[32*32];

void start_stage();

void print_statusinfo() {
    printf("Accel: X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.ax, mpu.ay, mpu.az);
    printf("Norm:  X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.axn, mpu.ayn, mpu.azn);
    printf("Gyro:  X = % 3.6f, Y = % 3.6f, Z = % 3.6f\n", mpu.gx, mpu.gy, mpu.gz);

    printf("Temp:  % 2.8f\n", mpu.temp);
    
    char id[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1];
    pico_get_unique_board_id_string(id, 2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1);
    printf("ID: %s\n", id);
}

void gol_draw(uint32_t frame) {
    // No need for MPU updates, since GoL doesn't have user input

    absolute_time_t ts = get_absolute_time();
    bool ticked = gol.tick();
    int period = gol.get_period();

    absolute_time_t te = get_absolute_time();

    int count = 0;
    for (int x = 0; x < DISPLAY_SIZE; ++x) {
        for (int y = 0; y < DISPLAY_SIZE; ++y) {

            if (gol.universe[y][x]) {
                count++;
                gl_pixel(x, y, gol.alive_color);
            } else {
                gl_pixel(x, y, gol.dead_color);
            }
        }
    }

    if (count == 0) {
        // No more remaining cells, reset simulation
        // This avoids situations where the screen goes black even though it is running
        printf("GoL: population reached zero after %u generations, restarting\n", gol.generation);
        start_stage();
    }
    if (gol.periodic_autorestart && period != -1 && period <= GOL_RESTART_PERIOD) {
        // Restart if we are in a short periodic loop
        // Note that this does not recognize moving patterns like gliders, only stuff like blinkers
        printf("GoL: period=%d after %u generations, restarting\n", period, gol.generation);
        start_stage();
    }

    absolute_time_t et = get_absolute_time();
    if (ticked) {
        printf("GoL: gen=%u period=%d tick=%lldus draw=%lldus\n", gol.generation, period, absolute_time_diff_us(ts, te), absolute_time_diff_us(te, et));
    }
}

void snake_draw(uint32_t frame) {
    // Snake, both modes
    mpu.update();

    if (frame % (TPS / 1) == 0) {
        print_statusinfo();
    }
    absolute_time_t ts = get_absolute_time();
    bool ticked = snake.tick(mpu.ayn, mpu.axn, mpu.azn);  // X and Y swapped

    absolute_time_t te = get_absolute_time();

    snake.draw();

    absolute_time_t et = get_absolute_time();
    if (ticked) {
        printf("Snake: tick=%lldus draw=%lldus\n", absolute_time_diff_us(ts, te), absolute_time_diff_us(te, et));
    }
}

void start_anim(int id) {
    // Called on every animation start or reset
    switch (id) {
        case 0:
            // Colorcycle Animation from animations_basic.h
            anim_colorcycle_start();
            break;
        case 1:
            // Slow Colorcycle Animation from animations_basic.h
            anim_colorcycleslow_start();
            break;
        case 2:
            // Ultraslow Colorcycle Animation from animations_basic.h
            anim_colorcycleuslow_start();
            break;
        case 3:
            // Perlin Noise Animation from animations_basic.h
            anim_perlinnoise_start();
            break;
        case 4:
            // Snake, classic mode, 3 updates per second
            snake.set_wall_collision(true);
            snake.set_tickdiv(SNAKE_TICKDIV_SLOW);
            snake.init();
            break;
        case 5:
            // Snake, classic mode, 4 updates per second
            snake.set_wall_collision(true);
            snake.set_tickdiv(SNAKE_TICKDIV_MEDIUM);
            snake.init();
            break;
        case 6:
            // Snake, classic mode, 5 updates per second
            snake.set_wall_collision(true);
            snake.set_tickdiv(SNAKE_TICKDIV_FAST);
            snake.init();
            break;
        case 7:
            // Snake, no wall collisions, 3 updates per second
            snake.set_wall_collision(false);
            snake.set_tickdiv(SNAKE_TICKDIV_SLOW);
            snake.init();
            break;
        case 8:
            // Snake, no wall collisions, 4 updates per second
            snake.set_wall_collision(false);
            snake.set_tickdiv(SNAKE_TICKDIV_MEDIUM);
            snake.init();
            break;
        case 9:
            // Snake, no wall collisions, 5 updates per second
            snake.set_wall_collision(false);
            snake.set_tickdiv(SNAKE_TICKDIV_FAST);
            snake.init();
            break;
        // Add new animations here
        default:
            panic("Invalid animation ID %d during initialize\n", id);
    }
}

void draw_anim(int id, uint32_t frame) {
    // Called every animation frame, usually TPS times a second
    switch (id) {
        case 0:
            // Colorcycle Animation from animations_basic.h
            anim_colorcycle_draw();
            break;
        case 1:
            // Slow Colorcycle Animation from animations_basic.h
            anim_colorcycleslow_draw();
            break;
        case 2:
            // Ultraslow Colorcycle Animation from animations_basic.h
            anim_colorcycleuslow_draw();
            break;
        case 3:
            // Perlin Noise from animations_basic.h
            anim_perlinnoise_draw();
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            snake_draw(frame);
            break;
        // Add new animations here
        default:
            panic("Invalid animation ID %d during render\n", id);
    }
}

void start_stage() {
    if (cur_stage < STAGE_COUNT) {
        sim.clearAll();
        sim.loadBackground(stages[cur_stage].bg);
        sim.loadParticles(stages[cur_stage].particles, stages[cur_stage].particlecount);
    } else if (cur_stage-STAGE_COUNT < UNIVERSE_COUNT) {
        gol.load_stage(&universes[cur_stage-STAGE_COUNT]);
    } else {
        start_anim(cur_stage-STAGE_COUNT-UNIVERSE_COUNT);
    }
}

const char* get_stage_name(int id) {
    if (id < STAGE_COUNT + UNIVERSE_COUNT) {
        return stage_names[id];
    } else {
        return animation_names[id-STAGE_COUNT-UNIVERSE_COUNT];
    }
}

int main()
{
    stdio_init_all();
    printf("particlesim-pico v%s\nBooting up...", VERSION);

    // Binary info
    bi_decl(bi_program_description("Particle simulator using MPU6050 and a 32x32 LED HUB75 Matrix"));
    bi_decl(bi_program_name("Particlesim-pico"));
    bi_decl(bi_1pin_with_name(BTN_RESET_PIN, "Simulation Reset Button, connected to ground"));
    bi_decl(bi_1pin_with_name(BTN_SELECT_PIN, "Mode Select Button, connected to ground"));

    // Initialize Button GPIOs
    gpio_init(BTN_RESET_PIN);
    gpio_init(BTN_SELECT_PIN);

    // Mark buttons as inputs
    gpio_set_dir(BTN_RESET_PIN, GPIO_IN);
    gpio_set_dir(BTN_SELECT_PIN, GPIO_IN);

    // Enable pull-ups for button inputs since buttons are connected to ground
    // If the buttons are connected to VCC instead, change gpio_pull_up to gpio_pull_down
    // The logic will be inverted, causing changes to happen on click instead of release
    gpio_pull_up(BTN_RESET_PIN);
    gpio_pull_up(BTN_SELECT_PIN);

    // Set buttons to initial state to prevent ghost clicks during boot
    btn_reset_pressed = gpio_get(BTN_RESET_PIN);
    btn_select_pressed = gpio_get(BTN_SELECT_PIN);

    // Initialize MPU6050
    mpu.reset();

    // Initialize HUB75
    hub75_init();

    // Launch matrix driver main loop
    multicore_launch_core1(hub75_main);
    sleep_ms(100);  // Sleep a bit to allow for proper initialization of HUB75 driver main loop

    // Initialize Simulation
    start_stage();

    display_background = stages[cur_stage].bg;

    // Uncomment to sleep a bit to wait for USB connection
    //sleep_ms(3000);

    uint32_t frame = 0;
    absolute_time_t frame_time = get_absolute_time();

    bool last_loop_rendered = false;
    absolute_time_t last_warn = get_absolute_time();
    uint32_t warn_count = 0;

    absolute_time_t btn_reset_last = get_absolute_time();
    absolute_time_t btn_select_last = get_absolute_time();

    if (!btn_select_pressed && !btn_reset_pressed) {
        // Enter diagnosis mode

        // Set up animation framebuffer for rendering
        display_background = anim_framebuf;
        display_particlecount = 0;

        // Draw code
        gl_fillscreen(BLACK);  // Clear screen

        // Version code, displayed as a line of white pixels
        gl_fillrect(0, 0, VERSION_NUM, 1, WHITE);

        // TODO: add real-time text diagnostics, e.g. MPU readouts

        // Trigger redraw, also consume FIFO token
        uint32_t fifo_out = 0;
        multicore_fifo_pop_timeout_us(FIFO_TIMEOUT, &fifo_out);
        multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

        // Sleep forever, since diagnosis mode is non-interactive
        while (true) {
            __wfi();
        }
    }

    while (true) {
        // Check simulation reset button
        if (gpio_get(BTN_RESET_PIN) != btn_reset_pressed) {
            if (gpio_get(BTN_RESET_PIN) && time_reached(delayed_by_ms(btn_reset_last, BTN_DEBOUNCE_MS))) {
                // Button up
                btn_reset_last = get_absolute_time();

                printf("Reset! id=%d name '%s'\n", cur_stage, get_stage_name(cur_stage));
                start_stage();
            } else if (!gpio_get(BTN_SELECT_PIN)){
                // Pressed RESET while SELECT was pressed, reboot into bootsel mode
                reset_usb_boot(0, 0);
            }

            btn_reset_pressed = gpio_get(BTN_RESET_PIN);
        }

        // Check mode select button
        if (gpio_get(BTN_SELECT_PIN) != btn_select_pressed) {
            if (gpio_get(BTN_SELECT_PIN) && time_reached(delayed_by_ms(btn_select_last, BTN_DEBOUNCE_MS))) {
                // Button up
                btn_select_last = get_absolute_time();

                printf("Select!\n");
                cur_stage = (cur_stage + 1) % MODE_COUNT;

                start_stage();

                printf("Selected stage with id %d and name '%s'\n", cur_stage, get_stage_name(cur_stage));
            }

            btn_select_pressed = gpio_get(BTN_SELECT_PIN);
        }

        // Limit frame rate if we are too fast
        if (time_reached(delayed_by_ms(frame_time, 1000/TPS))) {
            frame_time = get_absolute_time();

            // Simple over-utilization detection with rate limited warnings
            if (last_loop_rendered) {
                // Did not sleep between loops, likely near 100% utilization
                warn_count++;
                if (time_reached(delayed_by_ms(last_warn, 1000))) {
                    printf("WARNING: CPU Usage near 100%%, count=%lu\n", warn_count);
                    last_warn = get_absolute_time();
                }
            }

            if (cur_stage < STAGE_COUNT) {
                // Process / Render simulation

                // Update MPU6050, results are available as attributes
                // TODO: improve performance of MPU update, since it is currently quite slow
                mpu.update();

                if (frame % (TPS / 1) == 0) {
                    print_statusinfo();
                }

                absolute_time_t t2 = get_absolute_time();

                // Copy parameters from config struct
                sim.scale = stages[cur_stage].scale;
                sim.elasticity = stages[cur_stage].elasticity;
                sim.rand = stages[cur_stage].rand;

                // Step the simulation
                sim.iterate((int) (mpu.ayn * MPU_PRESCALE), (int) (mpu.axn * MPU_PRESCALE),
                            (int) (mpu.azn * MPU_PRESCALE));

                absolute_time_t t3 = get_absolute_time();

                // Wait until previous timestep is done rendering
                // Usually only a few microseconds
                uint32_t fifo_out = 0;
                if (!multicore_fifo_pop_timeout_us(FIFO_TIMEOUT, &fifo_out)) {
                    printf("ERROR: Timed out while waiting for HUB75 driver to finish redrawing!\n");
                    last_loop_rendered = false;
                    continue;  // Skip frame, because our outbound FIFO would fill up otherwise
                }

                absolute_time_t t4 = get_absolute_time();

                // Copy over particle data
                // memcpy should use pico-optimized variant and be relatively fast
                memcpy(&display_particles, &sim.particles, sizeof(particle_t) * sim.particlecount);
                display_particlecount = sim.particlecount;

                // Update background reference and trigger redraw by signalling other core
                display_background = stages[cur_stage].bg;
                multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

                frame++;
                last_loop_rendered = true;

                absolute_time_t t5 = get_absolute_time();

                // Performance measurements
                if (frame % (TPS / 1) == 0) {
                    printf("MPU=%lldus SIM=%lldus FIFO=%lldus COPY=%lldus\n",
                           absolute_time_diff_us(frame_time, t2),
                           absolute_time_diff_us(t2, t3),
                           absolute_time_diff_us(t3, t4),
                           absolute_time_diff_us(t4, t5)
                    );
                }

            } else {
                // Wait until previous frame is done rendering
                // Usually only a few microseconds, but may be more since
                // animations render quite fast and we don't have to wait for the MPU
                uint32_t fifo_out = 0;
                if (!multicore_fifo_pop_timeout_us(FIFO_TIMEOUT, &fifo_out)) {
                    printf("ERROR: Timed out while waiting for HUB75 driver to finish redrawing!\n");
                    last_loop_rendered = false;
                    continue;  // Skip frame, because our outbound FIFO would fill up otherwise
                }

                // Configure HUB75 driver to draw from framebuffer
                display_background = anim_framebuf;
                display_particlecount = 0;  // Animations could override this, but must do so every frame

                // Render animation / GoL
                if (cur_stage-STAGE_COUNT < UNIVERSE_COUNT) {
                    gol_draw(frame);
                } else {
                    draw_anim(cur_stage - STAGE_COUNT - UNIVERSE_COUNT, frame);
                }

                // Signal other core that we are done
                multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

                frame++;
                last_loop_rendered = true;
            }

            absolute_time_t et = get_absolute_time();

            if (frame % (TPS/1) == 0) {
                time_t ft = absolute_time_diff_us(frame_time, et);
                printf("Frametime=%lldus (max=%dus) cpu=%.3f%%\n", ft, 1000000/TPS,  ((int32_t)ft)/(1000000.0/TPS)*100);
            }
        } else {
            last_loop_rendered = false;
        }
    }
}
