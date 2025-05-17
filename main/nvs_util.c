#include "nvs_util.h"

static const char *TAG = "NVS_UTIL";
extern SemaphoreHandle_t user_config_mutex;

// 保存 Wi-Fi 配置到 NVS
esp_err_t save_wifi_config(char *wifi_ssid, char *wifi_pass)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        goto save_wifi_config_end;
    }
    err = nvs_set_str(nvs_handle, "ssid", wifi_ssid);
    if (err != ESP_OK) {
        goto save_wifi_config_end;
    }
    err = nvs_set_str(nvs_handle, "password", wifi_pass);
    if (err != ESP_OK) {
        goto save_wifi_config_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        goto save_wifi_config_end;
    }
save_wifi_config_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t save_to_namespace(char *user_namespace, const char *key, void *value, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_set_blob(nvs_handle, key, value, size);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
save_to_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_from_namespace(char *user_namespace, const char *key, void *out_data, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }
    err = nvs_get_blob(nvs_handle, key, out_data, &size);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }

load_from_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

extern user_config_t user_config;

void reset_user_config()
{
    ESP_LOGI(TAG, "Reset user config");
    user_config.key_gpio_num           = 9;
    user_config.wifi_scan_list_size    = 20;
    user_config.wifi_connect_max_retry = 5;
    strcpy(user_config.username, "murasame");
    strcpy(user_config.password, "0d00");
    strcpy(user_config.wifi_ap_ssid, "ESP-LIGHT-AP");
    strcpy(user_config.wifi_ap_pass, "07210721");
    strcpy(user_config.mdns_host_name, "esp-light");
    user_config.ws_recv_buf_size  = 1024;
    user_config.ws_send_buf_size  = 10 * 1024;
    user_config.msg_buf_recv_size = 1024;
    user_config.msg_buf_send_size = 30 * 1024;
}

void load_user_config()
{
    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Load user config from NVS");
    } else {
        ESP_LOGW(TAG, "Failed to load user config from NVS, using default values");
        reset_user_config();
    }

    ESP_LOGI(TAG, "KEY GPIO: %d", user_config.key_gpio_num);
    ESP_LOGI(TAG, "USERNAME: %s", user_config.username);
    ESP_LOGI(TAG, "PASSWORD: %s", user_config.password);
    ESP_LOGI(TAG, "WIFI SCAN LIST SIZE: %d", user_config.wifi_scan_list_size);
    ESP_LOGI(TAG, "WIFI CONNECT MAX RETRY: %d", user_config.wifi_connect_max_retry);
    ESP_LOGI(TAG, "MDNS HOST NAME: %s", user_config.mdns_host_name);
    ESP_LOGI(TAG, "WIFI AP SSID: %s", user_config.wifi_ap_ssid);
    ESP_LOGI(TAG, "WIFI AP PASSWORD: %s", user_config.wifi_ap_pass);
    ESP_LOGI(TAG, "WIFI SSID: %s", user_config.wifi_ssid);
    ESP_LOGI(TAG, "WIFI PASSWORD: %s", user_config.wifi_pass);
    ESP_LOGI(TAG, "WS RECV BUF SIZE: %d", user_config.ws_recv_buf_size);
    ESP_LOGI(TAG, "WS SEND BUF SIZE: %d", user_config.ws_send_buf_size);
    ESP_LOGI(TAG, "MSG BUF RECV SIZE: %d", user_config.msg_buf_recv_size);
    ESP_LOGI(TAG, "MSG BUF SEND SIZE: %d", user_config.msg_buf_send_size);
}

void save_user_config()
{
    if (save_to_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Saved user config to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save user config to NVS");
    }
}
