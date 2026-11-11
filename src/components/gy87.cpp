#include "gy87.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t MPU6050::init(i2c_master_bus_handle_t i2c_bus)
{
    /*
        0x6B Wake up/Clock Configuration
        0x1B Gyro Scale
        0x1C Acc Scale
        0x1A Gyro DLPF
        0x1D Acc DLPF
    */
    if (i2c_bus == nullptr)
        return ESP_ERR_INVALID_ARG;

    // Create the device handle at the MPU address
    i2c_device_config_t mpu_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_FREQUENCY
    };

    esp_err_t err = i2c_master_bus_add_device(
        i2c_bus,
        &mpu_config,
        &mpu_device
    );

    if (err != ESP_OK)
        return err;

    uint8_t data[2];

    // Reset MPU6050
    data[0] = 0x6B;
    data[1] = 0x80;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(100));

    // Clock source: PLL with X-axis gyroscope reference
    data[0] = 0x6B;
    data[1] = 0x01;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    // Gyroscope scale: ±250 °/s
    data[0] = 0x1B;
    data[1] = 0x00;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    // Accelerometer scale: ±2 g
    data[0] = 0x1C;
    data[1] = 0x00;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    // Gyroscope DLPF
    data[0] = 0x1A;
    data[1] = 0x03;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    // Accelerometer DLPF
    data[0] = 0x1D;
    data[1] = 0x03;

    err = i2c_master_transmit(mpu_device, data, sizeof(data), 100);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

esp_err_t MPU6050::read()
{
    uint8_t reg = 0x3B;
    uint8_t data[14];
    
    // MPU6050 can be read through a 14 lengh byte register, where we can find both the GY and ACC measures
    // Through this process we avoid We avoid accessing the MPU registers twice, optimizing the sequence.
    
    // Read accelerometer, temperature and gyroscope registers
    esp_err_t err = i2c_master_transmit_receive(
        mpu_device,
        &reg,
        1,
        data,
        sizeof(data),
        100
    );

    if (err != ESP_OK)
    {
        return err;
    }

    // Convert raw accelerometer values
    int16_t ax_raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    int16_t ay_raw = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    int16_t az_raw = (int16_t)(((uint16_t)data[4] << 8) | data[5]);

    // Convert raw gyroscope values
    int16_t gx_raw = (int16_t)(((uint16_t)data[8] << 8) | data[9]);
    int16_t gy_raw = (int16_t)(((uint16_t)data[10] << 8) | data[11]);
    int16_t gz_raw = (int16_t)(((uint16_t)data[12] << 8) | data[13]);

    // Convert to physical units
    // Accelerometer: ±2 g
    accel[0] = (float)ax_raw / 16384.0f;
    accel[1] = (float)ay_raw / 16384.0f;
    accel[2] = (float)az_raw / 16384.0f;

    // Gyroscope: ±250 °/s
    gyro[0] = (float)gx_raw / 131.0f;
    gyro[1] = (float)gy_raw / 131.0f;
    gyro[2] = (float)gz_raw / 131.0f;

    // Shift previous measurements to older positions
    for (int i = 4; i > 0; i--)
    {
        for (int j = 0; j < 6; j++)
        {
            measure_array[i][j] = measure_array[i - 1][j];
        }
    }

    // Store the newest measurement at position 0
    measure_array[0][0] = accel[0];
    measure_array[0][1] = accel[1];
    measure_array[0][2] = accel[2];

    measure_array[0][3] = gyro[0];
    measure_array[0][4] = gyro[1];
    measure_array[0][5] = gyro[2];

    return ESP_OK;
}

esp_err_t MPU6050::enableBypass()
{
    uint8_t data[2] = {
        0x37,  // INT_PIN_CFG
        0x02   // I2C_BYPASS_EN = 1
    };

    return i2c_master_transmit(
        mpu_device,
        data,
        sizeof(data),
        100
    );
}

