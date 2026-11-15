#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"

#define TCS3200_PIN_OUT  GPIO_NUM_18
#define TCS3200_PIN_S2   GPIO_NUM_19
#define TCS3200_PIN_S3   GPIO_NUM_16
#define TCS3200_PIN_S0   GPIO_NUM_23
#define TCS3200_PIN_S1   GPIO_NUM_4
#define TCS3200_PIN_LED  GPIO_NUM_17

#define TCS3200_SAMPLE_TIME_MS 100

class TCS3200
{
public:

    // Initialize the sensor
    esp_err_t init();

    // Read RGB channels
    esp_err_t read();

    // Latest raw frequency measurements (Hz)
    uint32_t frequency[3] = {};

    // Normalized RGB values (0-255)
    uint8_t rgb[3] = {};

    // Storage for the last 5 raw measurements
    uint32_t local_measure_array[5][3] = {};

    // Pointer to the selected storage buffer
    uint32_t (*measure_array)[3] = local_measure_array;

private:

    // Select the sensor filter
    esp_err_t selectFilter(bool s2, bool s3);

    // Measure output frequency
    esp_err_t measureFrequency(uint32_t& frequency_hz);

    // PCNT peripheral
    pcnt_unit_handle_t pcnt_unit = nullptr;

    pcnt_channel_handle_t pcnt_channel = nullptr;

    bool initialized = false;
};