#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "shared_config.h"
#include "accelerometer.h"
#include "max30102.h"
#include "hr_algorithm.h"
#include "mqtt_publisher.h"
#include "nvs_flash.h"

static const char *TAG = "ANXIETY_DETECTOR";

// Declaração externa da instância global do algoritmo HR
extern hr_algorithm_t g_hr_algorithm;

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Anxiety Detector...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_wifi();
    init_mqtt();

    // 1. Inicializa I2C e dispositivos
    ESP_ERROR_CHECK(i2c_master_init());

    // 2. Inicializa ring buffer do IMU
    ESP_ERROR_CHECK(ring_buf_init(&s_buf_imu));

    // 3. Inicializa acelerômetro
    ESP_ERROR_CHECK(accelerometer_init());

    // 4. Inicializa algoritmo HR (cria fila e mutex ANTES de criar a task)
    ESP_ERROR_CHECK(max30102_hr_algorithm_init());

    // 5. Cria Task do Acelerômetro no CORE 0 (50Hz)
    xTaskCreatePinnedToCore(
        vTaskAccelerometer,
        "TaskAccel",
        4096,
        NULL,
        5,
        NULL,
        0
    );

    // 6. Cria Task do MAX30102 no CORE 0 (50Hz - coleta dados e alimenta algoritmo)
    xTaskCreatePinnedToCore(
        vTaskMAX30102,
        "TaskMAX30102",
        4096,
        NULL,
        5,
        NULL,
        0
    );

    // 7. Cria Task do Algoritmo HR no CORE 1 (1Hz - processa janelas espectrais)
    xTaskCreatePinnedToCore(
        hr_algorithm_task,
        "TaskHRAlgo",
        8192,  // Stack maior para FFT/DFT
        &g_hr_algorithm,
        4,     // Prioridade ligeiramente menor
        NULL,
        1
    );

    ESP_LOGI(TAG, "Todas as tasks criadas. Sistema rodando.");
}