HMC5883L::init(i2c_master_bus_handle_t i2c_bus)
{
    if (i2c_bus == nullptr)
        return ESP_ERR_INVALID_ARG;

    // Configure the I2C device
    i2c_device_config_t hmc_config ={
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x1E,
        .scl_speed_hz = I2C_FREQUENCY
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device( i2c_bus, &hmc_config, &hmc_device));

    uint8_t data[2];

    // Configuration Register A
    // 8 samples averaged, 15 Hz output rate, normal measurement
    data[0] = 0x00;
    data[1] = 0x70;
    ESP_ERROR_CHECK(i2c_master_transmit( hmc_device, data, sizeof(data), 100));
    if (err != ESP_OK) return err;

    // Configuration Register B
    // Gain = +/- 1.3 Ga
    data[0] = 0x01;
    data[1] = 0x20;
    ESP_ERROR_CHECK(i2c_master_transmit( hmc_device, data, sizeof(data), 100));
    if (err != ESP_OK) return err;

    // Mode Register
    // Continuous measurement mode
    data[0] = 0x02;
    data[1] = 0x00;
    ESP_ERROR_CHECK(i2c_master_transmit( hmc_device, data, sizeof(data), 100));
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(10));

    return ESP_OK;
}

esp_err_t HMC5883L::read()
{
    uint8_t reg = 0x03;
    uint8_t data[6];

    // Read the six data registers
    esp_err_t err = i2c_master_transmit_receive(
        hmc_device,
        &reg,
        1,
        data,
        sizeof(data),
        100
    );

    if (err != ESP_OK)
    {
        return err;
    }

    // Convert raw data to signed 16-bit values
    int16_t x_raw = (int16_t)(
        ((uint16_t)data[0] << 8) | data[1]
    );

    int16_t z_raw = (int16_t)(
        ((uint16_t)data[2] << 8) | data[3]
    );

    int16_t y_raw = (int16_t)(
        ((uint16_t)data[4] << 8) | data[5]
    );

    // Store measurements in X, Y, Z order
    mag[0] = (float)x_raw;
    mag[1] = (float)y_raw;
    mag[2] = (float)z_raw;

    // Shift previous measurements to older positions
    for (int i = 4; i > 0; i--)
    {
        for (int j = 0; j < 3; j++)
        {
            measure_array[i][j] = measure_array[i - 1][j];
        }
    }

    // Store the newest measurement at position 0
    measure_array[0][0] = mag[0];
    measure_array[0][1] = mag[1];
    measure_array[0][2] = mag[2];

    return ESP_OK;
}

// Sem MUX
esp_err_t GY87::init(i2c_master_bus_handle_t i2c_bus)
{
    this->i2c_bus = i2c_bus;

    mux = nullptr;

    // Initialize MPU6050
    esp_err_t err = mpu.init(i2c_bus);

    if (err != ESP_OK)
        return err;

    // Enable I2C bypass
    err = mpu.enableBypass();

    if (err != ESP_OK)
        return err;

    // Initialize HMC5883L
    err = mag.init(i2c_bus);

    if (err != ESP_OK)
        return err;

    return ESP_OK;
}

// Com MUX
GY87::init(i2c_master_bus_handle_t i2c_bus, TCA9548A& mux, uint8_t channel)
{ esp_err_t GY87::init(
    i2c_master_bus_handle_t i2c_bus,
    TCA9548A& mux,
    uint8_t channel
)
{
    this->i2c_bus = i2c_bus;
    this->mux = &mux;
    this->channel = channel;

    // Select the channel before initializing the sensors
    esp_err_t err = mux.selectChannel(channel);

    if (err != ESP_OK)
        return err;

    // Initialize MPU6050
    err = mpu.init(i2c_bus);

    if (err != ESP_OK)
        return err;

    // Enable I2C bypass
    err = mpu.enableBypass();

    if (err != ESP_OK)
        return err;

    // Initialize HMC5883L
    err = mag.init(i2c_bus);

    if (err != ESP_OK)
        return err;

    return ESP_OK;
}
}

esp_err_t GY87::read()
{
    esp_err_t err;

    // Select MUX channel if available
    if (mux != nullptr)
    {
        err = mux->selectChannel(channel);

        if (err != ESP_OK)
            return err;
    }

    // Read MPU6050
    err = mpu.read();

    if (err != ESP_OK)
        return err;

    // Read HMC5883L
    err = mag.read();

    if (err != ESP_OK)
        return err;

    return ESP_OK;
}

MPU6050& GY87::getMPU()
{
    return mpu;
}

HMC5883L& GY87::getMag()
{
    return mag;
}