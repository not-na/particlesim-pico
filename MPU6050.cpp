#include "MPU6050.h"

// Roughly based on Pico MPU6050 app note

MPU6050::MPU6050() {

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(MPU6050_SDA_PIN, MPU6050_SCL_PIN, GPIO_FUNC_I2C));
}

void MPU6050::reset() {
    i2c_init(MPU6050_I2C_INSTANCE, 400 * 1000);
    gpio_set_function(MPU6050_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPU6050_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPU6050_SDA_PIN);
    gpio_pull_up(MPU6050_SCL_PIN);

    // This function brings the entire MPU6050 into a known state
    this->ax = 0;
    this->ay = 0;
    this->az = 0;

    this->gx = 0;
    this->gy = 0;
    this->gz = 0;

    this->temp = 0;

    // Exit sleep mode
    write_reg8(0x6B, 0x00);
    // TODO: switch clock to gyroscope after a second or so

    // Signal Path Reset
    write_reg8(0x68, 0x07);

    // Accelerometer Configuration
    // Currently fixed at full-scale = +/- 2g
    write_reg8(0x1C, 0x00);

    // Filter config
    // DLPF_CFG = 2
    // -> Accelerometer Bandwidth=94Hz, Delay=3ms
    // Lower bandwidth would probably also be fine, since we only
    // measure tilt and the simulation has a lot of inertia as well
    write_reg8(0x1A, 0x02);
}

void MPU6050::update() {
    updateAccelerometer();
    updateGyroscope();
    updateTemperature();
}

void MPU6050::updateAccelerometer() {
    // Read raw registers
    int16_t ra_x = read_reg16(0x3B);
    int16_t ra_y = read_reg16(0x3D);
    int16_t ra_z = read_reg16(0x3F);

    // TODO: implement calibration and self-test functionality
    // Convert to floating-point to ease calculations
    this->ax = ra_x*MPU6050_ACCEL_MULT_2G;
    this->ay = ra_y*MPU6050_ACCEL_MULT_2G;
    this->az = ra_z*MPU6050_ACCEL_MULT_2G;

    // Calculate magnitude of vector
    float mag = sqrt(ax*ax+ay*ay+az*az);

    // Just in case we are in zero-g...
    if (mag == 0) {
        mag = 1;
    }

    this->axn = ax/mag;
    this->ayn = ay/mag;
    this->azn = az/mag;
}

void MPU6050::updateGyroscope() {
    int16_t ga_x = read_reg16(0x43);
    int16_t ga_y = read_reg16(0x45);
    int16_t ga_z = read_reg16(0x47);

    // TODO: implement calibration and self-test functionality

    this->gx = ga_x*MPU6050_GYRO_MULT_250DPS;
    this->gy = ga_y*MPU6050_GYRO_MULT_250DPS;
    this->gz = ga_z*MPU6050_GYRO_MULT_250DPS;
}

void MPU6050::updateTemperature() {
    int16_t traw = read_reg16(0x41);

    // Calculation based on Pico SDK example
    this->temp = (traw / 340.0) + 36.53;
}


void MPU6050::write_reg8(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;

    i2c_write_blocking(MPU6050_I2C_INSTANCE, MPU6050_ADDR, buf, 2, false);
}

void MPU6050::write_reg16(uint8_t reg, uint16_t data) {
    // Not yet implemented
    // Probably just write both bytes at once, like read_reg16
}

uint8_t MPU6050::read_reg8(uint8_t reg) {
    uint8_t out;

    i2c_write_blocking(MPU6050_I2C_INSTANCE, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(MPU6050_I2C_INSTANCE, MPU6050_ADDR, &out, 1, false);

    return out;
}

uint16_t MPU6050::read_reg16(uint8_t reg) {
    uint8_t buf[2];

    i2c_write_blocking(MPU6050_I2C_INSTANCE, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(MPU6050_I2C_INSTANCE, MPU6050_ADDR, buf, 2, false);

    return buf[0] << 8 | buf[1];
}


