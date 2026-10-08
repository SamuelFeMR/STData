#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/rmt_rx.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "vl53l1x.h"

// UART
#define UART_BAUDRATE 115200

// GY-31
#define COLOR_OUT   GPIO_NUM_19

#define COLOR_S2    GPIO_NUM_18
#define COLOR_S3    GPIO_NUM_17

#define COLOR_S0    GPIO_NUM_16
#define COLOR_S1    GPIO_NUM_4

// I2C
#define I2C_PORT        I2C_NUM_0
#define I2C_SDA         GPIO_NUM_21
#define I2C_SCL         GPIO_NUM_22

#define I2C_FREQUENCY   400000

// TCA9548A
#define TCA9548A_ADDR   0x71

#define TCA_IMU1        0
#define TCA_IMU2        1
#define TCA_TOF1        2
#define TCA_TOF2        3

// MPU9250
#define MPU9250_ADDR    0x68

#define MPU_WHO_AM_I    0x75

#define MPU_PWR_MGMT_1  0x6B
#define MPU_PWR_MGMT_2  0x6C

#define MPU_CONFIG      0x1A

#define MPU_GYRO_CONFIG 0x1B

#define MPU_ACCEL_CONFIG    0x1C
#define MPU_ACCEL_CONFIG2   0x1D

#define MPU_ACCEL_XOUT_H    0x3B
#define MPU_GYRO_XOUT_H     0x43

// AK8963
#define AK8963_ADDR     0x0C

#define AK_WHO_AM_I     0x00

#define AK_ST1          0x02

#define AK_XOUT_L       0x03

#define AK_ST2          0x09

#define AK_CNTL1        0x0A

#define AK_ASAX         0x10

// VL53L1X
#define VL53L1X_ADDR    0x29

#define TOF1_CHANNEL    TCA_TOF1
#define TOF2_CHANNEL    TCA_TOF2

// TAG
static const char *TAG = "SENSOR";

/* HANDLES */
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t tca_dev;
static i2c_master_dev_handle_t mpu_dev;
static i2c_master_dev_handle_t mag_dev;


/* ESTRUTURA DOS DADOS */
typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;

    int16_t mx;
    int16_t my;
    int16_t mz;

} imu_data_t;


/* I2C - ESCRITA */
static esp_err_t i2c_write_reg(
    i2c_master_dev_handle_t dev,
    uint8_t reg,
    uint8_t value)
{
    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    return i2c_master_transmit(
        dev,
        data,
        2,
        100
    );
}

/* I2C - LEITURA */
static esp_err_t i2c_read_reg(
    i2c_master_dev_handle_t dev,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    return i2c_master_transmit_receive(
        dev,
        &reg,
        1,
        data,
        len,
        100
    );
}


/*
 * ============================================================
 * TCA9548A
 * ============================================================
 */

static esp_err_t tca_select(uint8_t channel)
{
    uint8_t value = 1 << channel;

    return i2c_master_transmit(
        tca_dev,
        &value,
        1,
        100
    );
}


static esp_err_t tca_disable_all(void)
{
    uint8_t value = 0;

    return i2c_master_transmit(
        tca_dev,
        &value,
        1,
        100
    );
}


/*
 * ============================================================
 * MPU9250
 * ============================================================
 */

static esp_err_t mpu9250_init(void)
{
    uint8_t who;

    esp_err_t err;

    err = i2c_read_reg(
        mpu_dev,
        MPU_WHO_AM_I,
        &who,
        1
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro lendo MPU9250");

        return err;
    }

    ESP_LOGI(
        TAG,
        "MPU9250 WHO_AM_I = 0x%02X",
        who
    );

    if (who != 0x71 && who != 0x73)
    {
        ESP_LOGE(
            TAG,
            "MPU9250 nao encontrado"
        );

        return ESP_ERR_NOT_FOUND;
    }


    // Reset
    i2c_write_reg(
        mpu_dev,
        MPU_PWR_MGMT_1,
        0x80
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );


    // Clock PLL
    i2c_write_reg(
        mpu_dev,
        MPU_PWR_MGMT_1,
        0x01
    );


    // Ativa acelerometro + gyro
    i2c_write_reg(
        mpu_dev,
        MPU_PWR_MGMT_2,
        0x00
    );


    // DLPF gyro
    i2c_write_reg(
        mpu_dev,
        MPU_CONFIG,
        0x03
    );


    // Gyro ±250 dps
    i2c_write_reg(
        mpu_dev,
        MPU_GYRO_CONFIG,
        0x00
    );


    // Acc ±2g
    i2c_write_reg(
        mpu_dev,
        MPU_ACCEL_CONFIG,
        0x00
    );


    // Acc DLPF
    i2c_write_reg(
        mpu_dev,
        MPU_ACCEL_CONFIG2,
        0x03
    );


    return ESP_OK;
}


