#ifndef __BUTTON_SERVICE_H__
#define __BUTTON_SERVICE_H__

#include "user_config.h"

#define BTN0_DOWN_BIT             BIT0
#define BTN0_UP_BIT               BIT1
#define BTN0_SINGLE_CLICK_BIT     BIT2
#define BTN0_LONG_PRESS_START_BIT BIT3
#define BTN0_LONG_PRESS_HOLD_BIT  BIT4

#ifdef __cplusplus
extern "C" {
#endif

uint8_t read_button_GPIO(uint8_t button_id);
void btn_single_click_handler(void *btn);
void btn_press_down_up_handler(void *btn);
void btn_long_press_handler(void *btn);

#ifdef __cplusplus
}
#endif

#endif