#include "handle_req_task.h"

static const char *TAG = "HANDLE_REQ_TASK";

extern MessageBufferHandle_t xMessageBufferReqSend;
extern user_config_t user_config;
MessageBufferHandle_t xMessageBufferReqRecv;

void handle_req_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    xMessageBufferReqRecv = xMessageBufferCreate(user_config.msg_buf_recv_size);
    configASSERT(xMessageBufferReqRecv);

    uint8_t *buffer = malloc(user_config.ws_recv_buf_size);

    while (1) {
        size_t msg_size  = xMessageBufferReceive(xMessageBufferReqRecv, buffer, user_config.ws_recv_buf_size, portMAX_DELAY); // 读取完整消息
        buffer[msg_size] = 0;
        ESP_LOGI(TAG, "Receive ws size: %d, data: %s", msg_size, buffer);
        // 处理 WebSocket 数据帧

        char *resp_string   = NULL;
        cJSON *resp_json    = NULL;
        const cJSON *type   = NULL;
        const cJSON *param  = NULL;
        cJSON *monitor_json = cJSON_Parse((char *)buffer);
        if (monitor_json == NULL) {
            goto json_parse_end;
        }

        type = cJSON_GetObjectItem(monitor_json, "type");
        if (type == NULL || !cJSON_IsString(type) || (type->valuestring == NULL)) {
            goto json_parse_end;
        }

        param = cJSON_GetObjectItem(monitor_json, "param");
        if (param == NULL || !cJSON_IsString(param) || (param->valuestring == NULL)) {
            goto json_parse_end;
        }

        resp_json = cJSON_CreateObject();
        if (resp_json == NULL) {
            goto json_parse_end;
        }

        if (strcmp(type->valuestring, "quest") == 0) {
            cJSON_AddStringToObject(resp_json, "type", "update");
            if (strcmp(param->valuestring, "get_device_info") == 0) {

                cJSON_AddStringToObject(resp_json, "param", "device_info");
                cJSON *device_info = get_device_info();
                if (device_info == NULL) {
                    goto json_parse_end;
                }
                cJSON_AddItemToObject(resp_json, "data", device_info);

            } else if (strcmp(param->valuestring, "get_wifi_info") == 0) {

                cJSON_AddStringToObject(resp_json, "param", "wifi_info");
                cJSON *wifi_info = get_wifi_info();
                if (wifi_info == NULL) {
                    goto json_parse_end;
                }
                cJSON_AddItemToObject(resp_json, "data", wifi_info);

            } else if (strcmp(param->valuestring, "get_wifi_list") == 0) {

                cJSON_AddStringToObject(resp_json, "param", "wifi_list");
                cJSON *wifi_list = get_wifi_list();
                if (wifi_list == NULL) {
                    goto json_parse_end;
                }
                cJSON_AddItemToObject(resp_json, "data", wifi_list);

            } else if (strcmp(param->valuestring, "connect_wifi") == 0) {
                const cJSON *data     = cJSON_GetObjectItem(monitor_json, "data");
                const cJSON *ssid     = cJSON_GetObjectItem(data, "ssid");
                const cJSON *password = cJSON_GetObjectItem(data, "password");
                strcpy(user_config.wifi_ssid, ssid->valuestring);
                strcpy(user_config.wifi_pass, password->valuestring);

                save_user_config();
                start_sta_mode();

            } else if (strcmp(param->valuestring, "get_state_info") == 0) {

                cJSON_AddStringToObject(resp_json, "param", "state_info");
                cJSON *task_state = get_task_state();
                if (task_state == NULL) {
                    goto json_parse_end;
                }
                cJSON_AddItemToObject(resp_json, "data", task_state);

            } else if (strcmp(param->valuestring, "get_state_info") == 0) {
            }
        } else if (strcmp(type->valuestring, "ping") == 0) {
            cJSON_AddStringToObject(resp_json, "type", "pong");
        }

        resp_string = cJSON_Print(resp_json);
        if (resp_string == NULL) {
            goto json_parse_end;
        }

    json_parse_end:
        if (resp_string != NULL) {
            ESP_LOGI(TAG, "Sending ws size: %d, data: %s", strlen(resp_string), resp_string);
            xMessageBufferSend(xMessageBufferReqSend, resp_string, strlen(resp_string), portMAX_DELAY);
        }
        cJSON_Delete(monitor_json);
        cJSON_Delete(resp_json);
        free(resp_string);
    }

    free(buffer);
    vTaskDelete(NULL);
}
