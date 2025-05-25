#ifndef __MQTT_SERVICE_H__
#define __MQTT_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_publish_brightness();
void bemfa_mqtt_start(void);
void bemfa_mqtt_stop(void);

#ifdef __cplusplus
}
#endif

#endif