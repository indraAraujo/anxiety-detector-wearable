#include "inference_task.h"
#include "model_wesad.h"
#include "shared_config.h"
#include "hr_algorithm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_publisher.h"
#include <math.h>
#include "esp_timer.h"
#include "esp_http_client.h"
#include "fcm_notifier.h"

static const char *TAG = "INFERENCE_TASK";

// Instância do algoritmo HR declarada em max30102.c
extern hr_algorithm_t g_hr_algorithm;

static int s_latest_stress_state = 0; // 0: Repouso, 1: Estresse

// Windowing: Janela de 60 segundos a 50Hz = 3000 amostras
#define WINDOW_SAMPLES_IMU (60 * 50)
#define WINDOW_SAMPLES_PPG HR_RING_SAMPLES // 3000 amostras (defined em shared_config.h)

static scalar_sample_t s_imu_snapshot[WINDOW_SAMPLES_IMU];
static float s_ppg_snapshot[WINDOW_SAMPLES_PPG];


static void vTaskInference(void *pvParameters) {
    ESP_LOGI(TAG, "Task de Inferência iniciada (execução a cada 5 segundos)");

    // Aguarda o sistema acumular amostras iniciais antes da 1ª predição
    vTaskDelay(pdMS_TO_TICKS(10000));

    while (1) {
        // 1. Verifica se o usuário está vestindo o sensor
        if (!hr_algorithm_is_finger_present(&g_hr_algorithm)) {
            ESP_LOGD(TAG, "Dedo ausente. Pulando inferência.");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // 2. Extrai snapshot do Ring Buffer do Acelerômetro
        ring_buf_extract_snapshot(&s_buf_imu, WINDOW_SAMPLES_IMU, s_imu_snapshot);

        // 3. Extrai snapshot seguro do Ring Buffer de PPG (Thread-safe via Mutex)
        int ppg_samples_copied = hr_algorithm_extract_snapshot(&g_hr_algorithm, s_ppg_snapshot, WINDOW_SAMPLES_PPG);
        
        if (ppg_samples_copied < 250) { // Exige ao menos 5 segundos de dados válidos (250 amostras a 50Hz)
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // 4. Extrai estatísticas (Features) para o modelo WESAD
        // Features: [ppg_mean, ppg_std, acc_mean, acc_std]
        
        // --- PPG Features ---
        double ppg_sum = 0.0;
        for (int i = 0; i < ppg_samples_copied; i++) {
            ppg_sum += s_ppg_snapshot[i];
        }
        float ppg_mean = (float)(ppg_sum / ppg_samples_copied);

        double ppg_sq_diff_sum = 0.0;
        for (int i = 0; i < ppg_samples_copied; i++) {
            float diff = s_ppg_snapshot[i] - ppg_mean;
            ppg_sq_diff_sum += diff * diff;
        }
        float ppg_std = sqrtf((float)(ppg_sq_diff_sum / ppg_samples_copied));

        // --- ACC Features ---
        double acc_sum = 0.0;
        for (int i = 0; i < WINDOW_SAMPLES_IMU; i++) {
            acc_sum += s_imu_snapshot[i].value;
        }
        float acc_mean = (float)(acc_sum / WINDOW_SAMPLES_IMU);

        double acc_sq_diff_sum = 0.0;
        for (int i = 0; i < WINDOW_SAMPLES_IMU; i++) {
            float diff = s_imu_snapshot[i].value - acc_mean;
            acc_sq_diff_sum += diff * diff;
        }
        float acc_std = sqrtf((float)(acc_sq_diff_sum / WINDOW_SAMPLES_IMU));

        // 5. Monta o vetor de entrada e realiza a inferência do Modelo SVM
        double features[4] = { ppg_mean, ppg_std, acc_mean, acc_std };
        int predicted_state = predict_wesad_stress(features);

        s_latest_stress_state = predicted_state;

        // 6. CÁLCULO SOB DEMANDA DO BPM NO MOMENTO DA INFERÊNCIA
        // Usa os últimos 8 segundos de amostras (400 amostras a 50Hz) do snapshot para maior precisão de FC
        int bpm_momento = 0;
        int dft_window_size = (ppg_samples_copied >= 400) ? 400 : ppg_samples_copied;
        float *dft_window_start = &s_ppg_snapshot[ppg_samples_copied - dft_window_size];
        
        bpm_momento = hr_algorithm_calculate_bpm_from_snapshot(dft_window_start, dft_window_size, HR_SAMPLE_RATE);

        // 7. Log e Publicação dos Resultados
        if (predicted_state == 1) {
            // Envia notificação push Firebase ao detectar ansiedade/estresse
            fcm_notifier_send_alert(
                "Opa, ansiedade detectada!",
                "Entre em contato ou vá até o usuário da pulseira em monitoramento para dar assistência médica"
            );

            int64_t t_tx_start = esp_timer_get_time();
            
            // Publica o BPM REAL medido no instante da inferência
            publish_heart_rate((float)bpm_momento, "111111");

            int64_t t_tx_end = esp_timer_get_time();
            float tx_duration_ms = (float)(t_tx_end - t_tx_start) / 1000.0f;
            printf("TEMPO DE PUBLICAÇÃO: %f", tx_duration_ms);
            
            ESP_LOGW(TAG, "ESTRESSE DETECTADO | BPM: %d | PPG (std: %.2f) | ACC (std: %.2f)",
                bpm_momento, ppg_std, acc_std);
        } else {
            ESP_LOGI(TAG, "Estado Normal/Repouso | BPM: %d | PPG (std: %.2f) | ACC (std: %.2f)",
                     bpm_momento, ppg_std, acc_std);
        }

        // Intervalo de inferência deslizante (a cada 5 segundos)
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

esp_err_t inference_task_init(void) {
    BaseType_t ret = xTaskCreatePinnedToCore(
        vTaskInference,
        "TaskInference",
        4096,
        NULL,
        3, // Prioridade menor que a amostragem dos sensores
        NULL,
        1  // Executa no Core 1
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

int inference_get_latest_state(void) {
    return s_latest_stress_state;
}