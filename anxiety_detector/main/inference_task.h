#ifndef INFERENCE_TASK_H
#define INFERENCE_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa e lança a task de inferência no FreeRTOS
esp_err_t inference_task_init(void);

// Obtém o último estado predito (0: Repouso, 1: Estresse)
int inference_get_latest_state(void);

#ifdef __cplusplus
}
#endif

#endif // INFERENCE_TASK_H