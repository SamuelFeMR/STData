#include "gy87.h"

void MPU6050::init(i2c_master_bus_handle_t i2c_bus)
{
    /*
        0x6B	Wake up/Clock Configuration
        0x1B	Gyro Scale
        0x1C	Acc Scale
        0x1A	Gyro DLPF
        0x1D	Acc DLPF
    */

    // Create the device handle at the MPU address
    i2c_device_config_t mpu_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_FREQUENCY
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &mpu_config,
            &mpu_device
        )
    );

    // Wake up MPU6050
    uint8_t data[] = {
        0x6B,   // PWR_MGMT_1
        0x00    // Wake up
    };

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            sizeof(data),
            100
        )
    );

    // Gyroscope scale: ±250 °/s
    data[0] = 0x1B;   // GYRO_CONFIG
    data[1] = 0x00;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            sizeof(data),
            100
        )
    );

    // Accelerometer scale: ±2 g
    data[0] = 0x1C;   // ACCEL_CONFIG
    data[1] = 0x00;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            sizeof(data),
            100
        )
    );

    // Gyroscope DLPF
    data[0] = 0x1A;   // CONFIG
    data[1] = 0x03;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            sizeof(data),
            100
        )
    );

    // Accelerometer DLPF
    data[0] = 0x1D;   // ACCEL_CONFIG_2
    data[1] = 0x03;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            sizeof(data),
            100
        )
    );

    // Reset
    data[0] = 0x6B;
    data[1] = 0x80;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            2,
            100
        )
    );

    vTaskDelay(pdMS_TO_TICKS(100));

    // Select gyro X PLL clock
    data[0] = 0x6B;
    data[1] = 0x01;

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            mpu_device,
            data,
            2,
            100
        )
    );
}

esp_err_t MPU6050::read()
{
    uint8_t data[6];

    esp_err_t err;

    uint8_t reg = 0x3B;
    uint8_t data[14];

    esp_err_t err = i2c_master_transmit_receive(
        mpu_device,
        &reg,
        1,
        data,
        14,
        100
    );

    if (err != ESP_OK)
    {
        return err;
    }

    // Convert raw accelerometer values
    int16_t ax_raw = (int16_t)((data[0] << 8) | data[1]);
    int16_t ay_raw = (int16_t)((data[2] << 8) | data[3]);
    int16_t az_raw = (int16_t)((data[4] << 8) | data[5]);

    // Convert raw gyroscope values
    int16_t gx_raw = (int16_t)((data[8] << 8) | data[9]);
    int16_t gy_raw = (int16_t)((data[10] << 8) | data[11]);
    int16_t gz_raw = (int16_t)((data[12] << 8) | data[13]);

    // Convert to physical units
    // Accelerometer: ±2 g → 16384 LSB/g
    accel[0] = (float)ax_raw / 16384.0f;
    accel[1] = (float)ay_raw / 16384.0f;
    accel[2] = (float)az_raw / 16384.0f;

    // Gyroscope: ±250 °/s → 131 LSB/(°/s)
    gyro[0] = (float)gx_raw / 131.0f;
    gyro[1] = (float)gy_raw / 131.0f;
    gyro[2] = (float)gz_raw / 131.0f;

    return ESP_OK;
}

void GY87::init(i2c_master_bus_handle_t i2c_bus, TCA9548A& mux, uint8_t channel_mpu, uint8_t channel_hmc)
{
    // Links a given TCA channel index to the IMU. 
    this->mux = &mux;
    this->channel_mpu = channel_mpu;
    this->channel_hmc = channel_hmc;

    // Creates individual objects for each sensor that makes up the module
    mpu.init(i2c_bus);
    mag.init(i2c_bus);

}

esp_err_t GY87::read()
{
    esp_err_t err;

    // Select MPU6050 channel
    err = mux->selectChannel(channel_mpu);

    if (err != ESP_OK)
    {
        return err;
    }

    // Read MPU6050
    err = mpu.read();

    if (err != ESP_OK)
    { 
        return err;
    }

    // Select HMC5883L channel
    err = mux->selectChannel(channel_hmc);

    if (err != ESP_OK)
        return err;

    // Read HMC5883L
    err = mag.read();

    if (err != ESP_OK)
        return err;


    return ESP_OK;
}
