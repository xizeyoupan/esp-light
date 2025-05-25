#include "user_config.h"
user_config_t user_config;

static const char *TAG = "MAIN";

extern MessageBufferHandle_t xMessageBufferReqSend;
extern MessageBufferHandle_t xMessageBufferReqRecv;

int my_vprintf(const char *_Format, va_list _ArgList)
{
    return vprintf(_Format, _ArgList);
    // return 0;
}

void app_main(void)
{
    // Initialize NVS
    ESP_LOGI(TAG, "Initialize NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        ESP_LOGI(TAG, "NVS flash erased");
    }
    ESP_ERROR_CHECK(ret);

    load_user_config();

    // Start wifi and web server
    xTaskCreate(&wand_server_task, "wand_server", 1024 * 5, NULL, 15, NULL);

    // Start ws
    xTaskCreate(&websocket_send_task, "websocket_send", 1024 * 5, NULL, 10, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    configASSERT(xMessageBufferReqSend);

    xTaskCreate(&handle_req_task, "handle_req", 1024 * 5, NULL, 12, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    configASSERT(xMessageBufferReqRecv);

    xTaskCreate(&scan_button_task, "scan_button", 1024 * 5, NULL, 5, NULL);

    ledc_init();

    esp_log_set_vprintf(my_vprintf);
}
