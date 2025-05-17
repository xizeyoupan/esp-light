#ifndef __WAND_SERVER_TASK_H__
#define __WAND_SERVER_TASK_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_ap_mode();
void start_sta_mode();
void wand_server_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif