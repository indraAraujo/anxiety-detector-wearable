#include "max30102.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "MAX30102";

static bool s_initialized = false;
hr_algorithm_t g_hr_algorithm = {0};

static esp_err_t max30102_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(max30102_dev_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
}

static esp_err_t max30102_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(max30102_dev_handle, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t max30102_read_regs_retry(uint8_t reg, uint8_t *data, size_t len, int retries) {
    esp_err_t ret = ESP_FAIL;

    for (int i = 0; i < retries; i++) {
        ret = max30102_read_regs(reg, data, len);
        if (ret == ESP_OK) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return ret;
}

esp_err_t max30102_init(void) {
    esp_err_t ret = ESP_OK;
    uint8_t part_id = 0;

    // Verifica PART_ID
    ret = i2c_master_transmit_receive(max30102_dev_handle, 
                                       (uint8_t[]){MAX30102_REG_PART_ID}, 1, 
                                       &part_id, 1, 
                                       pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao comunicar com MAX30102 (I2C)");
        return ret;
    }

    ESP_LOGI(TAG, "MAX30102 PART_ID: 0x%02X", part_id);

    // Reset de software
    ret = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha no reset de software");
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    // Configuração do FIFO
    ret = max30102_write_reg(MAX30102_REG_FIFO_CONFIG, MAX30102_FIFO_CONFIG_VAL);
    if (ret != ESP_OK) return ret;

    // Modo SpO2 / Frequência Cardíaca
    ret = max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2_HR);
    if (ret != ESP_OK) return ret;

    // Configuração SpO2
    ret = max30102_write_reg(MAX30102_REG_SPO2_CONFIG, MAX30102_SPO2_CONFIG_VAL);
    if (ret != ESP_OK) return ret;

    // Corrente dos LEDs
    ret = max30102_write_reg(MAX30102_REG_LED1_PA, MAX30102_LED_CURRENT_VAL); // RED
    if (ret != ESP_OK) return ret;
    ret = max30102_write_reg(MAX30102_REG_LED2_PA, MAX30102_LED_CURRENT_VAL); // IR
    if (ret != ESP_OK) return ret;

    // Limpa os ponteiros do FIFO para iniciar em estado conhecido
    ret = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (ret != ESP_OK) return ret;
    ret = max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00);
    if (ret != ESP_OK) return ret;
    ret = max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    if (ret != ESP_OK) return ret;

    s_initialized = true;
    ESP_LOGI(TAG, "MAX30102 inicializado com sucesso! (FIFO a 50Hz efetivo)");
    return ESP_OK;
}

esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir) {
    if (!s_initialized || red == NULL || ir == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[MAX30102_FIFO_SAMPLE_BYTES];

    esp_err_t ret = max30102_read_regs(MAX30102_REG_FIFO_DATA, buffer, sizeof(buffer));

    if (ret == ESP_OK) {
        // Converte os 3 bytes de cada canal (18 bits de resolução)
        *red = ((uint32_t)(buffer[0] & 0x03) << 16) | ((uint32_t)buffer[1] << 8) | (uint32_t)buffer[2];
        *ir  = ((uint32_t)(buffer[3] & 0x03) << 16) | ((uint32_t)buffer[4] << 8) | (uint32_t)buffer[5];
    }

    return ret;
}

// Lê status de interrupção e ponteiros do FIFO em uma única transação.
static esp_err_t max30102_read_fifo_status(uint8_t *intr_status, uint8_t *wr, uint8_t *ovf, uint8_t *rd) {
    uint8_t buf[7];

    esp_err_t ret = max30102_read_regs_retry(MAX30102_REG_INTR_STATUS_1, buf, sizeof(buf), 3);
    if (ret != ESP_OK) {
        return ret;
    }

    *intr_status = buf[0];
    *wr = buf[4] & 0x1F;
    *ovf = buf[5] & 0x1F;
    *rd = buf[6] & 0x1F;

    return ESP_OK;
}

// Lê n_samples amostras do FIFO em uma única transação (6 bytes por amostra).
static esp_err_t max30102_read_fifo_burst(uint8_t *buf, int n_samples) {
    if (n_samples <= 0 || n_samples > MAX30102_FIFO_DEPTH) {
        return ESP_ERR_INVALID_ARG;
    }

    return max30102_read_regs(MAX30102_REG_FIFO_DATA, buf,
                              (size_t)n_samples * MAX30102_FIFO_SAMPLE_BYTES);
}

float max30102_apply_lms_filter(lms_filter_t *f, float max30102_raw_ac, float bmi160_enmo) {
    // 1. Atualiza o histórico de ruído (Acelerômetro)
    for (int i = FILTER_TAP_NUM - 1; i > 0; i--) {
        f->x[i] = f->x[i - 1];
    }
    f->x[0] = bmi160_enmo;

    // 2. Estima o ruído de movimento contido no sinal óptico
    float noise_estimate = 0.0f;
    for (int i = 0; i < FILTER_TAP_NUM; i++) {
        noise_estimate += f->w[i] * f->x[i];
    }

    // 3. Subtrai o ruído do sinal bruto do MAX30102 (Sinal Limpo)
    float clean_signal = max30102_raw_ac - noise_estimate;

    // 4. Atualiza os pesos do filtro com base no erro
    for (int i = 0; i < FILTER_TAP_NUM; i++) {
        f->w[i] += 2.0f * f->mu * clean_signal * f->x[i];
    }

    return clean_signal;
}

esp_err_t max30102_hr_algorithm_init(void) {
    return hr_algorithm_init(&g_hr_algorithm, HR_RING_SEC);
}

void vTaskMAX30102(void *pvParameters) {
    uint8_t fifo_buf[MAX30102_FIFO_DEPTH * MAX30102_FIFO_SAMPLE_BYTES];

    esp_err_t init_ret = ESP_FAIL;
    for (int attempt = 0; attempt < 3; attempt++) {
        init_ret = max30102_init();
        if (init_ret == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "Tentativa %d de inicializar MAX30102 falhou, tentando novamente...", attempt + 1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar MAX30102; task encerrada");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Task MAX30102 iniciada (50Hz data -> HR algorithm)");

    int i2c_fail_count = 0;
    int data_fail_count = 0;

    for (;;) {
        uint8_t intr_status = 0;
        uint8_t wr = 0, ovf = 0, rd = 0;

        esp_err_t err = max30102_read_fifo_status(&intr_status, &wr, &ovf, &rd);
        if (err != ESP_OK) {
            i2c_fail_count++;
            if (i2c_fail_count == 1 || (i2c_fail_count % 50) == 0) {
                ESP_LOGW(TAG, "Falha I2C ao ler status do FIFO: %s (%d falhas consecutivas)",
                         esp_err_to_name(err), i2c_fail_count);
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        i2c_fail_count = 0;

        int available = (wr - rd) & 0x1F;
        int fifo_full = (available == 0) && (intr_status & MAX30102_INTR_STATUS_PPG_RDY);

        // wr == rd pode significar FIFO vazio ou cheio. PPG_RDY desempata.
        if (fifo_full) {
            available = MAX30102_FIFO_DEPTH;
        }

        if (available == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        // Só reinicia a janela quando há backlog real no FIFO.
        // OVF_COUNTER não é usado como critério: em alguns módulos ele é
        // impreciso e ficaria gerando gaps falsos.
        if (fifo_full || available >= 8) {
            ESP_LOGD(TAG, "FIFO backlog (ovf=%u, disponivel=%d), reiniciando janela",
                     (unsigned)ovf, available);
            hr_algorithm_notify_fifo_gap(&g_hr_algorithm);
        }

        err = max30102_read_fifo_burst(fifo_buf, available);
        if (err != ESP_OK) {
            data_fail_count++;
            if (data_fail_count == 1) {
                ESP_LOGW(TAG, "Falha I2C ao ler dados do FIFO: %s", esp_err_to_name(err));
                hr_algorithm_notify_fifo_gap(&g_hr_algorithm);
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        data_fail_count = 0;

        for (int i = 0; i < available; i++) {
            const uint8_t *s = &fifo_buf[i * MAX30102_FIFO_SAMPLE_BYTES];

            uint32_t ir_val = ((uint32_t)(s[3] & 0x03) << 16) |
                              ((uint32_t)s[4] << 8) |
                              (uint32_t)s[5];

            int finger_present = (ir_val > MAX30102_FINGER_THRESHOLD) ? 1 : 0;

            if (finger_present) {
                // 1. Remove nível DC do IR (Componente AC)
                uint32_t ac_ir = dc_filter(ir_val);

                // 2. Aplica filtro LMS: subtrai interferência de movimento
                float ac_ir_limpo = max30102_apply_lms_filter(&s_lms_filter, (float)ac_ir, s_latest_enmo);

                // 3. Envia amostra para algoritmo HR
                hr_algorithm_push_sample(&g_hr_algorithm, ac_ir_limpo, 1);
            } else {
                // Dedo removido: notifica o algoritmo (apenas na transição)
                hr_algorithm_push_sample(&g_hr_algorithm, 0.0f, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

int max30102_get_bpm(void) {
    return hr_algorithm_get_bpm(&g_hr_algorithm);
}

int max30102_get_rmssd(void) {
    return hr_algorithm_get_rmssd(&g_hr_algorithm);
}

int max30102_is_finger_present(void) {
    return hr_algorithm_is_finger_present(&g_hr_algorithm);
}
