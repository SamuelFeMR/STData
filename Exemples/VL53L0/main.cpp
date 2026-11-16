#include <stdio.h>
#include <inttypes.h>

// =====================================================
// ESP-IDF
// =====================================================

#include "esp_log.h"
#include "esp_err.h"

// =====================================================
// FREERTOS
// =====================================================

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// =====================================================
// I2C
// =====================================================

#include "i2cManage.h"
#include "driver/i2c_master.h"

// =====================================================
// TCA9548A
// =====================================================

#include "components/TCA9548A.h"

// =====================================================
// VL53L0X
// =====================================================

#include "vl53l0x_api.h"
#include "vl53l0x_idf.h"


// =====================================================
// CONFIGURAÇÕES
// =====================================================

static const char *TAG = "VL53_TEST";

static constexpr uint8_t CHANNEL_ESQ = 0;
static constexpr uint8_t CHANNEL_DIR = 1;

static constexpr uint32_t READ_DELAY_MS = 100;


// =====================================================
// INSTÂNCIAS
// =====================================================

// I2C
static i2cManage i2cManager;

// Multiplexador
static TCA9548A MUX;

// Sensores VL53L0X
static VL53L0X_Dev_t DISTANCE_ESQ;
static VL53L0X_Dev_t DISTANCE_DIR;


// =====================================================
// TRATAMENTO DE ERROS DA BIBLIOTECA
// =====================================================

static void logPalError(VL53L0X_Error status)
{
    char error_string[VL53L0X_MAX_STRING_LENGTH] = {0};

    VL53L0X_GetPalErrorString(status, error_string);

    ESP_LOGE(
        TAG,
        "Erro VL53L0X: %d (%s)",
        status,
        error_string
    );
}


// =====================================================
// INICIALIZAÇÃO INTERNA DO SENSOR
// =====================================================

static esp_err_t startupVL53L0X(VL53L0X_Dev_t *sensor)
{
    VL53L0X_Error status;

    // Inicialização dos dados do dispositivo
    status = VL53L0X_DataInit(sensor);

    if (status != VL53L0X_ERROR_NONE)
    {
        logPalError(status);
        return ESP_FAIL;
    }

    // Inicialização estática
    status = VL53L0X_StaticInit(sensor);

    if (status != VL53L0X_ERROR_NONE)
    {
        logPalError(status);
        return ESP_FAIL;
    }

    // Calibração de referência
    uint8_t vhv_settings = 0;
    uint8_t phase_cal = 0;

    status = VL53L0X_PerformRefCalibration(
        sensor,
        &vhv_settings,
        &phase_cal
    );

    if (status != VL53L0X_ERROR_NONE)
    {
        logPalError(status);
        return ESP_FAIL;
    }

    // Gerenciamento dos SPADs de referência
    uint32_t ref_spad_count = 0;
    uint8_t is_aperture_spads = 0;

    status = VL53L0X_PerformRefSpadManagement(
        sensor,
        &ref_spad_count,
        &is_aperture_spads
    );

    if (status != VL53L0X_ERROR_NONE)
    {
        logPalError(status);
        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "SPADs: %" PRIu32 " | Aperture: %u",
        ref_spad_count,
        is_aperture_spads
    );

    // Configuração do modo de medição
    status = VL53L0X_SetDeviceMode(
        sensor,
        VL53L0X_DEVICEMODE_SINGLE_RANGING
    );

    if (status != VL53L0X_ERROR_NONE)
    {
        logPalError(status);
        return ESP_FAIL;
    }

    return ESP_OK;
}


// =====================================================
// INICIALIZAÇÃO DE UM VL53L0X
// =====================================================

