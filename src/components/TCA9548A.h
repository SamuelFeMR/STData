#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#define TCA9548A_ADDR   0x71

class TCA9548A
{
    public:
        // Init the MUX TCA for i2c readings
        void init(i2c_master_bus_handle_t i2c_bus);

        // Select the MUX channel for the I2C protocol to read
        void selectChannel(uint8_t channel);

        
    private:

        // I2C frequency
        static constexpr uint32_t I2C_FREQ_HZ = 400000;

        i2c_master_dev_handle_t tca_device;
};  