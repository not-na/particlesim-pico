#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#include "MPU6050.h"
#include "hub75.h"
#include "simulation.h"

#include "images/img_rgbm.h"
#include "images/img_disttest.h"
#include "images/img_square8.h"
#include "images/img_zigzag.h"

#define TPS 60

#define MPU_SCALE 80
#define MPU_PRESCALE (64.0f)
#define SIM_ELASTICITY 180

#define BTN_SELECT_PIN 2
#define BTN_RESET_PIN 3

MPU6050 mpu;

bool btn_select_pressed = false;
bool btn_reset_pressed = false;

/*
 * Background image format
 *
 * Backgrounds are represented as a flat array of DISPLAY_SIZE**2 uint32_t
 *
 * Each uint32_t represents a pixel, with components 0xAARRGGBB.
 *
 * R, G and B are used for rendering. A is currently don't care, except for collision detection.
 *
 * A pixel is considered occupied if it is non-zero.
 * */

#define BG_IMAGE_COUNT 4

const uint32_t* bg_images[BG_IMAGE_COUNT] = {
        IMG_RGBM,
        IMG_DISTTEST,
        IMG_SQUARE8,
        IMG_ZIGZAG,
};


const uint32_t* bg_image_particles[BG_IMAGE_COUNT] = {
        IMG_RGBM_PARTICLES,
        IMG_DISTTEST_PARTICLES,
        IMG_SQUARE8_PARTICLES,
        IMG_ZIGZAG_PARTICLES,
};

const uint32_t bg_image_particlecount[BG_IMAGE_COUNT] = {
        IMG_RGBM_PARTICLE_COUNT,
        IMG_DISTTEST_PARTICLE_COUNT,
        IMG_SQUARE8_PARTICLE_COUNT,
        IMG_ZIGZAG_PARTICLE_COUNT,
};

int cur_bgimg = 0;

Simulation sim(DISPLAY_SIZE, DISPLAY_SIZE,
               MPU_SCALE, SIM_MAX_PARTICLECOUNT, SIM_ELASTICITY, false
               );

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
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

    // Uncomment to sleep a bit to wait for USB connection
    //sleep_ms(3000);

    display_background = bg_images[cur_bgimg];

    uint32_t frame = 0;
    absolute_time_t frame_time = get_absolute_time();

    bool last_loop_rendered = false;
    absolute_time_t last_warn = get_absolute_time();
    uint32_t warn_count = 0;

    while (true) {
        // Check simulation reset button
        if (gpio_get(BTN_RESET_PIN) != btn_reset_pressed) {
            if (gpio_get(BTN_RESET_PIN)) {
                // Button up
                printf("Reset!\n");

                sim.clearAll();
                sim.loadBackground(bg_images[cur_bgimg]);
                sim.loadParticles(bg_image_particles[cur_bgimg], bg_image_particlecount[cur_bgimg]);
            } else {
                // Button down
            }

            btn_reset_pressed = gpio_get(BTN_RESET_PIN);
        }

        // Check mode select button
        if (gpio_get(BTN_SELECT_PIN) != btn_select_pressed) {
            if (gpio_get(BTN_SELECT_PIN)) {
                // Button up
                printf("Select!\n");
                cur_bgimg = (cur_bgimg+1)%BG_IMAGE_COUNT;

                sim.clearAll();
                sim.loadBackground(bg_images[cur_bgimg]);
                sim.loadParticles(bg_image_particles[cur_bgimg], bg_image_particlecount[cur_bgimg]);

                printf("Selected background: %d\n", cur_bgimg);
            } else {
                // Button down
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
                }
            }

            // Update MPU6050, results are available as attributes
            mpu.update();

            if (frame % (TPS/2) == 0) {
                printf("Accel: X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.ax, mpu.ay, mpu.az);
                printf("Norm:  X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.axn, mpu.ayn, mpu.azn);
                printf("Gyro:  X = % 3.6f, Y = % 3.6f, Z = % 3.6f\n", mpu.gx, mpu.gy, mpu.gz);

                printf("Temp:  % 2.8f\n", mpu.temp);
            }

            absolute_time_t t2 = get_absolute_time();

            // Step the simulation
            sim.iterate((int) (mpu.axn * MPU_PRESCALE), (int) (mpu.ayn * MPU_PRESCALE), (int) (mpu.azn * MPU_PRESCALE));

            absolute_time_t t3 = get_absolute_time();

            // Wait until previous timestep is done rendering
            // Usually only a few microseconds
            multicore_fifo_pop_blocking();

            absolute_time_t t4 = get_absolute_time();

            // Copy over particle data
            // memcpy should use pico-optimized variant and be relatively fast
            memcpy(&display_particles, &sim.particles, sizeof(particle_t)*sim.particlecount);
            display_particlecount = sim.particlecount;

            // Update background reference and trigger redraw by signalling other core
            display_background = bg_images[cur_bgimg];
            multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

            frame++;
            last_loop_rendered = true;

            absolute_time_t t5 = get_absolute_time();

            // Performance measurements
            if (frame%(TPS/10)==0) {
                printf("MPU=%lldus SIM=%lldus FIFO=%lldus COPY=%lldus\n",
                       absolute_time_diff_us(frame_time, t2),
                       absolute_time_diff_us(t2, t3),
                       absolute_time_diff_us(t3, t4),
                       absolute_time_diff_us(t4, t5)
                );
                printf("Frametime=%lldus\n", absolute_time_diff_us(frame_time, t5));
            }
        } else {
            last_loop_rendered = false;
        }
    }
}
