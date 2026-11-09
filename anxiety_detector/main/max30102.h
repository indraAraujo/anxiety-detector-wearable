#ifndef MAX30102_H
#define MAX30102_H

#include "shared_config.h"
#include "hr_algorithm.h"

#ifdef __cplusplus
extern "C" {
#endif

// Registradores do MAX30102
#define MAX30102_REG_INTR_STATUS_1    0x00
#define MAX30102_REG_INTR_ENABLE_1    0x02
#define MAX30102_REG_FIFO_WR_PTR      0x04
#define MAX30102_REG_OVF_COUNTER      0x05
#define MAX30102_REG_FIFO_RD_PTR      0x06
#define MAX30102_REG_FIFO_DATA        0x07
#define MAX30102_REG_FIFO_CONFIG      0x08
#define MAX30102_REG_MODE_CONFIG      0x09
#define MAX30102_REG_SPO2_CONFIG      0x0A
#define MAX30102_REG_LED1_PA          0x0C // RED
#define MAX30102_REG_LED2_PA          0x0D // IR
#define MAX30102_REG_PART_ID          0xFF

// Configurações do MAX30102
#define MAX30102_FIFO_CONFIG_VAL      0x5F  // Average = 4, rollover enabled
#define MAX30102_MODE_SPO2_HR         0x03  // SpO2 + HR mode
#define MAX30102_SPO2_CONFIG_VAL      0x0B  // 18-bit, 200Hz, 411us + avg 4 = 50Hz efetivo
#define MAX30102_LED_CURRENT_VAL      0x17  // ~4.6mA (otimizado para pulso)

#define MAX30102_FIFO_DEPTH           32
#define MAX30102_FIFO_SAMPLE_BYTES    6     // RED (3) + IR (3)
#define MAX30102_INTR_STATUS_PPG_RDY  (1 << 6)

// Threshold para detecção de dedo (valor IR bruto)
#define MAX30102_FINGER_THRESHOLD     50000

// Instância global da estrutura de PPG
extern hr_algorithm_t g_hr_algorithm;

/**
 * @brief Inicializa o sensor MAX30102
 * @return ESP_OK se sucesso
 */
esp_err_t max30102_init(void);

/**
 * @brief Lê dados do FIFO do MAX30102
 * @param red Ponteiro para valor RED
 * @param ir Ponteiro para valor IR
 * @return ESP_OK se sucesso
 */
esp_err_t max30102_read_fifo(uint32_t *red, uint32_t *ir);

/**
 * @brief Aplica filtro LMS adaptativo para remover ruído de movimento
 * @param f Ponteiro para estrutura do filtro LMS
 * @param max30102_raw_ac Componente AC bruto do MAX30102
 * @param bmi160_enmo Valor ENMO do acelerômetro
 * @return Sinal limpo após remoção de ruído
 */
float max30102_apply_lms_filter(lms_filter_t *f, float max30102_raw_ac, float bmi160_enmo);

/**
 * @brief Inicializa a estrutura e mutex do buffer de PPG
 * @return ESP_OK se sucesso
 */
esp_err_t max30102_hr_algorithm_init(void);

/**
 * @brief Task FreeRTOS para leitura contínua do MAX30102 (50Hz) - alimenta o Ring Buffer
 * @param pvParameters Parâmetros da task (não utilizado)
 */
void vTaskMAX30102(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // MAX30102_H