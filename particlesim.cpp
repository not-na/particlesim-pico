#include <stdio.h>
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

#include "images/img_rgbm.h"
#include "images/img_disttest.h"
#include "images/img_square8.h"
#include "images/img_zigzag.h"

#define TPS 60

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

const uint32_t* bg_images[] = {
        IMG_RGBM,
        IMG_DISTTEST,
        IMG_SQUARE8,
        IMG_ZIGZAG,
};


const uint32_t* bg_image_particles[] = {
        IMG_RGBM_PARTICLES,
        IMG_DISTTEST_PARTICLES,
        IMG_SQUARE8_PARTICLES,
        IMG_ZIGZAG_PARTICLES,
};

const uint32_t bg_image_particlecount[] = {
        IMG_RGBM_PARTICLE_COUNT,
        IMG_DISTTEST_PARTICLE_COUNT,
        IMG_SQUARE8_PARTICLE_COUNT,
        IMG_ZIGZAG_PARTICLE_COUNT,
};

int cur_bgimg = 0;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}



[[noreturn]] int main()
{
    stdio_init_all();

    // Binary info
    bi_decl(bi_program_description("Particle simulator using MPU6050 and a 32x32 LED HUB57 Matrix"));
    // TODO: add more binary info here, especially for pins

    /*
    // Interpolator example code
    interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config 
    interp_set_config(interp0, 0, &cfg);

    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(2000, alarm_callback, NULL, false);

    puts("Hello, world!");
     */

    // Initialize Button GPIOs
    gpio_init(BTN_RESET_PIN);
    gpio_init(BTN_SELECT_PIN);

    gpio_set_dir(BTN_RESET_PIN, GPIO_IN);
    gpio_set_dir(BTN_SELECT_PIN, GPIO_IN);

    gpio_pull_up(BTN_RESET_PIN);
    gpio_pull_up(BTN_SELECT_PIN);

    // Initialize MPU6050
    mpu.reset();

    // Initialize HUB75
    hub75_init();

    // Launch matrix driver main loop
    multicore_launch_core1(hub75_main);

    // Sleep a bit to wait for USB connection
    sleep_ms(3000);
    //multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

    display_background = IMG_RGBM;

    uint32_t frame = 0;

    while (true) {
        mpu.update();

        if (frame % (TPS/2) == 0) {
            printf("Accel: X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.ax, mpu.ay, mpu.az);
            printf("Norm:  X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.axn, mpu.ayn, mpu.azn);
            printf("Gyro:  X = % 3.6f, Y = % 3.6f, Z = % 3.6f\n", mpu.gx, mpu.gy, mpu.gz);

            printf("Temp:  % 2.8f\n", mpu.temp);
        }

        //pio_sm_put_blocking(hub75_pio, hub75_sm, 1);
        sleep_ms(1000/TPS);
        //pio_sm_put_blocking(hub75_pio, hub75_sm, 0);
        //sleep_ms(200);

        if (gpio_get(BTN_RESET_PIN) != btn_reset_pressed) {
            if (gpio_get(BTN_RESET_PIN)) {
                // Button down
                printf("Reset!\n");
            } else {
                // Button up
            }

            btn_reset_pressed = gpio_get(BTN_RESET_PIN);
        }

        if (gpio_get(BTN_SELECT_PIN) != btn_select_pressed) {
            if (gpio_get(BTN_SELECT_PIN)) {
                // Button down
                printf("Select!\n");
                cur_bgimg = (cur_bgimg+1)%BG_IMAGE_COUNT;
                printf("Selected background: %d\n", cur_bgimg);
            } else {
                // Button up
            }

            btn_select_pressed = gpio_get(BTN_SELECT_PIN);
        }

        multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);
        multicore_fifo_pop_blocking();
        // TODO: do simulation here

        display_background = bg_images[cur_bgimg];

        frame++;
    }

    // Blink code
    /*gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(250);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(250);
    }*/
}