static esp_err_t initVL53L0X(
    VL53L0X_Dev_t *sensor,
    i2c_master_bus_handle_t bus,
    uint8_t channel
)
{
    esp_err_t err;

    // Seleciona o canal do multiplexador
    err = MUX.selectChannel(channel);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao selecionar canal %u", channel);
        return err;
    }

    // Configuração da comunicação I2C
    vl53l0x_idf_config_t config = {};

    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = GPIO_NUM_21;
    config.scl_io_num = GPIO_NUM_22;
    config.scl_speed_hz = 400000;

    config.i2c_address_7bit = 0x29;
    config.enable_internal_pullup = 1;

    config.xshut_gpio_num = GPIO_NUM_NC;
    config.gpio1_gpio_num = GPIO_NUM_NC;

    // Utiliza o barramento já criado pelo i2cManage
    config.existing_bus = bus;

    // Inicialização do driver ESP-IDF
    err = vl53l0x_idf_init(sensor, &config);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha no driver do canal %u", channel);
        return err;
    }

    // Inicialização e calibração do sensor
    err = startupVL53L0X(sensor);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha na inicialização do canal %u", channel);
        return err;
    }

    ESP_LOGI(TAG, "VL53L0X do canal %u inicializado!", channel);

    return ESP_OK;
}


// =====================================================
// LEITURA DE UM VL53L0X
// =====================================================

static void readVL53L0X(
    VL53L0X_Dev_t *sensor,
    uint8_t channel,
    const char *name
)
{
    // Seleciona o canal antes de acessar o sensor
    esp_err_t err = MUX.selectChannel(channel);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: erro ao selecionar canal", name);
        return;
    }

    VL53L0X_RangingMeasurementData_t measurement = {};

    // Realiza uma medição
    VL53L0X_Error status =
        VL53L0X_PerformSingleRangingMeasurement(
            sensor,
            &measurement
        );

    if (status != VL53L0X_ERROR_NONE)
    {
        ESP_LOGE(TAG, "%s: falha na medição", name);
        logPalError(status);
        return;
    }

    ESP_LOGI(
        TAG,
        "%s | Distância: %u mm | RangeStatus: %u",
        name,
        (unsigned)measurement.RangeMilliMeter,
        (unsigned)measurement.RangeStatus
    );
}


// =====================================================
// INICIALIZAÇÃO DOS SENSORES
// =====================================================

static void initSensors()
{
    ESP_LOGI(TAG, "Inicializando I2C...");

    i2cManager.init();

    i2c_master_bus_handle_t bus = i2cManager.getBus();


    // -------------------------------------------------
    // TCA9548A
    // -------------------------------------------------

    ESP_LOGI(TAG, "Inicializando TCA9548A...");

    MUX.init(bus);


    // -------------------------------------------------
    // VL53L0X - CANAL 1
    // -------------------------------------------------

    ESP_LOGI(TAG, "Inicializando VL53L0X esquerdo...");

    ESP_ERROR_CHECK(
        initVL53L0X(
            &DISTANCE_ESQ,
            bus,
            CHANNEL_ESQ
        )
    );


    // -------------------------------------------------
    // VL53L0X - CANAL 2
    // -------------------------------------------------

    ESP_LOGI(TAG, "Inicializando VL53L0X direito...");

    ESP_ERROR_CHECK(
        initVL53L0X(
            &DISTANCE_DIR,
            bus,
            CHANNEL_DIR
        )
    );


    ESP_LOGI(TAG, "Inicialização concluída!");
}


// =====================================================
// APP MAIN
// =====================================================

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "       TESTE DOS VL53L0X        ");
    ESP_LOGI(TAG, "================================");

    // Inicializa os sensores
    initSensors();


    // -------------------------------------------------
    // LOOP DE LEITURA
    // -------------------------------------------------

    while (true)
    {
        ESP_LOGI(TAG, "----------- LEITURAS -----------");

        // Sensor do canal 1
        readVL53L0X(
            &DISTANCE_ESQ,
            CHANNEL_ESQ,
            "ESQUERDO"
        );

        // Sensor do canal 2
        readVL53L0X(
            &DISTANCE_DIR,
            CHANNEL_DIR,
            "DIREITO"
        );

        vTaskDelay(
            pdMS_TO_TICKS(READ_DELAY_MS)
        );
    }
}