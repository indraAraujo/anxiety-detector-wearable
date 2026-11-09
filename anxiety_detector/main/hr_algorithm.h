#ifndef HR_ALGORITHM_H
#define HR_ALGORITHM_H

#include "shared_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ESTRUTURA DO ALGORITMO HR / PPG BUFFER
// ============================================================================

typedef struct {
    // Ring buffer circular contínuo de amostras PPG (IR AC)
    float ring[HR_RING_SAMPLES];
    size_t head;
    size_t count;

    // Estado de detecção do dedo
    int finger_present;

    // Mutex para leitura/escrita thread-safe
    SemaphoreHandle_t mutex;
} hr_algorithm_t;

// ============================================================================
// API PÚBLICA
// ============================================================================

/**
 * @brief Inicializa a estrutura do buffer PPG e mutex
 * @param hr Ponteiro para a estrutura
 * @return ESP_OK se sucesso
 */
esp_err_t hr_algorithm_init(hr_algorithm_t *hr);

/**
 * @brief Libera recursos (mutex)
 * @param hr Ponteiro para a estrutura
 */
void hr_algorithm_deinit(hr_algorithm_t *hr);

/**
 * @brief Adiciona amostra PPG (50Hz) ao Ring Buffer (Thread-safe)
 * @param hr Ponteiro para a estrutura
 * @param ir_ac Componente AC do infravermelho
 * @param finger_present 1 se dedo presente, 0 caso contrário
 * @return ESP_OK se sucesso
 */
esp_err_t hr_algorithm_push_sample(hr_algorithm_t *hr, float ir_ac, int finger_present);

/**
 * @brief Copia o histórico de amostras PPG para um buffer de saída (Thread-safe)
 * @param hr Ponteiro para a estrutura
 * @param out_buffer Array de destino
 * @param max_samples Quantidade de amostras a copiar
 * @return Número de amostras efetivamente copiadas
 */
int hr_algorithm_extract_snapshot(hr_algorithm_t *hr, float *out_buffer, int max_samples);

/**
 * @brief Verifica se o dedo/pulso está no sensor (Thread-safe)
 * @param hr Ponteiro para a estrutura
 * @return 1 se presente, 0 se ausente
 */
int hr_algorithm_is_finger_present(const hr_algorithm_t *hr);

/**
 * @brief Calcula o BPM sob demanda a partir de um snapshot de dados PPG
 * @param window Array contendo as amostras de PPG (ex: últimos 8 a 10 segundos)
 * @param n_samples Quantidade de amostras no window
 * @param sample_rate Taxa de amostragem (ex: 50 Hz)
 * @return BPM calculado (arredondado) ou 0 se sinal for inválido
 */
int hr_algorithm_calculate_bpm_from_snapshot(const float *window, int n_samples, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // HR_ALGORITHM_H