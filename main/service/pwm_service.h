#ifndef __PWM_SERVICE_H__
#define __PWM_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void ledc_init();
uint8_t get_output_pwm_value();
void ledc_update_pwm();
void update_brightness_and_push_config(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif