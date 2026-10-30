#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONFIGURAÇÕES DO BARRAMENTO I2C (DOIS BARRAMENTOS SEPARADOS)
// ============================================================================

// Barramento 0: MAX30102 (GPIO 4/5) - 100kHz, pull-ups externos
#define I2C_MASTER_0_SDA_IO     4
#define I2C_MASTER_0_SCL_IO     5
#define I2C_MASTER_0_FREQ_HZ    100000
#define I2C_MASTER_0_PORT       I2C_NUM_0

// Barramento 1: BMI160 (GPIO 8/9)
#define I2C_MASTER_1_SDA_IO     8
#define I2C_MASTER_1_SCL_IO     9
#define I2C_MASTER_1_FREQ_HZ    400000
#define I2C_MASTER_1_PORT       I2C_NUM_1

// Endereços I2C dos Sensores
#define MAX30102_I2C_ADDR       0x57
#define BMI160_I2C_ADDR         0x68

// Configurações do Ring Buffer
#define RING_BUF_CAPACITY     100       // 100 samples = 2.0 seconds of history @ 50Hz
#define HRV_BUFFER_SIZE       30

// Configurações do Acelerômetro
#define IMU_SUB_SAMPLES       5    

// Configurações de BPM
#define MIN_BPM               40
#define MAX_BPM               200
#define MIN_DELTA_MS          (60000 / MAX_BPM) 
#define MAX_DELTA_MS          (60000 / MIN_BPM)

// Configurações do Filtro LMS
#define FILTER_TAP_NUM        8

// Configurações do Algoritmo HR (50Hz para sincronizar com IMU)
#define HR_SAMPLE_RATE        50
#define HR_MIN_WINDOW_SEC     4
#define HR_ANALYSIS_SEC       8
#define HR_UPDATE_SEC         1
#define HR_RING_SEC           15
#define HR_RING_SAMPLES       (HR_RING_SEC * HR_SAMPLE_RATE)
#define HR_ANALYSIS_SAMPLES   (HR_ANALYSIS_SEC * HR_SAMPLE_RATE)
#define HR_MIN_RING_SAMPLES   HR_SAMPLE_RATE
#define HR_MIN_PUBLISH_SAMPLES (HR_SAMPLE_RATE * HR_MIN_WINDOW_SEC)
#define HR_EMA_ALPHA_FAST     0.55f
#define HR_EMA_ALPHA_SLOW     0.30f
#define HR_HOLD_MAX_SEC       8
#define HR_BPM_MIN            40.0f
#define HR_BPM_MAX            200.0f

// ============================================================================
// ESTRUTURAS DE DADOS COMPARTILHADAS
// ============================================================================

// Filtro LMS Adaptativo
typedef struct {
    float w[FILTER_TAP_NUM]; // Pesos do filtro
    float x[FILTER_TAP_NUM]; // Buffer de referência do ruído (Acelerômetro)
    float mu;                // Taxa de aprendizado/adaptação (ex: 0.01)
} lms_filter_t;

// Calculadora HRV
typedef struct {
    int32_t ibi_buffer[HRV_BUFFER_SIZE];
    size_t count;
    size_t head;
} hrv_calculator_t;

// Amostra escalar para ring buffer
typedef struct {
    int64_t timestamp_ms;
    float value;
} scalar_sample_t;

// Ring Buffer Genérico
typedef struct {
    scalar_sample_t data[RING_BUF_CAPACITY];
    size_t head;
    size_t count;
    SemaphoreHandle_t mutex;
} scalar_ring_buf_t;

// Handles do Barramento e Dispositivos (extern para acesso global)
extern i2c_master_bus_handle_t i2c_bus_handle_0;  // MAX30102
extern i2c_master_bus_handle_t i2c_bus_handle_1;  // BMI160
extern i2c_master_dev_handle_t max30102_dev_handle;
extern i2c_master_dev_handle_t bmi160_dev_handle;

// Variáveis globais compartilhadas
extern lms_filter_t s_lms_filter;
extern hrv_calculator_t s_hrv_data;
extern float s_latest_rmssd;
extern float s_latest_enmo;
extern scalar_ring_buf_t s_buf_imu;

// ============================================================================
// FUNÇÕES DO RING BUFFER (Thread-safe)
// ============================================================================

/**
 * @brief Inicializa o ring buffer (cria mutex)
 * @param rb Ponteiro para o ring buffer
 * @return ESP_OK se sucesso
 */
esp_err_t ring_buf_init(scalar_ring_buf_t *rb);

/**
 * @brief Insere amostra no ring buffer (O(1))
 * @param rb Ponteiro para o ring buffer
 * @param sample Amostra a inserir
 */
void ring_buf_push(scalar_ring_buf_t *rb, scalar_sample_t sample);

/**
 * @brief Extrai snapshot contínuo centrado no ponto de gatilho
 * @param rb Ponteiro para o ring buffer
 * @param total_samples Número de amostras a extrair
 * @param out_array Array de saída (deve ter espaço para total_samples)
 */
void ring_buf_extract_snapshot(scalar_ring_buf_t *rb, 
                                size_t total_samples, 
                                scalar_sample_t *out_array);

/**
 * @brief Inicialização dos dois barramentos I2C mestre e dispositivos
 * @return ESP_OK se sucesso
 */
esp_err_t i2c_master_init(void);

/**
 * @brief Função auxiliar: Filtro DC (remove componente DC)
 * @param sample Amostra bruta
 * @return Componente AC
 */
uint32_t dc_filter(uint32_t sample);

/**
 * @brief Função auxiliar: Filtro Passa-Baixas (média móvel de 4)
 * @param input Entrada
 * @return Saída filtrada
 */
float low_pass_filter(float input);

#ifdef __cplusplus
}
#endif

#endif // SHARED_CONFIG_H