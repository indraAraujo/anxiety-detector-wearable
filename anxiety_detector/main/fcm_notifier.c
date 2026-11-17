#include "fcm_notifier.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include <time.h>

extern const uint8_t google_root_ca_pem_start[] asm("_binary_google_root_ca_pem_start");
extern const uint8_t google_root_ca_pem_end[]   asm("_binary_google_root_ca_pem_end");

static const char *TAG = "FCM_NOTIFIER";

// ============================================================================
// CONFIGURAÇÕES DO FIREBASE
// ============================================================================
#define FCM_PROJECT_ID          "calm-3e974"
#define FCM_COOLDOWN_MINUTES    60
#define FCM_QUEUE_LENGTH        4
#define FCM_TASK_STACK_SIZE     8192   // 8 KB suficientes (sem RSA/PSA)
#define FCM_TASK_PRIORITY       2

// Cole aqui o Access Token OAuth2 gerado externamente (válido por 1 hora)
static char s_access_token[2048] = "ya29.c.c0AZ4bNpbyzVy_UOJ2swCXgD9dQOQlinmNo6Kvv-uk-g3U_juwVgLV1RJ3lsHTcd094HB1zgXMGSy4ivbGbdD4u8wCFdQNyZ7Luo9SApSQBYkxQmCvAnG8cCGPaxut6IwSJ6H7JwRc8i4YGu5E-h_SbRmPabxtsVrBu46vaDGv0aH5_OwEH0jpYChSk6fJ1JvSzw397rix5feINAN7jqtobSRQJ9Poft6nfxz1giAC5Wt4e-pwhUMCKaGgXqPnC7ivG7_aT90Hg2xCITqJaHkRZ2hQ2nvBdxkN4-2zEQHsRZXPL9VdXx6qVz6cn77YL3EQt-r1aIsaf636-ymnMlEHc8t1ByCn8xdauS2-ZakvGMJnYBztZdohz51QT385C8jUin0n-4SvhYbRWMI6jfmreq1Q35JjQaF-Oo3054rmd6vtSxcWJox9917eXexvzBjlsp64Qk0k0ojdyVhmOXgdlbsMJf66Q2cO94hOgjbSR2YobOVUX0OIqdFaaw3OeBUBQdpd_yqieRUOFRFwIh1ydzx7g633uugju6_31VqyaoiFYn85mdx2QhgQVvxrcYiWvtISpWIoxijVol3Vk9zZ4h9_Ye4xc_9ooss9brcBy9aOn0V5sRXuoQWZ2vmctJBueuhvWof_hhgZY-t5sQQBBzrOWmnbZrWI12ihQ5cORtBdmIiUyFZX6coRcpXUyrcXn1i1lFZIUlaf3ugVYe9w3YiomjF4WrjIS5u0jysuvsjaMghiFZ2puxVop68tJembIJqXyqz3SUJIerawgm610p7cie2z8Il6l-m1dVXghXvhmq0t17vcrnURcuxqubBsiQoWqvp-Iuth2F9cO8e1-VQfoIkOkYdrp2pwUWY_t5dVux2WtYyx8M0MnsvZoIxr5srl2pnRtOpeI7ydxMWmi7S6QWelh14e6JWBSzO_g1aMsmF1u023syfhiMc-acSV1aWMoOZmttmXhRbdx5p48b1aQm5uXxWw5s6sX-Xe-og8zl4aYJxYBz0";
// Token FCM do celular destinatário
static char s_device_fcm_token[256] = "ctk3W8g8Rvy23fezkKVrMk:APA91bEYFKVG0NQri232srz1zF8Srk_Y3eaGqQEG31N8qrdqzftighxpLZ9u2d49eLB7ZR-Z0lw3f4gi0ewuK3wwLPfon6-YuGzVfplnniyldnJq97pcGo4";

// ============================================================================
// ESTRUTURAS E BUFFERS ESTÁTICOS (FORA DA STACK DA TASK)
// ============================================================================
typedef struct {
    char title[96];
    char body[256];
} fcm_alert_msg_t;

static QueueHandle_t s_fcm_queue = NULL;
static int64_t s_last_notification_time_us = 0;

static char s_auth_header[2200];
static char s_json_payload[640];

// ============================================================================
// FUNÇÕES INTERNAS
// ============================================================================

static void sync_clock_sntp(void) {
    time_t now = 0;
    struct tm timeinfo = {0};
    time(&now);
    localtime_r(&now, &timeinfo);

    // Se o ano já estiver sincronizado (>= 2024), não faz nada
    if (timeinfo.tm_year >= (2024 - 1900)) {
        return;
    }

    ESP_LOGI(TAG, "Sincronizando data/hora via SNTP para validação SSL...");
    if (!esp_sntp_enabled()) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_setservername(1, "time.google.com");
        esp_sntp_init();
    }

    int retry = 0;
    while (timeinfo.tm_year < (2024 - 1900) && ++retry <= 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2024 - 1900)) {
        ESP_LOGI(TAG, "Data/hora sincronizada com sucesso!");
    } else {
        ESP_LOGW(TAG, "Timeout no SNTP; verificação de certificado pode falhar.");
    }
}

static void json_escape_str(const char *src, char *dst, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < max_len; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            dst[j++] = '\\';
        }
        dst[j++] = src[i];
    }
    dst[j] = '\0';
}

