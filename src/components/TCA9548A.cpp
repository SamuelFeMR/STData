#include "TCA9548A.h"

void TCA9548A::init(i2c_master_bus_handle_t i2c_bus)
{
    i2c_device_config_t tca_config =
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
            &tca_config,
            &tca_device
        )
    );
}

void TCA9548A::selectChannel(uint8_t channel)
{
    uint8_t value = 1 << channel;

    return i2c_master_transmit(
        tca_device,
        &value,
        1,
        100
    );
}