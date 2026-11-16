#include <stdio.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "components/TCS3200.h"


// =====================================================
// CONFIGURAÇÕES
// =====================================================

static const char *TAG = "TCS3200_TEST";

#define SAMPLE_TIME_MS 200


// =====================================================
// INSTÂNCIAS
// =====================================================

static TCS3200 COLOR;


// =====================================================
// INICIALIZAÇÃO
// =====================================================

static esp_err_t initSensor()
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "       TESTE DO TCS3200");
    ESP_LOGI(TAG, "================================");

    ESP_LOGI(TAG, "Inicializando TCS3200...");

    esp_err_t err = COLOR.init();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Erro ao inicializar TCS3200: %s",
                 esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG, "TCS3200 inicializado com sucesso!");

    return ESP_OK;
}


// =====================================================
// LEITURA DO SENSOR
// =====================================================

static void readSensor()
{
    esp_err_t err = COLOR.read();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "Erro na leitura do TCS3200: %s",
                 esp_err_to_name(err));

        return;
    }


    // -------------------------------------------------
    // FREQUÊNCIAS
    // -------------------------------------------------

    uint32_t red_frequency   = COLOR.frequency[0];
    uint32_t green_frequency = COLOR.frequency[1];
    uint32_t blue_frequency  = COLOR.frequency[2];


    // -------------------------------------------------
    // RGB NORMALIZADO
    // -------------------------------------------------

    uint8_t red   = COLOR.rgb[0];
    uint8_t green = COLOR.rgb[1];
    uint8_t blue  = COLOR.rgb[2];


    // -------------------------------------------------
    // EXIBIÇÃO
    // -------------------------------------------------

    ESP_LOGI(TAG, "--------------------------------");

    ESP_LOGI(TAG,
        "FREQUENCY [Hz] R: %lu | G: %lu | B: %lu",
        (unsigned long)red_frequency,
        (unsigned long)green_frequency,
        (unsigned long)blue_frequency
    );

    ESP_LOGI(TAG,
        "RGB [0-255]    R: %u | G: %u | B: %u",
        red,
        green,
        blue
    );
}


// =====================================================
// APP MAIN
// =====================================================

extern "C" void app_main(void)
{
    // Inicialização
    esp_err_t err = initSensor();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha na inicialização. Teste interrompido.");
        return;
    }

    ESP_LOGI(TAG, "Iniciando leituras...");

    // Loop de leitura
    while (true)
    {
        readSensor();

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_TIME_MS));
    }
}