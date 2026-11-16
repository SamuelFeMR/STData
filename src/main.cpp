#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_timer.h"

#include "i2cManage.h"
#include "components/TCA9548A.h"
#include "components/gy87.h"
#include "components/TCS3200.h"

#include "vl53l0x_api.h"
#include "vl53l0x_idf.h"


// =====================================================
// CONFIGURATION
// =====================================================

#define SERIAL_BAUD_RATE 115200

#define CORE_SENSOR 0
#define CORE_DISTANCE 1

#define IMU_TASK_PRIORITY       5
#define VL53_TASK_PRIORITY      5
#define COLOR_TASK_PRIORITY     2
#define SERIAL_TASK_PRIORITY    1

#define SERIAL_PERIOD_MS 20

// =====================================================
// HARDWARE OBJECTS
// =====================================================

static i2cManage I2CDevice;
static TCA9548A MUX;

static GY87 IMU;
static TCS3200 COLOR;

static VL53L0X_Dev_t DISTANCE_1;
static VL53L0X_Dev_t DISTANCE_2;

// =====================================================
// SENSOR DATA
// =====================================================

struct SensorData
{
    // Timestamps
    int64_t imu_timestamp = 0;
    int64_t tof_timestamp = 0;
    int64_t color_timestamp = 0;

    // Accelerometer
    float accel[3] = {};

    // Gyroscope
    float gyro[3] = {};

    // Magnetometer
    float mag[3] = {};

    // VL53L0X distances (mm)
    uint16_t distance_1 = 0;
    uint16_t distance_2 = 0;

    // TCS3200 frequencies
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;

    // Sensor status
    uint8_t imu_ok = 0;
    uint8_t tof_ok = 0;
    uint8_t color_ok = 0;
};

static SensorData sensor_data;


// Protects the shared data structure
static portMUX_TYPE data_mutex = portMUX_INITIALIZER_UNLOCKED;


// VL53L0X INITIALIZATION
static esp_err_t initVL53L0X(
    VL53L0X_Dev_t* sensor,
    i2c_master_bus_handle_t bus,
    uint8_t channel)
{
    esp_err_t err;

    // Select the multiplexer channel
    err = MUX.selectChannel(channel);

    if (err != ESP_OK)
        return err;

    // Configure the sensor
    vl53l0x_idf_config_t config = {};

    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = GPIO_NUM_21;
    config.scl_io_num = GPIO_NUM_22;
    config.scl_speed_hz = 400000;

    config.i2c_address_7bit = 0x29;
    config.enable_internal_pullup = 1;

    config.xshut_gpio_num = GPIO_NUM_NC;
    config.gpio1_gpio_num = GPIO_NUM_NC;

    config.existing_bus = bus;


    // Initialize I2C interface
    err = vl53l0x_idf_init(sensor, &config);

    if (err != ESP_OK)
        return err;


    // Initialize sensor
    if (VL53L0X_DataInit(sensor) != VL53L0X_ERROR_NONE)
        return ESP_FAIL;

    if (VL53L0X_StaticInit(sensor) != VL53L0X_ERROR_NONE)
        return ESP_FAIL;


    // Reference calibration
    uint8_t vhv_settings = 0;
    uint8_t phase_cal = 0;

    if (VL53L0X_PerformRefCalibration(
            sensor,
            &vhv_settings,
            &phase_cal
        ) != VL53L0X_ERROR_NONE)
        return ESP_FAIL;


    // SPAD management
    uint32_t ref_spad_count = 0;
    uint8_t is_aperture = 0;

    if (VL53L0X_PerformRefSpadManagement(
            sensor,
            &ref_spad_count,
            &is_aperture
        ) != VL53L0X_ERROR_NONE)
        return ESP_FAIL;


    // Single ranging mode
    if (VL53L0X_SetDeviceMode(
            sensor,
            VL53L0X_DEVICEMODE_SINGLE_RANGING
        ) != VL53L0X_ERROR_NONE)
        return ESP_FAIL;


    return ESP_OK;
}


// VL53L0X MEASUREMENT
static esp_err_t readVL53L0X(
    VL53L0X_Dev_t* sensor,
    uint16_t* distance
)
{
    VL53L0X_RangingMeasurementData_t measurement;

    VL53L0X_Error status;

    status = VL53L0X_PerformSingleRangingMeasurement(
        sensor,
        &measurement
    );

    if (status != VL53L0X_ERROR_NONE)
        return ESP_FAIL;

    if (measurement.RangeStatus != 0)
        return ESP_FAIL;

    *distance = measurement.RangeMilliMeter;

    return ESP_OK;
}


