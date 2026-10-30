#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <stdbool.h>
#include "esp_err.h"

// Tópico MQTT
#define MQTT_TOPIC_HEART_RATE "/detection/heart_rate"

// Inicializa a conexão Wi-Fi e MQTT
void init_wifi(void);
void init_mqtt(void);

// Função solicitada para publicar a mensagem de batimento cardíaco
esp_err_t publish_heart_rate(int heart_rate, const char *device_id);

// Retorna se o broker MQTT está conectado
bool is_mqtt_connected(void);

#endif // MQTT_PUBLISHER_H