#include "hr_algorithm.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

static const char *TAG = "HR_ALGORITHM";

// ============================================================================
// FUNÇÕES INTERNAS
// ============================================================================

// Análise espectral simples para estimar FC (substituto simplificado do esp_health_hr)
// Retorna BPM estimado ou -1 se falhar
static float estimate_hr_from_window(const float *window, int n_samples, int sample_rate) {
    if (n_samples < sample_rate * 2) return -1.0f;  // Mínimo 2 segundos
    
    // 1. Remover média (DC removal)
    float mean = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        mean += window[i];
    }
    mean /= n_samples;
    
    // 2. Janela de Hann para reduzir vazamento espectral
    float *windowed = (float *)malloc(n_samples * sizeof(float));
    if (!windowed) return -1.0f;
    
    for (int i = 0; i < n_samples; i++) {
        float hann = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n_samples - 1)));
        windowed[i] = (window[i] - mean) * hann;
    }
    
    // 3. FFT simples (apenas magnitude) - buscar pico na faixa 0.67-3.33 Hz (40-200 BPM)
    // Usamos DFT direta nas frequências de interesse (mais eficiente que FFT completa para poucas frequências)
    float best_freq = 0.0f;
    float best_mag = 0.0f;
    
    // Testar frequências de 0.67 Hz a 3.33 Hz (40-200 BPM) com passo de ~0.01 Hz
    for (float freq = 0.67f; freq <= 3.33f; freq += 0.01f) {
        float real = 0.0f, imag = 0.0f;
        float omega = 2.0f * M_PI * freq / sample_rate;
        
        for (int i = 0; i < n_samples; i++) {
            float angle = omega * i;
            real += windowed[i] * cosf(angle);
            imag += windowed[i] * sinf(angle);
        }
        
        float mag = sqrtf(real * real + imag * imag);
        if (mag > best_mag) {
            best_mag = mag;
            best_freq = freq;
        }
    }
    
    free(windowed);
    
    if (best_mag < 0.01f) return -1.0f;  // Sinal muito fraco
    
    return best_freq * 60.0f;  // Converter Hz para BPM
}

// Calcula RMSSD a partir de intervalos RR detectados na janela
static int calculate_rmssd_from_window(const float *window, int n_samples, int sample_rate) {
    // Detecção simples de picos para obter intervalos RR
    // Threshold adaptativo baseado na amplitude do sinal
    float max_val = -1e9f, min_val = 1e9f;
    for (int i = 0; i < n_samples; i++) {
        if (window[i] > max_val) max_val = window[i];
        if (window[i] < min_val) min_val = window[i];
    }
    float threshold = min_val + 0.4f * (max_val - min_val);
    
    int peaks[32];
    int n_peaks = 0;
    int min_distance = sample_rate * 60 / 200;  // Mínimo 300ms entre picos (200 BPM)
    
    for (int i = 1; i < n_samples - 1 && n_peaks < 32; i++) {
        if (window[i] > window[i-1] && window[i] > window[i+1] && window[i] > threshold) {
            if (n_peaks == 0 || (i - peaks[n_peaks-1]) >= min_distance) {
                peaks[n_peaks++] = i;
            }
        }
    }
    
    if (n_peaks < 3) return 0;  // Insuficiente para RMSSD
    
    // Calcular RMSSD
    float sum_sq_diff = 0.0f;
    for (int i = 1; i < n_peaks; i++) {
        int rr_ms = (peaks[i] - peaks[i-1]) * 1000 / sample_rate;
        int prev_rr_ms = (peaks[i-1] - (i>=2 ? peaks[i-2] : peaks[0])) * 1000 / sample_rate;
        if (i >= 2) {
            float diff = (float)(rr_ms - prev_rr_ms);
            sum_sq_diff += diff * diff;
        }
    }
    
    if (n_peaks < 3) return 0;
    float rmssd = sqrtf(sum_sq_diff / (n_peaks - 2));
    return (int)lrintf(rmssd);
}

