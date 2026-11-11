#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "TCA9548A.h"

#define I2C_FREQUENCY 400000

#define MPU6050_ADDR   0x68 
#define MPU6050_ADDR2   0x69

class MPU6050
{
    public:
        // Init for adding a MPU defice on the i2c protocol
        void init(i2c_master_bus_handle_t i2c_bus);

        // Read the IMU data considering a TCA channel already linked to the devide.
        esp_err_t read();

        // Magnetometer Bypass for data access 
        esp_err_t enableBypass();

        const float* getAccel();
        const float* getGyro();
    
        // Local measure buffer
        float local_measure_array[5][6];

        // Points to the buffer array
        float (*measure_array)[6] = local_measure_array;

        // Allow me to set an external array as buffer so it is automatically updated.
        // Also allow me to ask for the local_measure_array on externally if that's the buffer array.

    private:
        i2c_master_dev_handle_t mpu_device = nullptr;

        float accel[3];
        float gyro[3];

};

class HMC5883L
{
    public:
        // Init for adding a HMC device on the I2C protocol
        init(i2c_master_bus_handle_t i2c_bus);

        // Read the HMC data considering a TCA channel already linked to the device
        esp_err_t read();

        // Storage of the last 5 measurements
        float local_measure_array[5][3];

        // Pointer to the measurement storage
        float (*measure_array)[3] = local_measure_array;

    private:
        i2c_master_dev_handle_t hmc_device = nullptr;


        // Latest magnetometer measurements
        float mag[3];
};

class GY87
{
public:

    // Inicialização sem multiplexador
    esp_err_t init(i2c_master_bus_handle_t i2c_bus);

    // Inicialização com multiplexador
    esp_err_t init(i2c_master_bus_handle_t i2c_bus, TCA9548A& mux, uint8_t channel);

    // Leitura completa do módulo
    esp_err_t read();

    // Acesso aos dados do acelerômetro e giroscópio
    MPU6050& getMPU();

    // Acesso aos dados do magnetômetro
    HMC5883L& getMag();

private:

    i2c_master_bus_handle_t i2c_bus = nullptr;

    TCA9548A* mux = nullptr;

    uint8_t channel = 0;

    MPU6050 mpu;
    HMC5883L mag;
};