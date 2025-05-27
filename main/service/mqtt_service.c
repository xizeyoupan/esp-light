#include "mqtt_service.h"

extern user_config_t user_config;
extern MessageBufferHandle_t xMessageBufferReqRecv;

static const char *TAG          = "MQTT_SERVICE";
const char *send_config_payload = "{\"type\":\"quest\",\"param\":\"get_user_config\",\"data\":\"\"}";

esp_mqtt_client_handle_t bemfa_mqtt_client = NULL;
esp_mqtt_client_handle_t ha_mqtt_client    = NULL;

char discovery_topic[256];
char command_topic[256];
char state_topic[256];
char brightness_command_topic[256];
char brightness_state_topic[256];

void gen_discovery_topic()
{
    snprintf(discovery_topic, sizeof(discovery_topic), "%s/light/%s/config", user_config.ha_discovery_prefix, user_config.ha_unique_id);
    snprintf(command_topic, sizeof(command_topic), "%s/switch", user_config.ha_unique_id);
    snprintf(state_topic, sizeof(state_topic), "%s/state", user_config.ha_unique_id);
    snprintf(brightness_command_topic, sizeof(brightness_command_topic), "%s/brightness/set", user_config.ha_unique_id);
    snprintf(brightness_state_topic, sizeof(brightness_state_topic), "%s/brightness/state", user_config.ha_unique_id);
}

