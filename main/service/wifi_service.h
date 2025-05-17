#ifndef __WIFI_SERVICE_H__
#define __WIFI_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON *get_wifi_list(void);
cJSON *get_wifi_info(void);

#ifdef __cplusplus
}
#endif

#endif
