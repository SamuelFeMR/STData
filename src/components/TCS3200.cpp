#include "TCS3200.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"


// =====================================================
// INITIALIZATION
// =====================================================

esp_err_t TCS3200::init()
{
    esp_err_t err;

    // Configure control pins
    gpio_config_t io_config = {};

    io_config.pin_bit_mask =
        (1ULL << TCS3200_PIN_S0) |
        (1ULL << TCS3200_PIN_S1) |
        (1ULL << TCS3200_PIN_S2) |
        (1ULL << TCS3200_PIN_S3) |
        (1ULL << TCS3200_PIN_LED);

    io_config.mode = GPIO_MODE_OUTPUT;

    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.intr_type = GPIO_INTR_DISABLE;

    err = gpio_config(&io_config);

    if (err != ESP_OK)
        return err;


    // Configure sensor output pin
    gpio_config_t output_config = {};

    output_config.pin_bit_mask =
        (1ULL << TCS3200_PIN_OUT);

    output_config.mode = GPIO_MODE_INPUT;

    output_config.pull_up_en = GPIO_PULLUP_DISABLE;
    output_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    output_config.intr_type = GPIO_INTR_DISABLE;

    err = gpio_config(&output_config);

    if (err != ESP_OK)
        return err;


    // Frequency scaling: 20%
    gpio_set_level(TCS3200_PIN_S0, 1);
    gpio_set_level(TCS3200_PIN_S1, 0);

    // Enable sensor LED
    gpio_set_level(TCS3200_PIN_LED, 1);


    // =================================================
    // PCNT CONFIGURATION
    // =================================================

    pcnt_unit_config_t unit_config = {};

    unit_config.low_limit = -1;
    unit_config.high_limit = 30000;

    err = pcnt_new_unit(
        &unit_config,
        &pcnt_unit
    );

    if (err != ESP_OK)
        return err;


    // Configure PCNT input channel
    pcnt_chan_config_t channel_config = {};

    channel_config.edge_gpio_num = TCS3200_PIN_OUT;
    channel_config.level_gpio_num = -1;

    err = pcnt_new_channel(
        pcnt_unit,
        &channel_config,
        &pcnt_channel
    );

    if (err != ESP_OK)
        return err;


    // Count rising edges
    err = pcnt_channel_set_edge_action(
        pcnt_channel,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD
    );

    if (err != ESP_OK)
        return err;


    // Ignore short electrical glitches
    pcnt_glitch_filter_config_t filter_config = {};

    filter_config.max_glitch_ns = 1000;

    err = pcnt_unit_set_glitch_filter(
        pcnt_unit,
        &filter_config
    );

    if (err != ESP_OK)
        return err;


    // Enable PCNT
    err = pcnt_unit_enable(pcnt_unit);

    if (err != ESP_OK)
        return err;


    initialized = true;

    return ESP_OK;
}


// =====================================================
// FILTER SELECTION
// =====================================================

esp_err_t TCS3200::selectFilter(bool s2, bool s3)
{
    esp_err_t err;

    err = gpio_set_level(TCS3200_PIN_S2, s2);

    if (err != ESP_OK)
        return err;

    err = gpio_set_level(TCS3200_PIN_S3, s3);

    if (err != ESP_OK)
        return err;


    // Allow the sensor output to stabilize
    vTaskDelay(pdMS_TO_TICKS(2));

    return ESP_OK;
}


// =====================================================
// FREQUENCY MEASUREMENT
// =====================================================

esp_err_t TCS3200::measureFrequency(uint32_t& frequency_hz)
{
    if (!initialized)
        return ESP_ERR_INVALID_STATE;


    esp_err_t err;

    // Reset counter
    err = pcnt_unit_clear_count(pcnt_unit);

    if (err != ESP_OK)
        return err;


    // Start counting
    err = pcnt_unit_start(pcnt_unit);

    if (err != ESP_OK)
        return err;


    int64_t start_time = esp_timer_get_time();

    // Measurement window
    vTaskDelay(pdMS_TO_TICKS(TCS3200_SAMPLE_TIME_MS));

    int64_t end_time = esp_timer_get_time();


    // Stop counting
    err = pcnt_unit_stop(pcnt_unit);

    if (err != ESP_OK)
        return err;


    // Get pulse count
    int pulse_count = 0;

    err = pcnt_unit_get_count(
        pcnt_unit,
        &pulse_count
    );

    if (err != ESP_OK)
        return err;


    // Calculate actual frequency
    int64_t elapsed_time = end_time - start_time;

    if (elapsed_time <= 0)
        return ESP_FAIL;


    frequency_hz =
        ((uint64_t)pulse_count * 1000000ULL) /
        elapsed_time;


    return ESP_OK;
}


// =====================================================
// RGB READING
// =====================================================

esp_err_t TCS3200::read()
{
    if (!initialized)
        return ESP_ERR_INVALID_STATE;

    esp_err_t err;


    // =================================================
    // RED
    // S2 = LOW
    // S3 = LOW
    // =================================================

    err = selectFilter(false, false);

    if (err != ESP_OK)
        return err;

    err = measureFrequency(frequency[0]);

    if (err != ESP_OK)
        return err;


    // =================================================
    // GREEN
    // S2 = HIGH
    // S3 = HIGH
    // =================================================

    err = selectFilter(true, true);

    if (err != ESP_OK)
        return err;

    err = measureFrequency(frequency[1]);

    if (err != ESP_OK)
        return err;


    // =================================================
    // BLUE
    // S2 = LOW
    // S3 = HIGH
    // =================================================

    err = selectFilter(false, true);

    if (err != ESP_OK)
        return err;

    err = measureFrequency(frequency[2]);

    if (err != ESP_OK)
        return err;


    // =================================================
    // RGB NORMALIZATION
    // =================================================

    uint32_t max_frequency = frequency[0];

    if (frequency[1] > max_frequency)
        max_frequency = frequency[1];

    if (frequency[2] > max_frequency)
        max_frequency = frequency[2];


    if (max_frequency > 0)
    {
        for (int i = 0; i < 3; i++)
        {
            rgb[i] =
                (uint8_t)(
                    ((uint64_t)frequency[i] * 255) /
                    max_frequency
                );
        }
    }
    else
    {
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0;
    }


    // =================================================
    // UPDATE HISTORY
    // =================================================

    for (int i = 4; i > 0; i--)
    {
        for (int j = 0; j < 3; j++)
        {
            measure_array[i][j] =
                measure_array[i - 1][j];
        }
    }


    // Store newest measurement
    measure_array[0][0] = frequency[0];
    measure_array[0][1] = frequency[1];
    measure_array[0][2] = frequency[2];


    return ESP_OK;
}