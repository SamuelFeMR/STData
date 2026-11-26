#include "i2cManage.h"

#include "freertos/task.h"

void i2cManage::init(void)
{
    i2c_master_bus_config_t bus_config = {};

    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_0;
    bus_config.sda_io_num = I2C_SDA;
    bus_config.scl_io_num = I2C_SCL;
    bus_config.glitch_ignore_cnt = 7;

    // Internal pull-ups (~45 kOhm) are weak for 400 kHz with several modules.
    // External 4.7 kOhm (or 2.2 kOhm) resistors on SDA/SCL to 3.3 V are recommended.
    bus_config.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(
        i2c_new_master_bus(&bus_config, &i2c_bus)
    );

    recover_mutex = xSemaphoreCreateMutex();
}

i2c_master_bus_handle_t i2cManage::getBus()
{
    return i2c_bus;
}

esp_err_t i2cManage::recover(void)
{
    if (i2c_bus == nullptr || recover_mutex == nullptr)
        return ESP_ERR_INVALID_STATE;

    // If another task is already recovering the bus, just wait for it
    if (xSemaphoreTake(recover_mutex, 0) != pdTRUE)
    {
        xSemaphoreTake(recover_mutex, portMAX_DELAY);
        xSemaphoreGive(recover_mutex);
        return ESP_OK;
    }

    // Sends clock pulses so a slave holding SDA low can finish its byte,
    // then resets the I2C controller state machine.
    esp_err_t err = i2c_master_bus_reset(i2c_bus);

    vTaskDelay(pdMS_TO_TICKS(2));

    xSemaphoreGive(recover_mutex);

    return err;
}
