#include "i2cManage.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "I2C_SCAN";

static i2cManage I2C;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "       I2C SCANNER");
    ESP_LOGI(TAG, "==============================");

    ESP_LOGI(TAG, "Inicializando barramento I2C...");
    I2C.init();

    i2c_master_bus_handle_t bus = I2C.getBus();

    ESP_LOGI(TAG, "Procurando dispositivos...");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        esp_err_t err = i2c_master_probe(bus, addr, 100);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Dispositivo encontrado: 0x%02X", addr);
        }
    }

    ESP_LOGI(TAG, "Varredura concluida.");
}