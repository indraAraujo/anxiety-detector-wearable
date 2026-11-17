#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "shared_config.h"
#include "accelerometer.h"
#include "max30102.h"
#include "hr_algorithm.h"
#include "mqtt_publisher.h"
#include "nvs_flash.h"
#include "inference_task.h"
#include "fcm_notifier.h"

static const char *TAG = "ANXIETY_DETECTOR";

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Anxiety Detector...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t ret_hr_algorithm = hr_algorithm_init(&g_hr_algorithm);
    if (ret_hr_algorithm != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar algoritmo de BPM");
        return;
    }

    init_wifi();
    init_mqtt();

    fcm_notifier_init();

    ESP_ERROR_CHECK(i2c_master_init());

    ESP_ERROR_CHECK(ring_buf_init(&s_buf_imu));

    ESP_ERROR_CHECK(accelerometer_init());

    ESP_ERROR_CHECK(max30102_hr_algorithm_init());

    xTaskCreatePinnedToCore(
        vTaskAccelerometer,
        "TaskAccel",
        4096,
        NULL,
        5,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        vTaskMAX30102,
        "TaskMAX30102",
        4096,
        NULL,
        5,
        NULL,
        0
    );

    ESP_ERROR_CHECK(inference_task_init());

    ESP_LOGI(TAG, "Todas as tasks criadas. Sistema rodando.");
}