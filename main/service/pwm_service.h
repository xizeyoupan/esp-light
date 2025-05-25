#ifndef __PWM_SERVICE_H__
#define __PWM_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void ledc_init();
uint8_t get_output_pwm_value();
void ledc_update_pwm();

#ifdef __cplusplus
}
#endif

#endif