/*
 * ============================================================
 * MPU9250 - ACELEROMETRO
 * ============================================================
 */

static esp_err_t mpu9250_read_accel(
    int16_t *ax,
    int16_t *ay,
    int16_t *az)
{
    uint8_t data[6];

    esp_err_t err;

    err = i2c_read_reg(
        mpu_dev,
        MPU_ACCEL_XOUT_H,
        data,
        6
    );

    if (err != ESP_OK)
        return err;


    *ax =
        ((int16_t)data[0] << 8) |
        data[1];

    *ay =
        ((int16_t)data[2] << 8) |
        data[3];

    *az =
        ((int16_t)data[4] << 8) |
        data[5];


    return ESP_OK;
}


/*
 * ============================================================
 * MPU9250 - GIROSCOPIO
 * ============================================================
 */

static esp_err_t mpu9250_read_gyro(
    int16_t *gx,
    int16_t *gy,
    int16_t *gz)
{
    uint8_t data[6];

    esp_err_t err;

    err = i2c_read_reg(
        mpu_dev,
        MPU_GYRO_XOUT_H,
        data,
        6
    );

    if (err != ESP_OK)
        return err;


    *gx =
        ((int16_t)data[0] << 8) |
        data[1];

    *gy =
        ((int16_t)data[2] << 8) |
        data[3];

    *gz =
        ((int16_t)data[4] << 8) |
        data[5];


    return ESP_OK;
}


/*
 * ============================================================
 * AK8963
 *
 * O AK8963 está dentro do MPU9250.
 *
 * Colocamos o MPU em BYPASS para acessar o AK diretamente.
 * ============================================================
 */

static esp_err_t ak8963_init(void)
{
    uint8_t who;

    esp_err_t err;


    // Habilita bypass I2C
    i2c_write_reg(
        mpu_dev,
        0x37,
        0x02
    );

    vTaskDelay(
        pdMS_TO_TICKS(10)
    );


    // WHO AM I
    err = i2c_read_reg(
        mag_dev,
        AK_WHO_AM_I,
        &who,
        1
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro lendo AK8963"
        );

        return err;
    }


    ESP_LOGI(
        TAG,
        "AK8963 WHO_AM_I = 0x%02X",
        who
    );


    if (who != 0x48)
    {
        ESP_LOGE(
            TAG,
            "AK8963 nao encontrado"
        );

        return ESP_ERR_NOT_FOUND;
    }


    // Power down
    i2c_write_reg(
        mag_dev,
        AK_CNTL1,
        0x00
    );

    vTaskDelay(
        pdMS_TO_TICKS(10)
    );


    // Continuous measurement 2
    // 16 bits / 100 Hz
    i2c_write_reg(
        mag_dev,
        AK_CNTL1,
        0x16
    );

    vTaskDelay(
        pdMS_TO_TICKS(10)
    );


    return ESP_OK;
}


/*
 * ============================================================
 * AK8963 - MAGNETOMETRO
 * ============================================================
 */

static esp_err_t ak8963_read(
    int16_t *mx,
    int16_t *my,
    int16_t *mz)
{
    uint8_t st1;

    uint8_t data[7];


    // Data ready
    i2c_read_reg(
        mag_dev,
        AK_ST1,
        &st1,
        1
    );


    if (!(st1 & 0x01))
    {
        return ESP_ERR_NOT_FINISHED;
    }


    /*
     * XOUT_L ... ZOUT_H + ST2
     */

    esp_err_t err =
        i2c_read_reg(
            mag_dev,
            AK_XOUT_L,
            data,
            7
        );


    if (err != ESP_OK)
        return err;


    // Overflow
    if (data[6] & 0x08)
    {
        return ESP_ERR_INVALID_STATE;
    }


    /*
     * AK8963 é little endian
     */

    *mx =
        ((int16_t)data[1] << 8) |
        data[0];

    *my =
        ((int16_t)data[3] << 8) |
        data[2];

    *mz =
        ((int16_t)data[5] << 8) |
        data[4];


    return ESP_OK;
}


/*
 * ============================================================
 * LEITURA COMPLETA DA IMU
 * ============================================================
 */