static esp_err_t perform_fcm_http_post(const fcm_alert_msg_t *msg) {
    if (s_access_token[0] == '\0') {
        ESP_LOGE(TAG, "Access Token vazio! Defina s_access_token antes de enviar.");
        return ESP_FAIL;
    }

    char safe_title[192];
    char safe_body[512];
    json_escape_str(msg->title, safe_title, sizeof(safe_title));
    json_escape_str(msg->body, safe_body, sizeof(safe_body));

    int payload_len = snprintf(s_json_payload, sizeof(s_json_payload),
        "{\"message\":{\"token\":\"%s\",\"notification\":{\"title\":\"%s\",\"body\":\"%s\"}}}",
        s_device_fcm_token, safe_title, safe_body);

    if (payload_len < 0 || payload_len >= sizeof(s_json_payload)) {
        ESP_LOGE(TAG, "Buffer s_json_payload insuficiente");
        return ESP_FAIL;
    }

    char url[128];
    snprintf(url, sizeof(url), "https://fcm.googleapis.com/v1/projects/%s/messages:send", FCM_PROJECT_ID);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .cert_pem = (const char *)google_root_ca_pem_start,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Falha ao inicializar cliente HTTP FCM");
        return ESP_FAIL;
    }

    snprintf(s_auth_header, sizeof(s_auth_header), "Bearer %s", s_access_token);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", s_auth_header);
    esp_http_client_set_post_field(client, s_json_payload, payload_len);

    esp_err_t ret = esp_http_client_perform(client);
    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Notificação FCM enviada! HTTP Status: %d", status_code);

        if (status_code == 200) {
            s_last_notification_time_us = esp_timer_get_time();
        } else {
            if (status_code == 401) {
                ESP_LOGE(TAG, "Token OAuth2 expirado ou inválido (HTTP 401). Gere um novo token.");
            }
            ret = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "Erro na requisição HTTP FCM: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}

static void vTaskFcmNotifier(void *pvParameters) {
    fcm_alert_msg_t msg;
    sync_clock_sntp();
    ESP_LOGI(TAG, "Task FCM Notifier iniciada e aguardando alertas...");

    while (1) {
        if (xQueueReceive(s_fcm_queue, &msg, portMAX_DELAY) == pdTRUE) {
            int64_t now_us = esp_timer_get_time();
            const int64_t cooldown_us = (int64_t)FCM_COOLDOWN_MINUTES * 60 * 1000 * 1000;

            if (s_last_notification_time_us != 0 && (now_us - s_last_notification_time_us < cooldown_us)) {
                ESP_LOGD(TAG, "Alerta ignorado: FCM em cooldown.");
                continue;
            }

            perform_fcm_http_post(&msg);
        }
    }
}

// ============================================================================
// API PÚBLICA
// ============================================================================

esp_err_t fcm_notifier_init(void) {
    if (s_fcm_queue != NULL) {
        ESP_LOGW(TAG, "Módulo FCM Notifier já inicializado.");
        return ESP_OK;
    }

    s_fcm_queue = xQueueCreate(FCM_QUEUE_LENGTH, sizeof(fcm_alert_msg_t));
    if (s_fcm_queue == NULL) {
        ESP_LOGE(TAG, "Falha ao criar fila FCM.");
        return ESP_FAIL;
    }

    BaseType_t ret = xTaskCreatePinnedToCore(
        vTaskFcmNotifier,
        "TaskFcmNotifier",
        FCM_TASK_STACK_SIZE,
        NULL,
        FCM_TASK_PRIORITY,
        NULL,
        0
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar TaskFcmNotifier.");
        vQueueDelete(s_fcm_queue);
        s_fcm_queue = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t fcm_notifier_send_alert(const char *title, const char *body) {
    if (s_fcm_queue == NULL) {
        ESP_LOGE(TAG, "FCM Notifier não inicializado! Chame fcm_notifier_init() antes.");
        return ESP_FAIL;
    }

    int64_t now_us = esp_timer_get_time();
    const int64_t cooldown_us = (int64_t)FCM_COOLDOWN_MINUTES * 60 * 1000 * 1000;
    if (s_last_notification_time_us != 0 && (now_us - s_last_notification_time_us < cooldown_us)) {
        return ESP_OK;
    }

    fcm_alert_msg_t msg = {0};
    strncpy(msg.title, title ? title : "Alerta", sizeof(msg.title) - 1);
    strncpy(msg.body, body ? body : "", sizeof(msg.body) - 1);

    if (xQueueSend(s_fcm_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Fila FCM cheia, alerta descartado.");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void fcm_notifier_set_device_token(const char *new_token) {
    if (new_token != NULL && strlen(new_token) < sizeof(s_device_fcm_token)) {
        strncpy(s_device_fcm_token, new_token, sizeof(s_device_fcm_token) - 1);
        s_device_fcm_token[sizeof(s_device_fcm_token) - 1] = '\0';
        ESP_LOGI(TAG, "Token FCM do dispositivo atualizado.");
    }
}

void fcm_notifier_set_access_token(const char *new_access_token) {
    if (new_access_token != NULL && strlen(new_access_token) < sizeof(s_access_token)) {
        strncpy(s_access_token, new_access_token, sizeof(s_access_token) - 1);
        s_access_token[sizeof(s_access_token) - 1] = '\0';
        ESP_LOGI(TAG, "Access Token OAuth2 atualizado.");
    }
}