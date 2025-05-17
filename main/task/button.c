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
    // button_init(&button1, read_button_GPIO, 0, 1);

    button_attach(&button0, SINGLE_CLICK, btn_single_click_handler);
    // button_attach(&button1, SINGLE_CLICK, btn_single_click_handler);

    button_attach(&button0, PRESS_UP, btn_press_down_up_handler);
    // button_attach(&button0, PRESS_DOWN, btn_press_down_up_handler);

    button_attach(&button1, PRESS_UP, btn_press_down_up_handler);
    // button_attach(&button1, PRESS_DOWN, btn_press_down_up_handler);

    button_start(&button0);
    // button_start(&button1);

    while (1) {
        button_ticks();
        vTaskDelay(pdMS_TO_TICKS(TICKS_INTERVAL));
    }

    vTaskDelete(NULL);
}