static int get_brightness_from_bemfa_topic(const char *topic_data)
{
    char op[8];
    int brightness = -1;
    if (strchr(topic_data, '#')) {
        if (sscanf(topic_data, "%[^#]#%d", op, &brightness) == 2) {
            ESP_LOGI(TAG, "op: %s", op);
            ESP_LOGI(TAG, "value: %d", brightness);
        } else {
            ESP_LOGE(TAG, "Failed to parse MQTT data");
        }
    } else {
        if (sscanf(topic_data, "%s", op) == 1) {
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

    return brightness;
}

static void update_brightness_and_push_config(uint8_t brightness)
{
    user_config.brightness_input = brightness;
    ESP_LOGI(TAG, "Setting brightness to %d", user_config.brightness_input);
    ledc_update_pwm();
    save_user_config();
    xMessageBufferSend(xMessageBufferReqRecv, send_config_payload, strlen(send_config_payload), portMAX_DELAY);
}

static void ha_mqtt_publish_state_topic()
{
    if (ha_mqtt_client == NULL) {
        ESP_LOGW(TAG, "Home Assistant MQTT client is not initialized, skipping publish");
        return;
    }
    int brightness = user_config.brightness_input;
    if (brightness == 0) {
        int msg_id = esp_mqtt_client_publish(ha_mqtt_client, state_topic, "OFF", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish ha state OFF successful, msg_id=%d", msg_id);
    } else {
        char data_buf[8];
        snprintf(data_buf, sizeof(data_buf), "%d", brightness);
        int msg_id = esp_mqtt_client_publish(ha_mqtt_client, state_topic, "ON", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish ha state ON successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_publish(ha_mqtt_client, brightness_state_topic, data_buf, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish ha brightness %s successful, msg_id=%d", data_buf, msg_id);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    esp_mqtt_event_handle_t event   = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            if (client == bemfa_mqtt_client) {
                msg_id = esp_mqtt_client_subscribe(bemfa_mqtt_client, user_config.mqtt_topic, 0);
                ESP_LOGI(TAG, "sent subscribe bemfa successful, msg_id=%d", msg_id);
            } else if (client == ha_mqtt_client) {
                msg_id = esp_mqtt_client_subscribe(ha_mqtt_client, command_topic, 0);
                ESP_LOGI(TAG, "sent subscribe ha command_topic successful, msg_id=%d", msg_id);
                msg_id = esp_mqtt_client_subscribe(ha_mqtt_client, brightness_command_topic, 0);
                ESP_LOGI(TAG, "sent subscribe ha brightness_command_topic successful, msg_id=%d", msg_id);
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            if (client == bemfa_mqtt_client) {
                ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED from Bemfa MQTT");
            } else if (client == ha_mqtt_client) {
                ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED from Home Assistant MQTT");
            }
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received data:");
            ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA : %.*s", event->data_len, event->data);

            if (event->data_len == 0) {
                ESP_LOGW(TAG, "Received empty data");
                return;
            }

            char topic_buf[256];
            snprintf(topic_buf, sizeof(topic_buf), "%.*s", event->topic_len, event->topic);
            char data_buf[256];
            snprintf(data_buf, sizeof(data_buf), "%.*s", event->data_len, event->data);
            ESP_LOGI(TAG, "Received topic: %s, data: %s", topic_buf, data_buf);

            if (client == bemfa_mqtt_client) {

                int brightness = get_brightness_from_bemfa_topic(data_buf);
                if (brightness != -1) {
                    update_brightness_and_push_config(brightness);
                    ha_mqtt_publish_state_topic();
                } else {
                    ESP_LOGW(TAG, "Invalid operation or no brightness value");
                }
            } else if (client == ha_mqtt_client) {

                if (strcmp(topic_buf, command_topic) == 0) {
                    if (strcmp(data_buf, "OFF") == 0) {
                        ESP_LOGI(TAG, "Received ha command to turn off");
                        update_brightness_and_push_config(0);
                        bemfa_ha_mqtt_publish_state_topic();
                    }
                } else if (strcmp(topic_buf, brightness_command_topic) == 0) {
                    int brightness = atoi(data_buf);
                    ESP_LOGI(TAG, "Received ha brightness command: %d, %s", brightness, data_buf);
                    update_brightness_and_push_config(brightness);
                    bemfa_ha_mqtt_publish_state_topic();
                } else {
                    ESP_LOGW(TAG, "Unknown topic: %.*s", event->topic_len, event->topic);
                }
            }

            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error, error_handle: %s", esp_err_to_name(event->error_handle->error_type));
            break;

        default:
            break;
    }
}

void bemfa_mqtt_publish_state_topic()
{
    if (bemfa_mqtt_client == NULL) {
        ESP_LOGW(TAG, "Bemfa MQTT client is not initialized, skipping publish");
        return;
    }

    char topic_buf[256];
    char data_buf[16] = "off";
    snprintf(topic_buf, sizeof(topic_buf), "%s/set", user_config.mqtt_topic);
    if (user_config.brightness_input > 0) {
        snprintf(data_buf, sizeof(data_buf), "on#%d", user_config.brightness_input);
    }
    ESP_LOGI(TAG, "Publishing Bemfa state to topic: %s, data: %s", topic_buf, data_buf);
    const int msg_id = esp_mqtt_client_publish(bemfa_mqtt_client, topic_buf, data_buf, 0, 0, 0);
    ESP_LOGI(TAG, "sent publish bemfa state successful, topic: %s, data: %s, msg_id=%d", topic_buf, data_buf, msg_id);
}

void bemfa_ha_mqtt_publish_state_topic()
{
    ESP_LOGI(TAG, "Publishing state to Bemfa MQTT");
    bemfa_mqtt_publish_state_topic();
    ESP_LOGI(TAG, "Publishing state to Home Assistant MQTT");
    ha_mqtt_publish_state_topic();
    ESP_LOGI(TAG, "State published to Home Assistant");
}

void bemfa_ha_mqtt_start(void)
{
    ESP_LOGI(TAG, "Starting MQTT client");
    gen_discovery_topic();
    esp_mqtt_client_config_t bemfa_mqtt_cfg = {
        .broker = {
            .address.uri = user_config.broker_address_uri,
        },
        .credentials = {
            .client_id = user_config.mqtt_client_id,
        },
    };

    esp_mqtt_client_config_t ha_mqtt_cfg = {
        .broker = {
            .address.uri = user_config.ha_broker_address,
        },
        .credentials = {
            .username                = user_config.ha_broker_username,
            .authentication.password = user_config.ha_broker_password,
        },
    };

    if (strlen(user_config.broker_address_uri) > 0 &&
        strlen(user_config.mqtt_client_id) > 0 &&
        strlen(user_config.mqtt_topic) > 0) {
        bemfa_mqtt_client = esp_mqtt_client_init(&bemfa_mqtt_cfg);
        esp_mqtt_client_register_event(bemfa_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(bemfa_mqtt_client);
        ESP_LOGI(TAG, "Bemfa MQTT client started");
    } else {
        ESP_LOGW(TAG, "Bemfa MQTT client configuration is incomplete. Skip");
        return;
    }

    if (strlen(user_config.ha_broker_address) > 0 &&
        strlen(user_config.ha_broker_username) > 0 &&
        strlen(user_config.ha_broker_password) > 0 &&
        strlen(user_config.ha_entity_name) > 0 &&
        strlen(user_config.ha_unique_id) > 0 &&
        strlen(user_config.ha_discovery_prefix) > 0) {
        ha_mqtt_client = esp_mqtt_client_init(&ha_mqtt_cfg);
        esp_mqtt_client_register_event(ha_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(ha_mqtt_client);
        ESP_LOGI(TAG, "HA MQTT client started");
    } else {
        ESP_LOGW(TAG, "Home Assistant MQTT client configuration is incomplete. Skip");
        return;
    }
}

void bemfa_ha_mqtt_stop(void)
{
    ESP_LOGI(TAG, "Stopping MQTT client");
    if (bemfa_mqtt_client) {
        esp_mqtt_client_stop(bemfa_mqtt_client);
        esp_mqtt_client_destroy(bemfa_mqtt_client);
        bemfa_mqtt_client = NULL;
    }

    if (ha_mqtt_client) {
        esp_mqtt_client_stop(ha_mqtt_client);
        esp_mqtt_client_destroy(ha_mqtt_client);
        ha_mqtt_client = NULL;
    }
    ESP_LOGI(TAG, "MQTT client stopped");
}

void ha_mqtt_register_entity(void)
{
    if (ha_mqtt_client == NULL) {
        ESP_LOGE(TAG, "HA MQTT client is not initialized");
        return;
    }

    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "name", user_config.ha_entity_name);
    cJSON_AddStringToObject(data, "unique_id", user_config.ha_unique_id);
    cJSON_AddStringToObject(data, "command_topic", command_topic);
    cJSON_AddStringToObject(data, "state_topic", state_topic);
    cJSON_AddStringToObject(data, "payload_off", "OFF");
    cJSON_AddStringToObject(data, "on_command_type", "brightness");
    cJSON_AddStringToObject(data, "brightness_state_topic", brightness_state_topic);
    cJSON_AddStringToObject(data, "brightness_command_topic", brightness_command_topic);
    cJSON_AddNumberToObject(data, "brightness_scale", 100);
    cJSON_AddBoolToObject(data, "retain", true);

    char *json_data = cJSON_PrintUnformatted(data);
    int msg_id      = esp_mqtt_client_publish(ha_mqtt_client, discovery_topic, json_data, 0, 0, 0);
    ESP_LOGI(TAG, "sent HA entity registration successful, topic: %s, data: %s, msg_id=%d", discovery_topic, json_data, msg_id);

    free(json_data);
    cJSON_Delete(data);
}

void ha_mqtt_remove_entity(void)
{
    if (ha_mqtt_client == NULL) {
        ESP_LOGE(TAG, "HA MQTT client is not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_publish(ha_mqtt_client, discovery_topic, "", 0, 0, 0);
    ESP_LOGI(TAG, "sent HA entity removal successful, topic: %s, msg_id=%d", discovery_topic, msg_id);
}
