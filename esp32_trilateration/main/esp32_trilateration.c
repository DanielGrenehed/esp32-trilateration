#include <stdio.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/i2c.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart.h"
#include "bluetooth.h"

#define LED_PIN GPIO_NUM_22

void config_led() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(LED_PIN, GPIO_PULLDOWN_ENABLE);
}

static const char * TAG = "LED";
static uint32_t led_on = 0;

static void toggle_led() {
    led_on = !led_on;
    gpio_set_level(GPIO_NUM_22, led_on);
}

static void toggle_led_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        toggle_led();
    }
}

static void callback(const char* command) {
    toggle_led();
}

void app_main(void) {
    config_led();

    uart_cli_set_command_callback(callback);
    xTaskCreate(uart_cli_task, "uart_cli_task", TRILAT_TASK_STACK_SIZE, NULL, 10, NULL);
    //xTaskCreate(toggle_led_task, "toggle_led_task", TRILAT_TASK_STACK_SIZE, NULL, 10, NULL);
    ble_scanner_task();
}
