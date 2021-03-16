#pragma once

#include <stdio.h>
#include "math.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#define MPU6050_I2C_INSTANCE i2c0
#define MPU6050_SDA_PIN 4
#define MPU6050_SCL_PIN 5

#define MPU6050_ADDR 0x68

#define MPU6050_ACCEL_MULT_2G       0.000061
#define MPU6050_ACCEL_MULT_4G       0.000122
#define MPU6050_ACCEL_MULT_8G       0.000244
#define MPU6050_ACCEL_MULT_16G      0.000488

#define MPU6050_GYRO_MULT_250DPS    0.007633
#define MPU6050_GYRO_MULT_500DPS    0.015267
#define MPU6050_GYRO_MULT_1000DPS   0.030487
#define MPU6050_GYRO_MULT_2000DPS   0.060975

class MPU6050 {
public:
    MPU6050();

    void reset();

    void update();
    void updateAccelerometer();
    void updateGyroscope();
    void updateTemperature();

    float ax{}, ay{}, az{};
    float axn{}, ayn{}, azn{};

    float gx{}, gy{}, gz{};

    double temp{};

private:
    static void write_reg8(uint8_t reg, uint8_t data);
    static void write_reg16(uint8_t reg, uint16_t data);

    static uint8_t read_reg8(uint8_t reg);
    static uint16_t read_reg16(uint8_t reg);
};