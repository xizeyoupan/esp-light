#include "button_service.h"

extern struct Button button0;
extern struct Button button1;

static const char *TAG = "BTN SERVICE";
extern EventGroupHandle_t button_event_group;

uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
    case 0:
        return gpio_get_level(BUTTON0_GPIO_NUM);
        break;
    case 1:
        return gpio_get_level(BUTTON1_GPIO_NUM);
        break;
    default:
        return 0;
        break;
    }
}

void btn_single_click_handler(void *btn)
{
    switch ((((Button *)btn)->button_id))
    {
    case 0:
        ESP_LOGI(TAG, "btn 0 single click");
        xEventGroupSetBits(button_event_group, BTN0_SINGLE_CLICK_BIT);
        break;
    case 1:
        ESP_LOGI(TAG, "btn 1 single click");
        xEventGroupSetBits(button_event_group, BTN1_SINGLE_CLICK_BIT);
        break;
    default:
        break;
    }
}

void btn_press_down_up_handler(void *btn)
{
    switch ((((Button *)btn)->button_id))
    {
    case 0:
        switch (((Button *)btn)->event)
        {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN0_DOWN_BIT);
            ESP_LOGI(TAG, "btn 0 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN0_DOWN_BIT);
            ESP_LOGI(TAG, "btn 0 press up");
            break;
        default:
            break;
        }
        break;
    case 1:
        switch (((Button *)btn)->event)
        {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN1_DOWN_BIT);
            ESP_LOGI(TAG, "btn 1 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN1_DOWN_BIT);
            ESP_LOGI(TAG, "btn 1 press up");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
