#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "shared_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Registradores do BMI160
#define BMI160_REG_ACC_X_LSB      0x12
#define BMI160_REG_ACC_CONF       0x40
#define BMI160_REG_ACC_RANGE      0x41
#define BMI160_REG_CMD            0x7E
#define BMI160_REG_PMU_STATUS     0x03
#define BMI160_CHIP_ID            0xD1

// Configurações do BMI160
#define BMI160_ACC_RANGE_2G       0x03
#define BMI160_ACC_ODR_100HZ      0x28
#define BMI160_ACC_NORMAL_MODE    0x11
#define BMI160_SCALE_FACTOR_2G    16384.0f

/**
 * @brief Inicializa o acelerômetro BMI160
 * @return ESP_OK se sucesso
 */
esp_err_t accelerometer_init(void);

/**
 * @brief Lê aceleração bruta e calcula magnitude ENMO
 * @param out_mag_g Ponteiro para armazenar magnitude em g
 * @return true se leitura bem-sucedida
 */
bool accelerometer_read_enmo(float *out_mag_g);

/**
 * @brief Task FreeRTOS para leitura contínua do acelerômetro (50Hz)
 * @param pvParameters Parâmetros da task (não utilizado)
 */
void vTaskAccelerometer(void *pvParameters);

/**
 * @brief Obtém o último valor ENMO calculado
 * @return Último ENMO médio
 */
float accelerometer_get_latest_enmo(void);

#ifdef __cplusplus
}
#endif

#endif // ACCELEROMETER_H