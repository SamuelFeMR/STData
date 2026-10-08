#include "gy87.h"

void MPU6050::init(i2c_master_bus_handle_t i2c_bus, TCA9548A mux, uint8_t channel)
{
    TCA9548A.selectChannel(channel);

    // Links a given TCA channel index to the IMU. 
    TCA_IMU = channel;
    
    i2c_device_config_t mpu_config =
    {
        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            TCA9548A_ADDR,

        .scl_speed_hz =
            I2C_FREQUENCY
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &mpu_config,
            &mpu_device
        )
    );
}

void MPU6050::read()
{



}