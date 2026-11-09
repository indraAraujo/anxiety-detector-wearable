#include "hr_algorithm.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

static const char *TAG = "HR_ALGORITHM";

// ============================================================================
// FUNÇÃO INTERNA: FILTRO PASSA-BANDA (DoMA) + DFT PARA CÁLCULO DE BPM
// ============================================================================

static float estimate_hr_from_window(const float *window, float *windowed_buf, int n_samples, int sample_rate) {
    if (n_samples < sample_rate * 2) return -1.0f;  // Mínimo 2 segundos
    
    // 1. Filtro Passa-Banda (Diferença de Médias Móveis - DoMA)
    for (int i = 0; i < n_samples; i++) {
        // Passa-Baixas: Média curta de 5 amostras (0.1s) - mata ruídos de alta frequência
        float signal_val = 0.0f;
        int count_lp = 0;
        for (int j = i - 2; j <= i + 2; j++) {
            if (j >= 0 && j < n_samples) {
                signal_val += window[j];
                count_lp++;
            }
        }
        signal_val /= count_lp;

        // Passa-Altas: Média longa de 51 amostras (~1.0s em 50Hz) - isola linha de base da respiração
        float baseline_val = 0.0f;
        int count_hp = 0;
        for (int j = i - 25; j <= i + 25; j++) {
            if (j >= 0 && j < n_samples) {
                baseline_val += window[j];
                count_hp++;
            }
        }
        baseline_val /= count_hp;

        windowed_buf[i] = signal_val - baseline_val;
    }
    
    // 2. Janela de Hann
    for (int i = 0; i < n_samples; i++) {
        float hann = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n_samples - 1)));
        windowed_buf[i] *= hann;
    }
    
    // 3. DFT Otimizada (40 a 180 BPM)
    float best_freq = 0.0f;
    float best_mag = 0.0f;
    
    for (float freq = 0.67f; freq <= 3.00f; freq += 0.01f) { 
        float real = 0.0f, imag = 0.0f;
        float omega = 2.0f * M_PI * freq / sample_rate;
        
        float cos_w = cosf(omega);
        float sin_w = sinf(omega);
        
        float cos_nw = 1.0f;
        float sin_nw = 0.0f;
        
        for (int i = 0; i < n_samples; i++) {
            real += windowed_buf[i] * cos_nw;
            imag += windowed_buf[i] * sin_nw;
            
            float next_cos = cos_nw * cos_w - sin_nw * sin_w;
            float next_sin = sin_nw * cos_w + cos_nw * sin_w;
            cos_nw = next_cos;
            sin_nw = next_sin;
        }
        
        float mag = sqrtf(real * real + imag * imag);
        if (mag > best_mag) {
            best_mag = mag;
            best_freq = freq;
        }
    }
    
    if (best_mag < 0.01f) return -1.0f;  // Sinal fraco
    
    return best_freq * 60.0f;  // Retorna BPM em float
}

// ============================================================================
// IMPLEMENTAÇÃO DA API PÚBLICA
// ============================================================================

esp_err_t hr_algorithm_init(hr_algorithm_t *hr) {
    if (!hr) return ESP_ERR_INVALID_ARG;
    
    memset(hr, 0, sizeof(hr_algorithm_t));
    
    hr->mutex = xSemaphoreCreateMutex();
    if (!hr->mutex) {
        ESP_LOGE(TAG, "Falha ao criar mutex do HR Algorithm");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Módulo de Ring Buffer PPG inicializado");
    return ESP_OK;
}

void hr_algorithm_deinit(hr_algorithm_t *hr) {
    if (!hr) return;
    if (hr->mutex) {
        vSemaphoreDelete(hr->mutex);
        hr->mutex = NULL;
    }
}

esp_err_t hr_algorithm_push_sample(hr_algorithm_t *hr, float ir_ac, int finger_present) {
    if (!hr) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(hr->mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        hr->finger_present = finger_present;

        if (finger_present) {
            hr->ring[hr->head] = ir_ac;
            hr->head = (hr->head + 1) % HR_RING_SAMPLES;
            if (hr->count < HR_RING_SAMPLES) {
                hr->count++;
            }
        } else {
            // Reseta contagem do buffer se o dedo for removido
            hr->count = 0;
            hr->head = 0;
        }
        xSemaphoreGive(hr->mutex);
    }

    return ESP_OK;
}

int hr_algorithm_extract_snapshot(hr_algorithm_t *hr, float *out_buffer, int max_samples) {
    if (!hr || !out_buffer || max_samples <= 0) return 0;

    int copied = 0;
    if (xSemaphoreTake(hr->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        int available = (int)hr->count;
        copied = (available > max_samples) ? max_samples : available;

        for (int i = 0; i < copied; i++) {
            size_t idx = (hr->head + HR_RING_SAMPLES - copied + i) % HR_RING_SAMPLES;
            out_buffer[i] = hr->ring[idx];
        }

        xSemaphoreGive(hr->mutex);
    }
    return copied;
}

int hr_algorithm_is_finger_present(const hr_algorithm_t *hr) {
    if (!hr) return 0;
    int present = 0;
    if (xSemaphoreTake(((hr_algorithm_t *)hr)->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        present = hr->finger_present;
        xSemaphoreGive(((hr_algorithm_t *)hr)->mutex);
    }
    return present;
}

int hr_algorithm_calculate_bpm_from_snapshot(const float *window, int n_samples, int sample_rate) {
    if (!window || n_samples < sample_rate * 2) return 0;

    // Buffer dinâmico de curta duração na stack ou heap temporário
    float *dsp_windowed_buf = (float *)malloc(n_samples * sizeof(float));
    if (!dsp_windowed_buf) {
        ESP_LOGE(TAG, "Falha de memória na inferência de BPM");
        return 0;
    }

    float bpm_float = estimate_hr_from_window(window, dsp_windowed_buf, n_samples, sample_rate);
    free(dsp_windowed_buf);

    if (bpm_float >= HR_BPM_MIN && bpm_float <= HR_BPM_MAX) {
        return (int)lrintf(bpm_float);
    }

    return 0; // Sinal fora dos limites ou inválido
}