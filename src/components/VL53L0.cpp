#include "VL53L0.h

void VL53L0::init()
{

    // Create the device handle at the ToF address
    i2c_device_config_t ToF_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ToF_ADDR,
        .scl_speed_hz = I2C_FREQUENCY
    };







}