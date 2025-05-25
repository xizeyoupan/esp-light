#include "mqtt_service.h"

extern user_config_t user_config;
static const char *TAG = "PWM_SERVICE";

#define LEDC_TIMER    LEDC_TIMER_0
#define LEDC_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL  LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_7_BIT

uint8_t get_output_pwm_value()
{
    if (user_config.brightness_input == 0) {
        return 0;
    }

    uint8_t brightness = 0;

    if (user_config.output_func == OUTPUT_FUNC_GAMMA) {
        brightness = powf((float)user_config.brightness_input / 100.0f, user_config.gamma_value) * 100.0f;
        brightness = (uint8_t)(brightness * (user_config.pwm_duty_max - user_config.pwm_duty_min) / 100 + user_config.pwm_duty_min);
    } else {
        brightness = user_config.brightness_input * (user_config.pwm_duty_max - user_config.pwm_duty_min) / 100 + user_config.pwm_duty_min;
    }

    if (brightness < user_config.pwm_duty_min) {
        brightness = user_config.pwm_duty_min;
    } else if (brightness > user_config.pwm_duty_max) {
        brightness = user_config.pwm_duty_max;
    }

    return brightness;
}

uint32_t get_output_pwm_duty()
{
    uint8_t brightness = get_output_pwm_value();
    uint32_t duty      = (uint32_t)(brightness * ((1 << LEDC_DUTY_RES) - 1) / 100);
    return duty;
}

void ledc_init()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = user_config.frequency,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = user_config.pwm_gpio_num,
        .duty       = 0, // Set duty to 0%
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_LOGI(TAG, "LEDC initialized with frequency: %lu Hz, duty resolution: %d bits", user_config.frequency, LEDC_DUTY_RES);

    if (user_config.boot_action == BOOT_ACTION_FIXED) {
        user_config.brightness_input = user_config.boot_brightness;
    }

    ledc_fade_func_install(0);

    ledc_update_pwm();
}

void ledc_update_pwm()
{
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, user_config.frequency);
    ledc_set_duty_and_update(LEDC_MODE, LEDC_CHANNEL, get_output_pwm_duty(), 0);
    ESP_LOGI(TAG, "LEDC duty updated to: %lu, freq: %lu", get_output_pwm_duty(), user_config.frequency);
}
