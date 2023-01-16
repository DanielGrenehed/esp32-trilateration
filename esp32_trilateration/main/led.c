#include <driver/gpio.h>
#include "led.h"
#include "str.h"
#include "esp_log.h"

static const char * TAG = "LED";

#define LED_PIN CONFIG_TRILAT_LED_PIN
#define LED_ON_VALUE 0

static uint8_t led_state_on = 0;

void led_init() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(LED_PIN, GPIO_PULLDOWN_ENABLE);
    led_set_off();
}

void led_toggle() {
    led_state_on = !led_state_on;
    gpio_set_level(LED_PIN, led_state_on?LED_ON_VALUE:!LED_ON_VALUE);
}

void led_set_on() {
    if (!led_state_on) led_state_on = 1;
    gpio_set_level(LED_PIN, LED_ON_VALUE);
}

void led_set_off() {
    if (led_state_on) led_state_on = 0;
    gpio_set_level(LED_PIN, !LED_ON_VALUE);
} 

void on_led_command(const char* arguments) {
    if (str_starts_with(arguments, "t") > -1) led_toggle();
    else if (str_starts_with(arguments, "h") > -1) led_set_on();
    else if (str_starts_with(arguments, "l") > -1) led_set_off();
    else ESP_LOGI(TAG, "Unknown arguments: %s", arguments);
}