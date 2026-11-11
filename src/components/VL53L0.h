#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "TCA9548A.h"

#define I2C_FREQUENCY 400000
#define ToF_ADDR 0x29

class VL053L0
{
    public:
        // Initiates the device
        void init(i2c_master_bus_handle_t i2c_bus);

        // Perform a distance measurement
        esp_err_t read(i2c_master_bus_handle_t i2c_bus, );

        // Latest distance measurement in millimeters
        uint16_t distance_mm;

        // Storage for the last 5 measurements
        uint16_t local_measure_array[5];

        // Pointer to the selected storage buffer
        uint16_t* measure_array = local_measure_array;

    private:
        i2c_master_dev_handle_t tof_device;


};