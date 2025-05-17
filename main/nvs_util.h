#ifndef __NVS_UTIL_H__
#define __NVS_UTIL_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t save_wifi_config(char *wifi_ssid, char *wifi_pass);
void load_user_config();
void save_user_config();
void reset_user_config();

#ifdef __cplusplus
}
#endif

#endif