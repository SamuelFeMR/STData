#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "TCA9548A.h"

#define I2C_FREQUENCY 400000
#define ToF_ADDR 0x29

class VL53L0
{
public:
    // Init without MUX
    esp_err_t init(i2c_master_bus_handle_t i2c_bus);

    // Init with MUX
    esp_err_t init(
        i2c_master_bus_handle_t i2c_bus,
        TCA9548A& mux,
        uint8_t channel
    );

    // Perform a distance measurement
    esp_err_t read();

    // Latest valid distance measurement
    uint16_t distance_mm = 0;

    // Storage for the last 5 measurements
    uint16_t local_measure_array[5] = {};

    // Pointer to the selected storage buffer
    uint16_t* measure_array = local_measure_array;

private:
    i2c_master_bus_handle_t i2c_bus = nullptr;

    i2c_master_dev_handle_t tof_device = nullptr;

    TCA9548A* mux = nullptr;
    uint8_t channel = 0;
};