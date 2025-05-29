#include "wand_server_task.h"

extern user_config_t user_config;
static const char *TAG = "WAND_SERVER_TASK";
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT      = BIT1;

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif  = NULL;

static int s_retry_num = 0;

// Wi-Fi 事件处理程序
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START.");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {

            wifi_mode_t current_mode;
            ESP_ERROR_CHECK(esp_wifi_get_mode(&current_mode));

            if (current_mode != WIFI_MODE_STA) {
                return;
            }

            if (s_retry_num < user_config.wifi_connect_max_retry) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP, cnt = %d", s_retry_num);
            } else {
                ESP_LOGW(TAG, "Failed to connect, switching back to AP mode.");
                xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
                esp_wifi_stop();
                bemfa_ha_mqtt_stop();
                start_ap_mode();
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_retry_num = 0;
        bemfa_ha_mqtt_start();
    }
}

// 配置 STA 模式
void start_sta_mode()
{
    ESP_LOGI(TAG, "Starting STA Mode...");
    esp_wifi_stop();
    s_retry_num = 0;

    wifi_config_t sta_config = {
        .sta = {
            .ssid     = "",
            .password = "",
        },
    };

    strlcpy((char *)sta_config.sta.ssid, user_config.wifi_ssid, sizeof(sta_config.sta.ssid));
    strlcpy((char *)sta_config.sta.password, user_config.wifi_pass, sizeof(sta_config.sta.password));

    ESP_LOGI(TAG, "connecting to ap SSID:%s password:%s", sta_config.sta.ssid, sta_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void start_ap_mode()
{
    ESP_LOGI(TAG, "Starting AP Mode...");
    esp_wifi_stop();

    wifi_config_t ap_config = {
        .ap = {
            .ssid_len        = strlen(user_config.wifi_ap_ssid),
            .max_connection  = 2,
            .authmode        = WIFI_AUTH_WPA_WPA2_PSK,
            .beacon_interval = 100,
            .channel         = 11,
        },
    };

    strcpy((char *)ap_config.ap.ssid, user_config.wifi_ap_ssid);
    strcpy((char *)ap_config.ap.password, user_config.wifi_ap_pass);

    if (strlen(user_config.wifi_ap_pass) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "AP Mode started. SSID:%s Password:%s", user_config.wifi_ap_ssid, user_config.wifi_ap_pass);
}

esp_err_t start_wifi(void)
{
    esp_err_t ret_value = ESP_OK;
    s_wifi_event_group  = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ap_netif  = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    start_webserver();

    if (strlen(user_config.wifi_ssid) > 0) {
        start_sta_mode();
    } else {
        start_ap_mode();
    }

    return ret_value;
}

static void start_mdns(void)
{
    // initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    // set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(user_config.mdns_host_name));
    ESP_LOGI(TAG, "mdns hostname set to: [ %s ]", user_config.mdns_host_name);
    mdns_instance_name_set("Puteng's ESP32 Light");

    // initialize service
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
}

void wand_server_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    start_wifi();
    start_mdns();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}
