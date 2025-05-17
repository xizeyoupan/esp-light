#include "ws_task.h"

static const char *TAG = "WS_TASK";

extern httpd_handle_t server;
extern int current_ws_fd;
extern user_config_t user_config;

esp_err_t ws_send_data(uint8_t *data, uint16_t data_size, httpd_ws_type_t type)
{
    if (data == NULL) {
        return ESP_FAIL;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = data;
    ws_pkt.len     = data_size;
    ws_pkt.type    = type;

    // ESP_LOGI(TAG, "data len:%d", ws_pkt.len);

    httpd_ws_send_frame_async(server, current_ws_fd, &ws_pkt);
    return ESP_OK;
}

MessageBufferHandle_t xMessageBufferReqSend;
void websocket_send_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    // Create Message Buffer
    xMessageBufferReqSend = xMessageBufferCreate(user_config.msg_buf_send_size);
    configASSERT(xMessageBufferReqSend);

    uint8_t *buffer = malloc(user_config.ws_send_buf_size);

    while (1) {
        size_t msg_size = xMessageBufferReceive(xMessageBufferReqSend, buffer, user_config.ws_send_buf_size, portMAX_DELAY); // 读取完整消息

        if (server && current_ws_fd != -1) {
            ws_send_data(buffer, msg_size, HTTPD_WS_TYPE_TEXT);
        } else {
            // ESP_LOGW(TAG, "server is null");
        }
    }
    free(buffer);
    vTaskDelete(NULL);
}
