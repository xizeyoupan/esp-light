#include "mqtt_service.h"

extern user_config_t user_config;
extern MessageBufferHandle_t xMessageBufferReqRecv;

static const char *TAG               = "BEMFA_MQTT";
esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    esp_mqtt_event_handle_t event   = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(mqtt_client, user_config.mqtt_topic, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received data:");
            ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA : %.*s", event->data_len, event->data);

            if (event->data_len == 0) {
                ESP_LOGW(TAG, "Received empty data");
                return;
            }
            event->data[event->data_len] = '\0';

            char op[16];
            int value      = 0;
            int brightness = -1;
            if (strchr(event->data, '#')) {
                if (sscanf(event->data, "%[^#]#%d", op, &value) == 2) {
                    ESP_LOGI(TAG, "op: %s", op);
                    ESP_LOGI(TAG, "value: %d", value);
                    brightness = value;
                } else {
                    ESP_LOGE(TAG, "Failed to parse MQTT data");
                }
            } else {
                if (sscanf(event->data, "%s", op) == 1) {
                    ESP_LOGI(TAG, "op: %s", op);
                }
                if (strcmp(op, "on") == 0) {
                    brightness = 100;
                } else if (strcmp(op, "off") == 0) {
                    brightness = 0;
                } else {
                    ESP_LOGW(TAG, "Unknown operation: %s", op);
                }
            }

            if (brightness != -1) {
                const char *payload          = "{\"type\":\"quest\",\"param\":\"get_user_config\",\"data\":\"\"}";
                user_config.brightness_input = brightness;

                ESP_LOGI(TAG, "Setting brightness to %d", user_config.brightness_input);
                ledc_update_pwm();
                save_user_config();
                xMessageBufferSend(xMessageBufferReqRecv, payload, strlen(payload), portMAX_DELAY);

            } else {
                ESP_LOGW(TAG, "Invalid operation or no brightness value");
            }

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

void mqtt_publish_brightness()
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }

    char topic_buf[256];
    char data_buf[16] = "off";
    snprintf(topic_buf, sizeof(topic_buf), "%s/set", user_config.mqtt_topic);
    if (user_config.brightness_input > 0) {
        snprintf(data_buf, sizeof(data_buf), "on#%d", user_config.brightness_input);
    }
    const int msg_id = esp_mqtt_client_enqueue(mqtt_client, topic_buf, data_buf, 0, 0, 1, true);
    ESP_LOGI(TAG, "sent publish successful, topic: %s, data: %s, msg_id=%d", topic_buf, data_buf, msg_id);
}

void bemfa_mqtt_start(void)
{
    ESP_LOGI(TAG, "Starting MQTT client");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = user_config.broker_address_uri,
        },
        .credentials = {
            .client_id = user_config.mqtt_client_id,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT client started");
}

void bemfa_mqtt_stop(void)
{
    ESP_LOGI(TAG, "Stopping MQTT client");
    if (mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }
    ESP_LOGI(TAG, "MQTT client stopped");
}
