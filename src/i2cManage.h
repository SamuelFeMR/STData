#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

class i2cManage
{
    public:
        // Init the i2c protocol, creating the i2c bus instance
        void init(void);
        i2c_master_bus_handle_t getBus();

    private:

        // I2C pins
        static constexpr gpio_num_t I2C_SDA = GPIO_NUM_21;
        static constexpr gpio_num_t I2C_SCL = GPIO_NUM_22;

        // i2c bus
        i2c_master_bus_handle_t i2c_bus;
};