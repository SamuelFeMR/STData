#include <stdio.h>
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
// COMPONENTES
// =====================================================

#include "components/TCA9548A.h"
#include "components/gy87.h"
#include "components/TCS3200.h"

// =====================================================
// VL53L0X
// =====================================================

#include "vl53l0x_api.h"
#include "vl53l0x_idf.h"


// =====================================================
// CONFIGURAÇÕES
// =====================================================

static const char *TAG = "MAIN";

#define CORE_0 0
#define CORE_1 1

#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY   1


// =====================================================
// INSTÂNCIAS DOS COMPONENTES
// =====================================================

// I2C
static i2cManage I2C;

// Multiplexador
static TCA9548A MUX;

// GY-87
static GY87 IMU;

// TCS3200
static TCS3200 COLOR;

// VL53L0X
static VL53L0X_Dev_t DISTANCE_ESQ;
static VL53L0X_Dev_t DISTANCE_DIR;


// =====================================================
// INICIALIZAÇÃO DOS VL53L0X
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
        return err;

    // Configuração da comunicação
    vl53l0x_idf_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .scl_speed_hz = 400000,
        .i2c_address_7bit = 0x29,
        .enable_internal_pullup = 1,
        .xshut_gpio_num = GPIO_NUM_NC,
        .gpio1_gpio_num = GPIO_NUM_NC,
        .existing_bus = bus
    };

    // Inicialização do driver
    err = vl53l0x_idf_init(sensor, &config);

    if (err != ESP_OK)
        return err;

    // Inicialização do dispositivo
    VL53L0X_Error status;

    status = VL53L0X_DataInit(sensor);

    if (status != VL53L0X_ERROR_NONE)
        return ESP_FAIL;

    status = VL53L0X_StaticInit(sensor);

    if (status != VL53L0X_ERROR_NONE)
        return ESP_FAIL;

    return ESP_OK;
}


// =====================================================
// INICIALIZAÇÃO DOS SENSORES
// =====================================================

static void initSensors()
{
    ESP_LOGI(TAG, "Inicializando sensores...");

    // I2C
    ESP_LOGI(TAG, "Inicializando I2C...");

    I2C.init();

    i2c_master_bus_handle_t bus = I2C.getBus();

    // TCA9548A
    ESP_LOGI(TAG, "Inicializando TCA9548A...");

    MUX.init(bus);

    // GY-87
    ESP_LOGI(TAG, "Inicializando GY-87...");

    ESP_ERROR_CHECK(
        IMU.init(bus)
    );

    // TCS3200
    ESP_LOGI(TAG, "Inicializando TCS3200...");

    ESP_ERROR_CHECK(
        COLOR.init()
    );


    // VL53L0X - SENSOR 1
    ESP_LOGI(TAG, "Inicializando VL53L0X - Canal 1...");

    ESP_ERROR_CHECK(
        initVL53L0X(&DISTANCE_ESQ
        , bus, 1)
    );


    // -------------------------------------------------
    // VL53L0X - SENSOR 2
    // -------------------------------------------------

    ESP_LOGI(TAG, "Inicializando VL53L0X - Canal 2...");

    ESP_ERROR_CHECK(
        initVL53L0X(&DIR, bus, 2)
    );


    ESP_LOGI(TAG, "Todos os sensores foram inicializados.");
}


// =====================================================
// TAREFA - NÚCLEO 0
// =====================================================

static void taskCore0(void *pvParameters)
{
    while (true)
    {
        // Rotinas do núcleo 0 serão implementadas aqui.

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// =====================================================
// TAREFA - NÚCLEO 1
// =====================================================

static void taskCore1(void *pvParameters)
{
    while (true)
    {
        // Rotinas do núcleo 1 serão implementadas aqui.

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// =====================================================
// APP MAIN
// =====================================================

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando sistema...");


    // -------------------------------------------------
    // INICIALIZAÇÃO DOS SENSORES
    // -------------------------------------------------

    initSensors();


    // -------------------------------------------------
    // INICIALIZAÇÃO DO FREERTOS
    // -------------------------------------------------

    ESP_LOGI(TAG, "Criando tarefas...");


    // Núcleo 0
    BaseType_t task0 = xTaskCreatePinnedToCore(
        taskCore0,
        "Task_Core_0",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY,
        nullptr,
        CORE_0
    );

    // Núcleo 1
    BaseType_t task1 = xTaskCreatePinnedToCore(
        taskCore1,
        "Task_Core_1",
        TASK_STACK_SIZE,
        nullptr,
        TASK_PRIORITY,
        nullptr,
        CORE_1
    );


    // Verificação das tarefas
    if (task0 != pdPASS || task1 != pdPASS)
    {
        ESP_LOGE(TAG, "Erro ao criar tarefas FreeRTOS!");
        return;
    }


    ESP_LOGI(TAG, "Sistema inicializado com sucesso.");
}