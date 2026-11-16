#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "mqtt_publisher.h"

#define WIFI_SSID      "VIVOFIBRA-2701"
#define WIFI_PASS      "fHJPeRepqR"

static const char *TAG = "MQTT_PUB";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

// --- WI-FI EVENT HANDLER ---
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi desconectado. Tentando reconectar...");
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        mqtt_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Se o Wi-Fi voltou e o cliente MQTT já existe, força a reconexão no broker
        if (mqtt_client != NULL && !mqtt_connected) {
            ESP_LOGI(TAG, "Reconectando MQTT após recuperação da rede...");
            esp_mqtt_client_reconnect(mqtt_client);
        }
    }
}

// --- INITIALIZE WI-FI ---
void init_wifi(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    esp_netif_set_hostname(sta_netif, "ESP32S3-HeartRate");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Aguardando conexão Wi-Fi...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

// --- MQTT EVENT HANDLER ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao Broker HiveMQ!");
            mqtt_connected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado do Broker MQTT. Tentando reconectar...");
            mqtt_connected = false;
            // Força o cliente a tentar reconectar ao broker
            esp_mqtt_client_reconnect(mqtt_client);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Erro na comunicação MQTT. Reagendando conexão...");
            mqtt_connected = false;
            break;

        default:
            break;
    }
}

// --- INITIALIZE MQTT ---
void init_mqtt(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.15.6:1883",
        .network.reconnect_timeout_ms = 5000, // Tenta reconectar a cada 5 segundos se falhar
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// --- FUNÇÃO DE PUBLICAÇÃO DE HEART RATE ---
esp_err_t publish_heart_rate(int heart_rate, const char *device_id) {
    if (mqtt_client == NULL || !mqtt_connected) {
        ESP_LOGW(TAG, "Falha ao publicar: Cliente MQTT não conectado. Aguardando reconexão...");
        return ESP_FAIL;
    }

    char payload[128];
    snprintf(payload, sizeof(payload), "{\"heart_rate\": \"%d\", \"device_id\": \"%s\"}", heart_rate, device_id);

    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_HEART_RATE, payload, 0, 1, 0);

    if (msg_id != -1) {
        ESP_LOGI(TAG, "Publicado com sucesso (msg_id=%d) -> Tópico: %s | Payload: %s", 
                 msg_id, MQTT_TOPIC_HEART_RATE, payload);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Erro ao publicar mensagem no MQTT.");
        return ESP_FAIL;
    }
}

bool is_mqtt_connected(void) {
    return mqtt_connected;
}