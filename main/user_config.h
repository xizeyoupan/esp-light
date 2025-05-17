#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "math.h"
#include "string.h"

#include "cJSON.h"
#include "mdns.h"
#include "multi_button.h"

#include "esp_clk_tree.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "hal/efuse_hal.h"
#include "hal/efuse_ll.h"

#include "service/device_service.h"
#include "service/wifi_service.h"
#include "service/button_service.h"
#include "service/http_service.h"

#include "task/button.h"
#include "task/ws_task.h"
#include "task/handle_req_task.h"
#include "task/wand_server_task.h"

#include "nvs_flash.h"
#include "nvs_util.h"

typedef enum {
    COLOR_RED     = 0xff0000,
    COLOR_GREEN   = 0x00ff00,
    COLOR_BLUE    = 0x0000ff,
    COLOR_YELLOW  = 0xFFFF00,
    COLOR_MAGENTA = 0xFF00FF,
    COLOR_CYAN    = 0x00FFFF,
    COLOR_WHITE   = 0xFFFFFF,
    COLOR_NONE    = 0x000000,
} color_enum;

#define SW_VERSION                "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_NVS_KEY       "config_data"
#define MODEL_DATASET_ID          -1
#define WIFI_SSID_MAX_LEN            32
#define WIFI_PASS_MAX_LEN            64


#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
typedef struct
{
    gpio_num_t key_gpio_num;
    char username[32];
    char password[32];
    char mdns_host_name[32];
    char wifi_ap_ssid[WIFI_SSID_MAX_LEN];
    char wifi_ap_pass[WIFI_PASS_MAX_LEN];
    char wifi_ssid[WIFI_SSID_MAX_LEN];
    char wifi_pass[WIFI_PASS_MAX_LEN];
    int wifi_scan_list_size;
    int wifi_connect_max_retry;
    int ws_recv_buf_size;
    int ws_send_buf_size;
    int msg_buf_recv_size;
    int msg_buf_send_size;
} user_config_t;

#ifdef __cplusplus
}
#endif

#endif