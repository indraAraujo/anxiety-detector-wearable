#include "accelerometer.h"
#include "esp_log.h"
#include <math.h>
#include "esp_timer.h"

static const char *TAG = "ACCELEROMETER";

static bool s_initialized = false;

esp_err_t accelerometer_init(void) {
    esp_err_t ret = ESP_OK;
    uint8_t chip_id = 0;

    // Verifica se o dispositivo responde (lê CHIP_ID)
    ret = i2c_master_transmit_receive(bmi160_dev_handle, 
                                       (uint8_t[]){0x00}, 1, 
                                       &chip_id, 1, 
                                       pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao comunicar com BMI160 (I2C)");
        return ret;
    }

    if (chip_id != BMI160_CHIP_ID) {
        ESP_LOGE(TAG, "Chip ID inválido: 0x%02X (esperado 0x%02X)", chip_id, BMI160_CHIP_ID);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "BMI160 detectado (Chip ID: 0x%02X)", chip_id);

    // Configura range do acelerômetro para +/- 2g
    uint8_t acc_range_cmd[2] = {BMI160_REG_ACC_RANGE, BMI160_ACC_RANGE_2G};
    ret = i2c_master_transmit(bmi160_dev_handle, acc_range_cmd, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar range do acelerômetro");
        return ret;
    }

    // Configura ODR (Output Data Rate) para 100Hz, modo normal
    uint8_t acc_conf_cmd[2] = {BMI160_REG_ACC_CONF, BMI160_ACC_ODR_100HZ};
    ret = i2c_master_transmit(bmi160_dev_handle, acc_conf_cmd, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao configurar ODR do acelerômetro");
        return ret;
    }

    // Coloca acelerômetro em modo normal
    uint8_t cmd_normal[2] = {BMI160_REG_CMD, BMI160_ACC_NORMAL_MODE};
    ret = i2c_master_transmit(bmi160_dev_handle, cmd_normal, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao ativar modo normal do acelerômetro");
        return ret;
    }

    // Aguarda estabilização
    vTaskDelay(pdMS_TO_TICKS(10));

    s_initialized = true;
    ESP_LOGI(TAG, "Acelerômetro BMI160 inicializado com sucesso!");
    return ESP_OK;
}

bool accelerometer_read_enmo(float *out_mag_g) {
    if (!s_initialized || out_mag_g == NULL) {
        return false;
    }

    uint8_t reg = BMI160_REG_ACC_X_LSB;
    uint8_t raw_buf[6];

    // Leitura em burst: Accel X (H/L), Y (H/L), Z (H/L)
    esp_err_t ret = i2c_master_transmit_receive(bmi160_dev_handle, &reg, 1, raw_buf, 6, pdMS_TO_TICKS(50));
    if (ret != ESP_OK) {
        return false;
    }

    // Reconstrói inteiros de 16 bits com sinal
    int16_t raw_x = (int16_t)((raw_buf[1] << 8) | raw_buf[0]);
    int16_t raw_y = (int16_t)((raw_buf[3] << 8) | raw_buf[2]);
    int16_t raw_z = (int16_t)((raw_buf[5] << 8) | raw_buf[4]);

    // Converte para aceleração física (g)
    // Fator de escala para range +/- 2g = 16384 LSB/g
    float ax = (float)raw_x / BMI160_SCALE_FACTOR_2G;
    float ay = (float)raw_y / BMI160_SCALE_FACTOR_2G;
    float az = (float)raw_z / BMI160_SCALE_FACTOR_2G;

    // Calcula magnitude (ENMO - Euclidean Norm Minus One)
    *out_mag_g = sqrtf(ax * ax + ay * ay + az * az);
    
    // Subtrai 1g para isolar movimento linear, valor absoluto
    *out_mag_g = fabsf(*out_mag_g - 1.0f);

    return true;
}

void vTaskAccelerometer(void *pvParameters) {
    float mag_samples[IMU_SUB_SAMPLES];

    while (1) {
        TickType_t start_tick = xTaskGetTickCount();

        // Coleta 5 sub-amostras na janela de 20ms (intervalo de 4ms)
        float enmo_abs_sum = 0.0f;
        for (int i = 0; i < IMU_SUB_SAMPLES; i++) {
            if (!accelerometer_read_enmo(&mag_samples[i])) {
                mag_samples[i] = 0.0f; // Fallback
            }
            enmo_abs_sum += mag_samples[i];

            // Atraso de 4ms ENTRE amostras (4 atrasos para 5 amostras)
            if (i < IMU_SUB_SAMPLES - 1) {
                vTaskDelay(pdMS_TO_TICKS(4));
            }
        }

        // Calcula média do ENMO na janela de 20ms
        float enmo_abs_mean = enmo_abs_sum / (float)IMU_SUB_SAMPLES;

        // Push para Ring Buffer do IMU a 50 Hz
        scalar_sample_t sample = {
            .timestamp_ms = esp_timer_get_time() / 1000,
            .value = enmo_abs_mean
        };

        s_latest_enmo = enmo_abs_mean;
        ring_buf_push(&s_buf_imu, sample);

        // Mantém período estrito de 50 Hz (20 ms)
        vTaskDelayUntil(&start_tick, pdMS_TO_TICKS(20));
    }
}

float accelerometer_get_latest_enmo(void) {
    return s_latest_enmo;
}