static void imu_read(
    imu_data_t *imu)
{
    memset(
        imu,
        0,
        sizeof(imu_data_t)
    );


    mpu9250_read_accel(
        &imu->ax,
        &imu->ay,
        &imu->az
    );


    mpu9250_read_gyro(
        &imu->gx,
        &imu->gy,
        &imu->gz
    );


    /*
     * O magnetometro atualiza mais lentamente.
     * Se nao houver dado novo, mantemos zero.
     */
    ak8963_read(
        &imu->mx,
        &imu->my,
        &imu->mz
    );
}


/*
 * ============================================================
 * GY-31
 *
 * Medimos o período do sinal OUT.
 *
 * O TCS3200/TCS34725-style GY-31 gera frequência.
 * pulseIn() do Arduino retornava o tempo em LOW.
 *
 * Aqui usamos polling com micros().
 *
 * ============================================================
 */

static uint32_t color_measure_period(void)
{
    uint32_t start;
    uint32_t timeout;

    /*
     * Espera LOW
     */
    timeout = esp_timer_get_time() + 100000;

    while (gpio_get_level(COLOR_OUT) != 0)
    {
        if (esp_timer_get_time() > timeout)
            return 0;
    }


    /*
     * Espera HIGH
     */
    timeout = esp_timer_get_time() + 100000;

    while (gpio_get_level(COLOR_OUT) == 0)
    {
        if (esp_timer_get_time() > timeout)
            return 0;
    }


    /*
     * Começo do HIGH
     */
    start = esp_timer_get_time();


    /*
     * Espera LOW novamente
     */
    timeout = start + 100000;

    while (gpio_get_level(COLOR_OUT) != 0)
    {
        if (esp_timer_get_time() > timeout)
            return 0;
    }


    /*
     * Período aproximado.
     *
     * Para manter compatibilidade com o pulseIn()
     * do seu código original, podemos usar o tempo
     * do ciclo inteiro.
     */

    return esp_timer_get_time() - start;
}


/*
 * ============================================================
 * SELECIONA COR
 * ============================================================
 */

static void color_red(void)
{
    gpio_set_level(
        COLOR_S2,
        0
    );

    gpio_set_level(
        COLOR_S3,
        0
    );
}


static void color_green(void)
{
    gpio_set_level(
        COLOR_S2,
        1
    );

    gpio_set_level(
        COLOR_S3,
        1
    );
}


static void color_blue(void)
{
    gpio_set_level(
        COLOR_S2,
        0
    );

    gpio_set_level(
        COLOR_S3,
        1
    );
}


/*
 * ============================================================
 * LEITURA GY-31
 * ============================================================
 */

static uint32_t read_red(void)
{
    color_red();

    vTaskDelay(
        pdMS_TO_TICKS(2)
    );

    return color_measure_period();
}


static uint32_t read_green(void)
{
    color_green();

    vTaskDelay(
        pdMS_TO_TICKS(2)
    );

    return color_measure_period();
}


static uint32_t read_blue(void)
{
    color_blue();

    vTaskDelay(
        pdMS_TO_TICKS(2)
    );

    return color_measure_period();
}


/*
 * ============================================================
 * CONFIGURA GY-31
 * ============================================================
 */

static void color_init(void)
{
    gpio_config_t output_config =
    {
        .pin_bit_mask =
            (1ULL << COLOR_S0) |
            (1ULL << COLOR_S1) |
            (1ULL << COLOR_S2) |
            (1ULL << COLOR_S3),

        .mode =
            GPIO_MODE_OUTPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    gpio_config(
        &output_config
    );


    gpio_config_t input_config =
    {
        .pin_bit_mask =
            (1ULL << COLOR_OUT),

        .mode =
            GPIO_MODE_INPUT,

        .pull_up_en =
            GPIO_PULLUP_DISABLE,

        .pull_down_en =
            GPIO_PULLDOWN_DISABLE,

        .intr_type =
            GPIO_INTR_DISABLE
    };


    gpio_config(
        &input_config
    );


    /*
     * Escala 20%
     *
     * S0 = HIGH
     * S1 = LOW
     */

    gpio_set_level(
        COLOR_S0,
        1
    );

    gpio_set_level(
        COLOR_S1,
        0
    );
}


/*
 * ============================================================
 * I2C INIT
 * ============================================================
 */

static void i2c_init(void)
{
    i2c_master_bus_config_t bus_config =
    {
        .i2c_port = I2C_PORT,

        .sda_io_num = I2C_SDA,

        .scl_io_num = I2C_SCL,

        .clk_source = I2C_CLK_SRC_DEFAULT,

        .glitch_ignore_cnt = 7,

        .flags.enable_internal_pullup = true
    };


    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &i2c_bus
        )
    );


    /*
     * TCA9548A
     */

    i2c_device_config_t tca_config =
    {
        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            TCA9548A_ADDR,

        .scl_speed_hz =
            I2C_FREQUENCY
    };


    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &tca_config,
            &tca_dev
        )
    );
}


