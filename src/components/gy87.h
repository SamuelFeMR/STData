#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "TCA9548A.h"

#define I2C_FREQUENCY 400000

#define MPU6050_ADDR   0x68 
#define MPU6050_ADDR2   0x69

class MPU6050
{
    public:
        // Init for adding a MPU defice on the i2c protocol
        void init(i2c_master_bus_handle_t i2c_bus);

        // Read the IMU data considering a TCA channel already linked to the devide.
        esp_err_t read();

        const float* getAccel();
        const float* getGyro();

    private:
        i2c_master_dev_handle_t mpu_device;

        float accel[3];
        float gyro[3];

};

class HMC5883L
{
    public:
        // Init for adding a HMC defice on the i2c protocol
        void init(i2c_master_bus_handle_t i2c_bus);
        
        // Read the HMC data considering a TCA channel already linked to the devide.
        esp_err_t read();

    private:
        i2c_master_dev_handle_t hmc_device;

};

class GY87
{
    public:
        void init(i2c_master_bus_handle_t i2c_bus, TCA9548A& mux, uint8_t channel_mpu, uint8_t channel_hmc);
        esp_err_t read();

    private:
        TCA9548A* mux;
        MPU6050 mpu;
        HMC5883L mag;
        uint8_t channel_mpu = 0;
        uint8_t channel_hmc = 0;

        last_read

};