// Empurra amostras para o ring buffer
static int ring_push(float *ring, int *ring_count, const float *src, int n) {
    if (n > HR_RING_SAMPLES) {
        memcpy(ring, src + (n - HR_RING_SAMPLES), HR_RING_SAMPLES * sizeof(float));
        *ring_count = HR_RING_SAMPLES;
        return HR_RING_SAMPLES;
    }
    if (*ring_count + n > HR_RING_SAMPLES) {
        int drop = *ring_count + n - HR_RING_SAMPLES;
        memmove(ring, ring + drop, (*ring_count - drop) * sizeof(float));
        *ring_count -= drop;
    }
    memcpy(ring + *ring_count, src, n * sizeof(float));
    *ring_count += n;
    return *ring_count;
}

// Publica BPM com EMA
static void publish_hr(hr_algorithm_t *hr, float bpm, int analyze_n) {
    float alpha = hr_ema_alpha_for_window(analyze_n);
    if (hr->hr_ema < 0.0f) {
        hr->hr_ema = bpm;
    } else {
        hr->hr_ema = alpha * bpm + (1.0f - alpha) * hr->hr_ema;
    }
    hr->miss_streak = 0;
    
    int bpm_int = (int)lrintf(hr->hr_ema);
    if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        hr->current_bpm = bpm_int;
        xSemaphoreGive(hr->output_mutex);
    }
    
    ESP_LOGI(TAG, "Heart Rate: %d BPM (win=%ds, ema=%.1f)", bpm_int, analyze_n / HR_SAMPLE_RATE, hr->hr_ema);
    printf(">bpm:%d\n", bpm_int);
}

// Mantém último BPM válido (hold)
static void publish_hold(hr_algorithm_t *hr, int analyze_n) {
    hr->miss_streak++;
    if (hr->hr_ema >= HR_BPM_MIN && hr->miss_streak <= HR_HOLD_MAX_SEC) {
        int bpm_int = (int)lrintf(hr->hr_ema);
        if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            hr->current_bpm = bpm_int;
            xSemaphoreGive(hr->output_mutex);
        }
        ESP_LOGI(TAG, "Heart Rate: %d BPM (win=%ds, hold)", bpm_int, analyze_n / HR_SAMPLE_RATE);
    } else {
        if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            hr->current_bpm = 0;
            xSemaphoreGive(hr->output_mutex);
        }
        ESP_LOGI(TAG, "Heart Rate: -- (win=%ds, measuring)", analyze_n / HR_SAMPLE_RATE);
    }
}

// Marca dedo como removido
static void mark_finger_off(hr_algorithm_t *hr) {
    hr->ring_count = 0;
    hr->hr_ema = -1.0f;
    hr->miss_streak = 0;
    hr->was_worn = 0;
    
    if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        hr->current_bpm = 0;
        hr->current_rmssd = 0;
        xSemaphoreGive(hr->output_mutex);
    }
    ESP_LOGI(TAG, "Heart Rate: -- (finger off)");
}

// Limpa fila de mensagens
static void discard_queue(QueueHandle_t q) {
    hr_algorithm_msg_t tmp;
    while (xQueueReceive(q, &tmp, 0) == pdTRUE) {
    }
}

// ============================================================================
// IMPLEMENTAÇÃO DA API
// ============================================================================

