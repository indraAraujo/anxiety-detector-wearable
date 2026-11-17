#ifndef FCM_NOTIFIER_H
#define FCM_NOTIFIER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o módulo de notificações FCM (fila e task dedicada).
 * Deve ser chamado uma única vez após o Wi-Fi conectar.
 */
esp_err_t fcm_notifier_init(void);

/**
 * @brief Enfileira o envio assíncrono de uma notificação push via FCM.
 * Não bloqueia a task chamadora e respeita o cooldown configurado.
 *
 * @param title Título da notificação.
 * @param body  Corpo da mensagem.
 */
esp_err_t fcm_notifier_send_alert(const char *title, const char *body);

/**
 * @brief Atualiza dinamicamente o token FCM do celular destinatário.
 */
void fcm_notifier_set_device_token(const char *new_token);

/**
 * @brief Atualiza dinamicamente o Access Token OAuth2 (ya29...) em tempo de execução.
 */
void fcm_notifier_set_access_token(const char *new_access_token);

#ifdef __cplusplus
}
#endif

#endif // FCM_NOTIFIER_H