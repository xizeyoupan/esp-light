#ifndef __DEVICE_SERVICE_H__
#define __DEVICE_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON *get_device_info(void);
cJSON *get_task_state(void);
cJSON *get_user_config_json(void);
void assign_ledc_config_from_json(const cJSON *);
void assign_user_config_from_json(const cJSON *data);

#ifdef __cplusplus
}
#endif

#endif