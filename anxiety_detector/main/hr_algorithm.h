#ifndef HR_ALGORITHM_H
#define HR_ALGORITHM_H

#include "shared_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONFIGURAÇÕES DO ALGORITMO DE FC (baseado em esp_health_hr)
// Definidas em shared_config.h - não redefinir aqui
// ============================================================================

// Estados do stream PPG
typedef enum {
    HR_STREAM_SAMPLES     = 0,  // Dados válidos
    HR_STREAM_FINGER_OFF  = 1,  // Dedo removido
    HR_STREAM_FIFO_GAP    = 2,  // Overflow/perda de dados
} hr_stream_kind_t;

// Estado do stream (dedo colocado/removido, gap detectado)
typedef struct {
    int worn;       // 1 = dedo detectado, 0 = dedo removido
    int drop_ring;  // 1 = limpar ring buffer
} hr_stream_state_t;

// Mensagem trocada entre task de dados e task de algoritmo
typedef struct {
    int kind;           // hr_stream_kind_t
    int n;              // Número de amostras válidas (0 se kind != SAMPLES)
    float samples[HR_SAMPLE_RATE];  // 1 segundo de amostras (50 Hz)
} hr_algorithm_msg_t;

// Estado interno do algoritmo
typedef struct {
    // Ring buffer de amostras PPG (IR AC component)
    float ring[HR_RING_SAMPLES];
    int ring_count;
    
    // Estado do stream
    hr_stream_state_t stream_state;
    int was_worn;
    int last_finger_present;  // Último estado visto pela task de aquisição

    // Lote parcial de amostras aguardando envio para a fila
    hr_algorithm_msg_t pending_msg;
    int pending_samples;

    // EMA do BPM
    float hr_ema;
    int miss_streak;      // Segundos consecutivos sem nova detecção válida
    
    // Fila de mensagens
    QueueHandle_t msg_queue;
    
    // Handle do algoritmo DSP (placeholder - usa análise espectral simples)
    // Em produção, usar esp_health_hr_handle_t da ESP-IDF
    void *dsp_handle;
    
    // Output atômico
    int current_bpm;      // BPM atual (arredondado)
    int current_rmssd;    // RMSSD atual (ms)
    
    // Mutex para acesso thread-safe ao BPM/RMSSD
    SemaphoreHandle_t output_mutex;
} hr_algorithm_t;

// ============================================================================
// API PÚBLICA
// ============================================================================

/**
 * @brief Inicializa o algoritmo de frequência cardíaca
 * @param hr Ponteiro para estrutura do algoritmo
 * @param queue_depth Profundidade da fila de mensagens (padrão: HR_RING_SEC)
 * @return ESP_OK se sucesso
 */
esp_err_t hr_algorithm_init(hr_algorithm_t *hr, int queue_depth);

/**
 * @brief Libera recursos do algoritmo
 * @param hr Ponteiro para estrutura do algoritmo
 */
void hr_algorithm_deinit(hr_algorithm_t *hr);

/**
 * @brief Processa nova amostra PPG (chamado pela task de dados a 50Hz)
 * @param hr Ponteiro para estrutura do algoritmo
 * @param ir_ac Componente AC do IR (após filtro DC e LMS)
 * @param finger_present 1 se dedo detectado, 0 caso contrário
 * @return ESP_OK se sucesso
 */
esp_err_t hr_algorithm_push_sample(hr_algorithm_t *hr, float ir_ac, int finger_present);

/**
 * @brief Notifica gap no FIFO (overflow)
 * @param hr Ponteiro para estrutura do algoritmo
 * @return ESP_OK se sucesso
 */
esp_err_t hr_algorithm_notify_fifo_gap(hr_algorithm_t *hr);

/**
 * @brief Task FreeRTOS do algoritmo (roda a 1Hz)
 * @param pvParameters Ponteiro para hr_algorithm_t
 */
void hr_algorithm_task(void *pvParameters);

/**
 * @brief Obtém BPM atual (thread-safe)
 * @param hr Ponteiro para estrutura do algoritmo
 * @return BPM atual (0 se inválido)
 */
int hr_algorithm_get_bpm(const hr_algorithm_t *hr);

/**
 * @brief Obtém RMSSD atual (thread-safe)
 * @param hr Ponteiro para estrutura do algoritmo
 * @return RMSSD em ms (0 se inválido)
 */
int hr_algorithm_get_rmssd(const hr_algorithm_t *hr);

/**
 * @brief Verifica se dedo está presente
 * @param hr Ponteiro para estrutura do algoritmo
 * @return 1 se dedo detectado, 0 caso contrário
 */
int hr_algorithm_is_finger_present(const hr_algorithm_t *hr);

// ============================================================================
// FUNÇÕES AUXILIARES (inline)
// ============================================================================

/**
 * @brief Atualiza estado do stream baseado no tipo de mensagem
 * @param st Estado do stream
 * @param kind Tipo de mensagem
 */
static inline void hr_stream_apply(hr_stream_state_t *st, int kind) {
    if (st == NULL) return;
    if (kind == HR_STREAM_FIFO_GAP) {
        st->drop_ring = 1;
        return;
    }
    if (kind == HR_STREAM_FINGER_OFF) {
        st->worn = 0;
        st->drop_ring = 1;
        return;
    }
    // HR_STREAM_SAMPLES
    st->worn = 1;
    st->drop_ring = 0;
}

/**
 * @brief Calcula alpha do EMA baseado no tamanho da janela
 * @param analyze_n Número de amostras na análise
 * @return Alpha (0.55 para janela < 5s, 0.30 caso contrário)
 */
static inline float hr_ema_alpha_for_window(int analyze_n) {
    if (analyze_n < HR_SAMPLE_RATE * 5) {
        return HR_EMA_ALPHA_FAST;
    }
    return HR_EMA_ALPHA_SLOW;
}

#ifdef __cplusplus
}
#endif

#endif // HR_ALGORITHM_H