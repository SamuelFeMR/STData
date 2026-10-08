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
        // Also link the TCA channel to the device
        void init(i2c_master_bus_handle_t i2c_bus, TCA9548A mux, uint8_t channel);

        // Read the IMU data considering a TCA channel already linked to the devide.
        void read();

    private:
        i2c_master_dev_handle_t mpu_device;
        uint8_t TCA_IMU = 0;
        TCA9548A mux;

}

class HMC5883L
{
    public:
        void init();
        void read();

    private:
        i2c_master_dev_handle_t hmc_device;

}