// IMU TASK - CORE 0
static void imuTask(void* parameter)
{
    while (true)
    {
        esp_err_t err = IMU.read();

        if (err == ESP_OK)
        {
            const float* accel = IMU.getMPU().getAccel();
            const float* gyro = IMU.getMPU().getGyro();

            // Magnetometer is stored in its local buffer
            const float* mag = IMU.getMag().measure_array[0];

            portENTER_CRITICAL(&data_mutex);

            sensor_data.accel[0] = accel[0];
            sensor_data.accel[1] = accel[1];
            sensor_data.accel[2] = accel[2];

            sensor_data.gyro[0] = gyro[0];
            sensor_data.gyro[1] = gyro[1];
            sensor_data.gyro[2] = gyro[2];

            sensor_data.mag[0] = mag[0];
            sensor_data.mag[1] = mag[1];
            sensor_data.mag[2] = mag[2];

            sensor_data.imu_timestamp = esp_timer_get_time();
            sensor_data.imu_ok = 1;

            portEXIT_CRITICAL(&data_mutex);
        }
        else
        {
            portENTER_CRITICAL(&data_mutex);
            sensor_data.imu_ok = 0;
            portEXIT_CRITICAL(&data_mutex);
        }

        // Short interval to yield CPU time
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


// VL53L0X TASK - CORE 1
static void distanceTask(void* parameter)
{
    uint16_t distance_1 = 0;
    uint16_t distance_2 = 0;

    while (true)
    {
        esp_err_t err_1;
        esp_err_t err_2;

        // Sensor 1 - MUX channel 0
        err_1 = MUX.selectChannel(0);

        if (err_1 == ESP_OK)
            err_1 = readVL53L0X(&DISTANCE_1, &distance_1);


        // Sensor 2 - MUX channel 1
        err_2 = MUX.selectChannel(1);

        if (err_2 == ESP_OK)
            err_2 = readVL53L0X(&DISTANCE_2, &distance_2);


        portENTER_CRITICAL(&data_mutex);

        if (err_1 == ESP_OK)
            sensor_data.distance_1 = distance_1;

        if (err_2 == ESP_OK)
            sensor_data.distance_2 = distance_2;

        sensor_data.tof_ok =
            (err_1 == ESP_OK && err_2 == ESP_OK);

        sensor_data.tof_timestamp = esp_timer_get_time();

        portEXIT_CRITICAL(&data_mutex);

        // No additional delay:
        // the sensor measurement time determines the acquisition rate.
    }
}


// COLOR SENSOR TASK - CORE 0
static void colorTask(void* parameter)
{
    while (true)
    {
        esp_err_t err = COLOR.read();

        portENTER_CRITICAL(&data_mutex);

        if (err == ESP_OK)
        {
            sensor_data.red = COLOR.frequency[0];
            sensor_data.green = COLOR.frequency[1];
            sensor_data.blue = COLOR.frequency[2];

            sensor_data.color_timestamp = esp_timer_get_time();
            sensor_data.color_ok = 1;
        }
        else
        {
            sensor_data.color_ok = 0;
        }

        portEXIT_CRITICAL(&data_mutex);

        // TCS3200 already spends approximately 300 ms reading RGB.
        // No additional delay is necessary.
    }
}


// SERIAL OUTPUT TASK - CORE 0
static void serialTask(void* parameter)
{
    SensorData output;

    while (true)
    {
        // Copy the latest sensor data
        portENTER_CRITICAL(&data_mutex);
        output = sensor_data;
        portEXIT_CRITICAL(&data_mutex);


        // CSV output
        printf(
            "%lld,"
            "%.4f,%.4f,%.4f,"
            "%.4f,%.4f,%.4f,"
            "%.1f,%.1f,%.1f,"
            "%u,%u,"
            "%lu,%lu,%lu,"
            "%u,%u,%u\n",

            (long long)esp_timer_get_time(),

            output.accel[0],
            output.accel[1],
            output.accel[2],

            output.gyro[0],
            output.gyro[1],
            output.gyro[2],

            output.mag[0],
            output.mag[1],
            output.mag[2],

            output.distance_1,
            output.distance_2,

            (unsigned long)output.red,
            (unsigned long)output.green,
            (unsigned long)output.blue,

            output.imu_ok,
            output.tof_ok,
            output.color_ok
        );


        vTaskDelay(pdMS_TO_TICKS(SERIAL_PERIOD_MS));
    }
}


// MAIN
extern "C" void app_main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("       STDATA SENSOR ACQUISITION        \n");
    printf("========================================\n");

    // I2C INITIALIZATION
    I2CDevice.init();

    i2c_master_bus_handle_t bus = I2CDevice.getBus();

    // MULTIPLEXER
    MUX.init(bus);

    esp_err_t err = i2c_master_probe(bus, 0x68, 100);

    if (err == ESP_OK)
        printf("[I2C] MPU6050 encontrado em 0x68\n");
    else
        printf("[I2C] MPU6050 NÃO encontrado. Erro: %s\n",
            esp_err_to_name(err));

    // GY87
    ESP_ERROR_CHECK(IMU.init(bus));

    // VL53L0X
    ESP_ERROR_CHECK(initVL53L0X(&DISTANCE_1, bus, 0));

    ESP_ERROR_CHECK(initVL53L0X(&DISTANCE_2, bus, 1));

    // TCS3200
    ESP_ERROR_CHECK(COLOR.init());

    printf("All sensors initialized successfully.\n");

    printf("Starting acquisition tasks...\n\n");

    // FREERTOS TASKS
    xTaskCreatePinnedToCore(
        imuTask,
        "IMU",
        4096,
        nullptr,
        IMU_TASK_PRIORITY,
        nullptr,
        CORE_SENSOR
    );

    xTaskCreatePinnedToCore(
        colorTask,
        "COLOR",
        4096,
        nullptr,
        COLOR_TASK_PRIORITY,
        nullptr,
        CORE_SENSOR
    );

    xTaskCreatePinnedToCore(
        distanceTask,
        "DISTANCE",
        6144,
        nullptr,
        VL53_TASK_PRIORITY,
        nullptr,
        CORE_DISTANCE
    );

    xTaskCreatePinnedToCore(
        serialTask,
        "SERIAL",
        4096,
        nullptr,
        SERIAL_TASK_PRIORITY,
        nullptr,
        CORE_SENSOR
    );
}