/*
 * ============================================================
 * CRIA DEVICE DO MPU
 * ============================================================
 */

static void create_mpu_device(void)
{
    i2c_device_config_t config =
    {
        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            MPU9250_ADDR,

        .scl_speed_hz =
            I2C_FREQUENCY
    };


    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &config,
            &mpu_dev
        )
    );
}


/*
 * ============================================================
 * CRIA DEVICE DO MAG
 * ============================================================
 */

static void create_mag_device(void)
{
    i2c_device_config_t config =
    {
        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            AK8963_ADDR,

        .scl_speed_hz =
            I2C_FREQUENCY
    };


    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            i2c_bus,
            &config,
            &mag_dev
        )
    );
}


/*
 * ============================================================
 * TOF
 *
 * A API pública do componente VL53L1X pode variar conforme
 * a versão. Deixamos a inicialização separada para que os
 * dois sensores possam usar o mesmo barramento através do
 * TCA.
 * ============================================================
 */


/* CABEÇALHO CSV */
static void print_header(void)
{
    printf(
        "R_raw;"
        "G_raw;"
        "B_raw;"

        "IMU1_AX;"
        "IMU1_AY;"
        "IMU1_AZ;"

        "IMU1_GX;"
        "IMU1_GY;"
        "IMU1_GZ;"

        "IMU1_MX;"
        "IMU1_MY;"
        "IMU1_MZ;"

        "IMU2_AX;"
        "IMU2_AY;"
        "IMU2_AZ;"

        "IMU2_GX;"
        "IMU2_GY;"
        "IMU2_GZ;"

        "IMU2_MX;"
        "IMU2_MY;"
        "IMU2_MZ;"

        "TOF1;"
        "TOF2"

        "\n"
    );
}

/* APP MAIN */
void app_main(void)
{
    ESP_LOGI(
        TAG,
        "Inicializando sistema..."
    );

    /* GY-31 */
    color_init();

    /* I2C */
    i2c_init();

    /* Devices I2C */
    create_mpu_device();
    create_mag_device();


    /* * IMU 1 */
    ESP_LOGI(
        TAG,
        "Inicializando IMU 1..."
    );
    ESP_ERROR_CHECK(
        tca_select(TCA_IMU1)
    );
    ESP_ERROR_CHECK(
        mpu9250_init()
    );
    ESP_ERROR_CHECK(
        ak8963_init()
    );


    /* * IMU 2 */

    ESP_LOGI(
        TAG,
        "Inicializando IMU 2..."
    );
    ESP_ERROR_CHECK(
        tca_select(TCA_IMU2)
    );
    ESP_ERROR_CHECK(
        mpu9250_init()
    );
    ESP_ERROR_CHECK(
        ak8963_init()
    );
    tca_disable_all();


    /* * CSV */
    print_header();
    imu_data_t imu1;
    imu_data_t imu2;


    while (1)
    {
        /* * GY-31 */

        uint32_t r =
            read_red();

        uint32_t g =
            read_green();

        uint32_t b =
            read_blue();


        /* IMU 1 */

        tca_select(TCA_IMU1);

        imu_read(
            &imu1
        );


        /* IMU 2 */

        tca_select(TCA_IMU2);

        imu_read(
            &imu2
        );

        int tof1 = -1;
        int tof2 = -1;

        printf(
            "%lu;%lu;%lu;"

            "%d;%d;%d;"
            "%d;%d;%d;"
            "%d;%d;%d;"

            "%d;%d;%d;"
            "%d;%d;%d;"
            "%d;%d;%d;"

            "%d;%d\n",

            (unsigned long)r,
            (unsigned long)g,
            (unsigned long)b,

            imu1.ax,
            imu1.ay,
            imu1.az,

            imu1.gx,
            imu1.gy,
            imu1.gz,

            imu1.mx,
            imu1.my,
            imu1.mz,

            imu2.ax,
            imu2.ay,
            imu2.az,

            imu2.gx,
            imu2.gy,
            imu2.gz,

            imu2.mx,
            imu2.my,
            imu2.mz,

            tof1,
            tof2
        );

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}