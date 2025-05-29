#include "button.h"

extern user_config_t user_config;
struct Button button0;
struct Button button1;

static const char *TAG = "BTN TASK";
EventGroupHandle_t button_event_group;

void scan_button_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    button_event_group = xEventGroupCreate();

    gpio_config_t io_conf = {};
    io_conf.intr_type     = GPIO_INTR_DISABLE;                  // 禁用中断
    io_conf.pin_bit_mask  = (1ULL << user_config.key_gpio_num); // 设置 GPIO
    io_conf.mode          = GPIO_MODE_INPUT;                    // 设置为输入模式
    io_conf.pull_up_en    = GPIO_PULLUP_ENABLE;                 // 启用上拉电阻
    io_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;              // 禁用下拉电阻
    gpio_config(&io_conf);

    button_init(&button0, read_button_GPIO, 0, 0);

    button_attach(&button0, SINGLE_CLICK, btn_single_click_handler);
    button_attach(&button0, PRESS_UP, btn_press_down_up_handler);
    button_attach(&button0, LONG_PRESS_HOLD, btn_long_press_handler);
    button_attach(&button0, LONG_PRESS_START, btn_long_press_handler);

    button_start(&button0);

    while (1) {
        button_ticks();
        vTaskDelay(pdMS_TO_TICKS(TICKS_INTERVAL));
    }

    vTaskDelete(NULL);
}

void handle_button_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID();
    ESP_LOGI(TAG, "handle_button_task is running on core %d.", core_id);
    uint8_t prev       = 50;
    uint8_t long_press = 0;
    uint8_t dir        = 0;
    float value        = 0;

    uint64_t start = esp_timer_get_time();

    while (1) {
        EventBits_t uxReturn = xEventGroupWaitBits(button_event_group, BTN0_SINGLE_CLICK_BIT | BTN0_UP_BIT | BTN0_LONG_PRESS_HOLD_BIT | BTN0_LONG_PRESS_START_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxReturn & BTN0_SINGLE_CLICK_BIT) {
            ESP_LOGI(TAG, "uxReturn & BTN0_SINGLE_CLICK_BIT");
            if (user_config.brightness_input != 0) {
                prev = user_config.brightness_input;
                update_brightness_and_push_config(0);
            } else {
                update_brightness_and_push_config(prev);
            }
            bemfa_ha_mqtt_publish_state_topic();
        } else if (uxReturn & BTN0_UP_BIT) {
            ESP_LOGI(TAG, "uxReturn & BTN0_UP_BIT");
            if (long_press) {
                ESP_LOGI(TAG, "value: %f", value);
                if (value >= 100 || value <= 1) dir = 1 - dir;
                update_brightness_and_push_config(user_config.brightness_input);
                bemfa_ha_mqtt_publish_state_topic();
            }
            long_press = 0;
        } else if (uxReturn & BTN0_LONG_PRESS_START_BIT) {
            ESP_LOGI(TAG, "uxReturn & BTN0_LONG_PRESS_START_BIT");
            value = user_config.brightness_input;
            start = esp_timer_get_time();
        } else if (uxReturn & BTN0_LONG_PRESS_HOLD_BIT) {
            // ESP_LOGI(TAG, "uxReturn & BTN0_LONG_PRESS_HOLD_BIT");
            if (user_config.brightness_input == 0) continue;
            long_press  = 1;
            float delta = 100.0 / user_config.button_period_ms * (esp_timer_get_time() - start) / 1000;
            start       = esp_timer_get_time();

            if (dir) {
                value += delta;
                if (value >= 100) {
                    value = 100;
                }
            } else {
                value -= delta;
                if (value <= 1) {
                    value = 1;
                }
            }

            user_config.brightness_input = value;
            ledc_update_pwm();
        }
    }

    vTaskDelete(NULL);
}