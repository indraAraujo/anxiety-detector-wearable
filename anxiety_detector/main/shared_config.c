#include "shared_config.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SHARED_CONFIG";

// Variáveis globais
i2c_master_bus_handle_t i2c_bus_handle_0 = NULL;  // MAX30102 (GPIO 4/5)
i2c_master_bus_handle_t i2c_bus_handle_1 = NULL;  // BMI160 (GPIO 8/9)
i2c_master_dev_handle_t max30102_dev_handle = NULL;
i2c_master_dev_handle_t bmi160_dev_handle = NULL;

lms_filter_t s_lms_filter = { .mu = 0.005f };
hrv_calculator_t s_hrv_data = {0};
float s_latest_rmssd = 0.0f;
float s_latest_enmo = 0.0f;
scalar_ring_buf_t s_buf_imu = {0};

esp_err_t ring_buf_init(scalar_ring_buf_t *rb) {
    if (rb == NULL) return ESP_ERR_INVALID_ARG;
    
    rb->head = 0;
    rb->count = 0;
    memset(rb->data, 0, sizeof(rb->data));
    
    rb->mutex = xSemaphoreCreateMutex();
    if (rb->mutex == NULL) {
        ESP_LOGE(TAG, "Falha ao criar mutex do ring buffer");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

void ring_buf_push(scalar_ring_buf_t *rb, scalar_sample_t sample) {
    if (rb == NULL || rb->mutex == NULL) return;
    
    xSemaphoreTake(rb->mutex, portMAX_DELAY);
    rb->data[rb->head] = sample;
    rb->head = (rb->head + 1) % RING_BUF_CAPACITY;
    if (rb->count < RING_BUF_CAPACITY) {
        rb->count++;
    }
    xSemaphoreGive(rb->mutex);
}

void ring_buf_extract_snapshot(scalar_ring_buf_t *rb, 
                                size_t total_samples, 
                                scalar_sample_t *out_array) {
    if (rb == NULL || rb->mutex == NULL || out_array == NULL) return;
    if (total_samples == 0 || total_samples > RING_BUF_CAPACITY) return;
    
    xSemaphoreTake(rb->mutex, portMAX_DELAY);
    
    size_t start_idx;
    if (rb->head >= total_samples) {
        start_idx = rb->head - total_samples;
    } else {
        start_idx = RING_BUF_CAPACITY + rb->head - total_samples;
    }

    for (size_t i = 0; i < total_samples; i++) {
        out_array[i] = rb->data[(start_idx + i) % RING_BUF_CAPACITY];
    }

    xSemaphoreGive(rb->mutex);
}

esp_err_t i2c_master_init(void) {
    esp_err_t err = ESP_OK;

    // ============================================================
    // Barramento 0: MAX30102 (GPIO 4/5, I2C_NUM_0) - 100kHz, pull-ups externos
    // ============================================================
    i2c_master_bus_config_t bus_config_0 = {
        .i2c_port = I2C_MASTER_0_PORT,
        .sda_io_num = I2C_MASTER_0_SDA_IO,
        .scl_io_num = I2C_MASTER_0_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // Pull-ups externos na placa
    };

    err = i2c_new_master_bus(&bus_config_0, &i2c_bus_handle_0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar barramento I2C 0 (MAX30102): %s", esp_err_to_name(err));
        return err;
    }

    // Aguarda estabilização do sensor após power-on
    vTaskDelay(pdMS_TO_TICKS(100));

    i2c_device_config_t max30102_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_0_FREQ_HZ,
    };

    err = i2c_master_bus_add_device(i2c_bus_handle_0, &max30102_dev_config, &max30102_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar MAX30102: %s", esp_err_to_name(err));
        return err;
    }

    // ============================================================
    // Barramento 1: BMI160 (GPIO 8/9, I2C_NUM_1)
    // ============================================================
    i2c_master_bus_config_t bus_config_1 = {
        .i2c_port = I2C_MASTER_1_PORT,
        .sda_io_num = I2C_MASTER_1_SDA_IO,
        .scl_io_num = I2C_MASTER_1_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&bus_config_1, &i2c_bus_handle_1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar barramento I2C 1 (BMI160): %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t bmi160_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMI160_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_1_FREQ_HZ,
    };

    err = i2c_master_bus_add_device(i2c_bus_handle_1, &bmi160_dev_config, &bmi160_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar BMI160: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Dois barramentos I2C e dispositivos inicializados com sucesso");
    return ESP_OK;
}

uint32_t dc_filter(uint32_t sample) {
    static uint32_t dc_level = 0;
    if (dc_level == 0) dc_level = sample << 4;
    
    dc_level = dc_level - (dc_level >> 7) + sample;
    uint32_t current_dc = dc_level >> 7;
    
    return (sample > current_dc) ? (sample - current_dc) : 0;
}

float low_pass_filter(float input) {
    static float buf[4] = {0};
    buf[3] = buf[2];
    buf[2] = buf[1];
    buf[1] = buf[0];
    buf[0] = input;
    
    // Média móvel de 4 amostras
    return (buf[0] + buf[1] + buf[2] + buf[3]) / 4.0f;
}

// Função para calcular RMSSD (movida para shared pois é usada pelo MAX30102)
float calcular_rmssd(hrv_calculator_t *hrv, int32_t novo_ibi_ms) {
    // 1. Armazena o novo IBI no buffer circular
    hrv->ibi_buffer[hrv->head] = novo_ibi_ms;
    hrv->head = (hrv->head + 1) % HRV_BUFFER_SIZE;
    if (hrv->count < HRV_BUFFER_SIZE) {
        hrv->count++;
    }

    // Só calcula quando acumular pelo menos 10 batimentos
    if (hrv->count < 10) {
        return 0.0f; 
    }

    // 2. Soma dos quadrados das diferenças sucessivas
    float sum_squared_diffs = 0.0f;
    size_t num_pairs = hrv->count - 1;

    for (size_t i = 0; i < num_pairs; i++) {
        size_t idx1 = (hrv->head + HRV_BUFFER_SIZE - hrv->count + i) % HRV_BUFFER_SIZE;
        size_t idx2 = (idx1 + 1) % HRV_BUFFER_SIZE;

        float diff = (float)(hrv->ibi_buffer[idx2] - hrv->ibi_buffer[idx1]);
        sum_squared_diffs += (diff * diff);
    }

    // 3. Média e Raiz Quadrada
    float mean_square = sum_squared_diffs / (float)num_pairs;
    return sqrtf(mean_square);
}