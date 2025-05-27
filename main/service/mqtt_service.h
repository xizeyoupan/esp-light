#ifndef __MQTT_SERVICE_H__
#define __MQTT_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void bemfa_mqtt_publish_state_topic();
void bemfa_ha_mqtt_start(void);
void bemfa_ha_mqtt_stop(void);
void ha_mqtt_register_entity(void);
void ha_mqtt_remove_entity(void);
void bemfa_ha_mqtt_publish_state_topic();

#ifdef __cplusplus
}
#endif

#endif