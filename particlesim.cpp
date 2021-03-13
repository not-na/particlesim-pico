#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"

#include "MPU6050.h"
#include "hub75.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

MPU6050 mpu;


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

    mpu.reset();

    hub75_init();

    multicore_launch_core1(hub75_main);

    sleep_ms(100);
    sleep_ms(3000);
    //multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);

    while (1) {
        mpu.update();

        printf("Accel: X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.ax, mpu.ay, mpu.az);
        printf("Norm:  X = % 1.8fg, Y = % 1.8fg, Z = % 1.8fg\n", mpu.axn, mpu.ayn, mpu.azn);
        printf("Gyro:  X = % 3.6f, Y = % 3.6f, Z = % 3.6f\n", mpu.gx, mpu.gy, mpu.gz);

        printf("Temp:  % 2.8f\n", mpu.temp);

        //pio_sm_put_blocking(hub75_pio, hub75_sm, 1);
        sleep_ms(1000/60);
        //pio_sm_put_blocking(hub75_pio, hub75_sm, 0);
        //sleep_ms(200);

        printf("Push\n");
        multicore_fifo_push_blocking(DISPLAY_TRIGGER_REDRAW_MAGIC_NUMBER);
        multicore_fifo_pop_blocking();
        // TODO: do simulation here
        printf("Pop\n");
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
