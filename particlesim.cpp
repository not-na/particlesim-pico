#include "particlesim.h"

#include "MPU6050.h"
#include "hub75.h"
#include "simulation.h"

#include "images/img_rgbm.h"
#include "images/img_disttest.h"
#include "images/img_square8.h"
#include "images/img_zigzag.h"
#include "images/img_dual.h"
#include "images/img_linrainbow.h"
#include "images/img_maze.h"
#include "images/img_single.h"
#include "images/img_blank.h"

#include "anim_helpers.h"
#include "animations_basic.h"


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

#define STAGE_COUNT 10

const stage_t STAGE_RGBM = {
        STAGE_HEAD(RGBM),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

const stage_t STAGE_DISTTEST = {
        STAGE_HEAD(DISTTEST),
        .scale = MPU_SCALE,
        .elasticity = 100,
        .rand = true,
};

const stage_t STAGE_SQUARE8 = {
        STAGE_HEAD(SQUARE8),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

const stage_t STAGE_ZIGZAG = {
        STAGE_HEAD(ZIGZAG),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

const stage_t STAGE_DUAL = {
        STAGE_HEAD(DUAL),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

const stage_t STAGE_LINRAINBOW = {
        STAGE_HEAD(LINRAINBOW),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

const stage_t STAGE_MAZE = {
        STAGE_HEAD(MAZE),
        .scale = MPU_SCALE,
        .elasticity = 70,
        .rand = false,
};

const stage_t STAGE_SINGLE = {
        STAGE_HEAD(SINGLE),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = false,
};

const stage_t STAGE_SINGLEBOUNCY = {
        STAGE_HEAD(SINGLE),
        .scale = MPU_SCALE,
        .elasticity = 255,
        .rand = false,
};

const stage_t STAGE_BLANK = {
        STAGE_HEAD(BLANK),
        .scale = MPU_SCALE,
        .elasticity = SIM_ELASTICITY,
        .rand = true,
};

// Add new stage definitions here

// List of stages in the order they are presented to the user
const stage_t stages[STAGE_COUNT] = {
        STAGE_RGBM,
        STAGE_DISTTEST,
        STAGE_ZIGZAG,
        STAGE_DUAL,
        STAGE_LINRAINBOW,
        STAGE_MAZE,
        STAGE_SINGLE,
        STAGE_SQUARE8,
        STAGE_SINGLEBOUNCY,
        STAGE_BLANK,
        // Add new stages to the list here
};

// Number of available animations
#define ANIMATION_COUNT 4

#define MODE_COUNT (STAGE_COUNT+ANIMATION_COUNT)

MPU6050 mpu;

bool btn_select_pressed = false;
bool btn_reset_pressed = false;

int cur_stage = 0;

Simulation sim(DISPLAY_SIZE, DISPLAY_SIZE,
               MPU_SCALE, SIM_MAX_PARTICLECOUNT, SIM_ELASTICITY, true
               );

uint32_t anim_framebuf[32*32];


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
        // Add new animations here
        default:
            panic("Invalid animation ID %d during render\n", id);
    }
}

[[noreturn]] int main()
{
    stdio_init_all();

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
    sim.clearAll();
    sim.loadBackground(stages[cur_stage].bg);
    sim.loadParticles(stages[cur_stage].particles, stages[cur_stage].particlecount);

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

    while (true) {
        // Check simulation reset button
        if (gpio_get(BTN_RESET_PIN) != btn_reset_pressed) {
            if (gpio_get(BTN_RESET_PIN) && time_reached(delayed_by_ms(btn_reset_last, BTN_DEBOUNCE_MS))) {
                // Button up
                btn_reset_last = get_absolute_time();

                printf("Reset!\n");

                if (cur_stage < STAGE_COUNT) {
                    sim.clearAll();
                    sim.loadBackground(stages[cur_stage].bg);
                    sim.loadParticles(stages[cur_stage].particles, stages[cur_stage].particlecount);
                } else {
                    start_anim(cur_stage-STAGE_COUNT);
                }
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

                if (cur_stage < STAGE_COUNT) {
                    sim.clearAll();
                    sim.loadBackground(stages[cur_stage].bg);
                    sim.loadParticles(stages[cur_stage].particles, stages[cur_stage].particlecount);
                } else {
                    start_anim(cur_stage-STAGE_COUNT);
                }

                printf("Selected background: %d\n", cur_stage);
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
                    printf("Accel: X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.ax, mpu.ay, mpu.az);
                    printf("Norm:  X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.axn, mpu.ayn, mpu.azn);
                    printf("Gyro:  X = % 3.6f, Y = % 3.6f, Z = % 3.6f\n", mpu.gx, mpu.gy, mpu.gz);

                    printf("Temp:  % 2.8f\n", mpu.temp);
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

                // Render animation
                draw_anim(cur_stage-STAGE_COUNT, frame);

                // Signal other core that we are done
                multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

                frame++;
                last_loop_rendered = true;
            }

            absolute_time_t et = get_absolute_time();

            if (frame % (TPS/1) == 0) {
                printf("Frametime=%lldus\n", absolute_time_diff_us(frame_time, et));
            }
        } else {
            last_loop_rendered = false;
        }
    }
}