esp_err_t hr_algorithm_init(hr_algorithm_t *hr, int queue_depth) {
    if (!hr) return ESP_ERR_INVALID_ARG;
    
    memset(hr, 0, sizeof(hr_algorithm_t));
    
    hr->hr_ema = -1.0f;
    hr->current_bpm = 0;
    hr->current_rmssd = 0;
    
    hr->msg_queue = xQueueCreate(queue_depth > 0 ? queue_depth : HR_RING_SEC, sizeof(hr_algorithm_msg_t));
    if (!hr->msg_queue) {
        ESP_LOGE(TAG, "Falha ao criar fila de mensagens");
        return ESP_ERR_NO_MEM;
    }
    
    hr->output_mutex = xSemaphoreCreateMutex();
    if (!hr->output_mutex) {
        ESP_LOGE(TAG, "Falha ao criar mutex de saída");
        vQueueDelete(hr->msg_queue);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Algoritmo HR inicializado (queue_depth=%d)", queue_depth);
    return ESP_OK;
}

void hr_algorithm_deinit(hr_algorithm_t *hr) {
    if (!hr) return;
    
    if (hr->msg_queue) {
        vQueueDelete(hr->msg_queue);
        hr->msg_queue = NULL;
    }
    if (hr->output_mutex) {
        vSemaphoreDelete(hr->output_mutex);
        hr->output_mutex = NULL;
    }
    if (hr->dsp_handle) {
        free(hr->dsp_handle);
        hr->dsp_handle = NULL;
    }
}

esp_err_t hr_algorithm_push_sample(hr_algorithm_t *hr, float ir_ac, int finger_present) {
    if (!hr) return ESP_ERR_INVALID_ARG;

    if (!finger_present) {
        // Só envia FINGER_OFF na transição para evitar inundar a fila
        if (hr->last_finger_present) {
            if (hr->pending_samples > 0) {
                if (uxQueueMessagesWaiting(hr->msg_queue) >= HR_RING_SEC) {
                    hr_algorithm_msg_t throw_msg;
                    xQueueReceive(hr->msg_queue, &throw_msg, 0);
                }
                xQueueSend(hr->msg_queue, &hr->pending_msg, portMAX_DELAY);
                hr->pending_samples = 0;
                hr->pending_msg.n = 0;
            }

            hr->pending_msg.kind = HR_STREAM_FINGER_OFF;
            hr->pending_msg.n = 0;
            xQueueSend(hr->msg_queue, &hr->pending_msg, portMAX_DELAY);
            hr->last_finger_present = 0;
        }
        return ESP_OK;
    }

    if (!hr->last_finger_present) {
        // Dedo recolocado: descarta lote parcial antigo
        hr->pending_samples = 0;
        hr->pending_msg.n = 0;
        hr->last_finger_present = 1;
    }

    hr->pending_msg.kind = HR_STREAM_SAMPLES;
    hr->pending_msg.samples[hr->pending_samples] = ir_ac;
    hr->pending_samples++;
    hr->pending_msg.n = hr->pending_samples;

    // Envia a cada 1 segundo (50 amostras)
    if (hr->pending_samples >= HR_SAMPLE_RATE) {
        if (uxQueueMessagesWaiting(hr->msg_queue) >= HR_RING_SEC) {
            hr_algorithm_msg_t throw_msg;
            xQueueReceive(hr->msg_queue, &throw_msg, 0);
        }
        xQueueSend(hr->msg_queue, &hr->pending_msg, portMAX_DELAY);
        hr->pending_samples = 0;
        hr->pending_msg.n = 0;
    }

    return ESP_OK;
}

esp_err_t hr_algorithm_notify_fifo_gap(hr_algorithm_t *hr) {
    if (!hr) return ESP_ERR_INVALID_ARG;

    // Descarta dados antigos e o lote parcial para não misturar
    // amostras de antes e depois do gap na mesma janela.
    discard_queue(hr->msg_queue);
    hr->pending_samples = 0;
    hr->pending_msg.n = 0;

    hr_algorithm_msg_t msg = { .kind = HR_STREAM_FIFO_GAP, .n = 0 };
    xQueueSend(hr->msg_queue, &msg, portMAX_DELAY);
    return ESP_OK;
}

void hr_algorithm_task(void *pvParameters) {
    hr_algorithm_t *hr = (hr_algorithm_t *)pvParameters;
    if (!hr) {
        ESP_LOGE(TAG, "Task HR: parâmetro inválido");
        vTaskDelete(NULL);
        return;
    }
    
    hr_algorithm_msg_t *msg = (hr_algorithm_msg_t *)malloc(sizeof(hr_algorithm_msg_t));
    if (!msg) {
        ESP_LOGE(TAG, "Task HR: falha ao alocar msg");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Task de algoritmo HR iniciada (update=%ds)", HR_UPDATE_SEC);
    
    while (1) {
        int new_batches = 0;
        int saw_gap = 0;
        hr_stream_state_t st = {
            .worn = hr->was_worn,
            .drop_ring = 0,
        };
        
        // Processa todas as mensagens na fila
        while (xQueueReceive(hr->msg_queue, msg, 0) == pdTRUE) {
            hr_stream_apply(&st, msg->kind);
            
            if (st.drop_ring) {
                hr->ring_count = 0;
            }
            
            if (msg->kind == HR_STREAM_FIFO_GAP) {
                hr->hr_ema = -1.0f;
                hr->miss_streak = 0;
                if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    hr->current_bpm = 0;
                    xSemaphoreGive(hr->output_mutex);
                }
                saw_gap = 1;
                continue;
            }
            
            if (msg->kind == HR_STREAM_FINGER_OFF) {
                mark_finger_off(hr);
                continue;
            }
            
            // HR_STREAM_SAMPLES
            if (!hr->was_worn) {
                hr->hr_ema = -1.0f;
                hr->miss_streak = 0;
                hr->was_worn = 1;
                ESP_LOGI(TAG, "Heart Rate: -- (measuring)");
            }
            
            ring_push(hr->ring, &hr->ring_count, msg->samples, msg->n);
            new_batches++;
        }
        
        if (saw_gap) {
            ESP_LOGW(TAG, "Gap no stream PPG detectado, janela reiniciada");
        }
        
        if (!hr->was_worn) {
            vTaskDelay(pdMS_TO_TICKS(HR_UPDATE_SEC * 1000));
            continue;
        }
        
        // Sem novos dados neste tick - hold
        if (new_batches == 0) {
            if (hr->ring_count >= HR_MIN_RING_SAMPLES) {
                int win_sec = hr->ring_count / HR_SAMPLE_RATE;
                if (win_sec > HR_ANALYSIS_SEC) win_sec = HR_ANALYSIS_SEC;
                if (win_sec < 1) win_sec = 1;
                publish_hold(hr, win_sec * HR_SAMPLE_RATE);
            }
            vTaskDelay(pdMS_TO_TICKS(HR_UPDATE_SEC * 1000));
            continue;
        }
        
        // Verifica se tem amostras suficientes para análise
        if (hr->ring_count < HR_MIN_PUBLISH_SAMPLES) {
            ESP_LOGI(TAG, "Heart Rate: -- (win=%ds, measuring)", hr->ring_count / HR_SAMPLE_RATE);
            vTaskDelay(pdMS_TO_TICKS(HR_UPDATE_SEC * 1000));
            continue;
        }
        
        // Análise da janela
        int analyze_n = hr->ring_count;
        if (analyze_n > HR_ANALYSIS_SAMPLES) {
            analyze_n = HR_ANALYSIS_SAMPLES;
        }
        const float *window = hr->ring + (hr->ring_count - analyze_n);
        
        // Estima FC via análise espectral
        float bpm = estimate_hr_from_window(window, analyze_n, HR_SAMPLE_RATE);
        
        if (bpm >= HR_BPM_MIN && bpm <= HR_BPM_MAX) {
            publish_hr(hr, bpm, analyze_n);
            
            // Calcula RMSSD
            int rmssd = calculate_rmssd_from_window(window, analyze_n, HR_SAMPLE_RATE);
            if (rmssd > 0) {
                if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    hr->current_rmssd = rmssd;
                    xSemaphoreGive(hr->output_mutex);
                }
                ESP_LOGI(TAG, "RMSSD: %d ms", rmssd);
            printf(">rmssd:%d\n", rmssd);
            }
        } else {
            publish_hold(hr, analyze_n);
        }
        
        vTaskDelay(pdMS_TO_TICKS(HR_UPDATE_SEC * 1000));
    }
}

int hr_algorithm_get_bpm(const hr_algorithm_t *hr) {
    if (!hr) return 0;
    int bpm = 0;
    if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        bpm = hr->current_bpm;
        xSemaphoreGive(hr->output_mutex);
    }
    return bpm;
}

int hr_algorithm_get_rmssd(const hr_algorithm_t *hr) {
    if (!hr) return 0;
    int rmssd = 0;
    if (xSemaphoreTake(hr->output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        rmssd = hr->current_rmssd;
        xSemaphoreGive(hr->output_mutex);
    }
    return rmssd;
}

int hr_algorithm_is_finger_present(const hr_algorithm_t *hr) {
    if (!hr) return 0;
    return hr->was_worn;
}