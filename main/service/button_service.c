#include "button_service.h"

extern struct Button button0;
extern struct Button button1;

static const char *TAG = "BTN SERVICE";
extern EventGroupHandle_t button_event_group;
extern user_config_t user_config;

uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id) {
        case 0:
            return gpio_get_level(user_config.key_gpio_num);
            break;
        default:
            break;
    }
    return 0;
}

void btn_single_click_handler(void *btn)
{
    switch ((((Button *)btn)->button_id)) {
        case 0:
            ESP_LOGI(TAG, "btn 0 single click");
            xEventGroupSetBits(button_event_group, BTN0_SINGLE_CLICK_BIT);
            break;
        default:
            break;
    }
}

void btn_press_down_up_handler(void *btn)
{
    switch ((((Button *)btn)->button_id)) {
        case 0:
            switch (((Button *)btn)->event) {
                case PRESS_UP:
                    xEventGroupSetBits(button_event_group, BTN0_UP_BIT);
                    ESP_LOGI(TAG, "btn 0 press up");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void btn_long_press_handler(void *btn)
{
    switch ((((Button *)btn)->button_id)) {
        case 0:
            switch (((Button *)btn)->event) {
                case LONG_PRESS_START:
                    xEventGroupSetBits(button_event_group, BTN0_LONG_PRESS_START_BIT);
                    ESP_LOGI(TAG, "btn 0 long start");
                    break;
                case LONG_PRESS_HOLD:
                    xEventGroupSetBits(button_event_group, BTN0_LONG_PRESS_HOLD_BIT);
                    // ESP_LOGI(TAG, "btn 0 long press");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
