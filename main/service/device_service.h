#ifndef __DEVICE_SERVICE_H__
#define __DEVICE_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON *get_device_info(void);
cJSON *get_task_state(void);

#ifdef __cplusplus
}
#endif

#endif