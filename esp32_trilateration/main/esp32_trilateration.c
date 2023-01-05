#include <stdio.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "uart.h"
#include "bluetooth.h"
#include "wifi.h"
#include "str.h"

#define LED_PIN GPIO_NUM_22
static int should_log_closest_bt_device = 0;

void config_led() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(LED_PIN, GPIO_PULLDOWN_ENABLE);
}

static const char * TAG = "MAIN";
static uint32_t led_on = 0;

static void toggle_led() {
    led_on = !led_on;
    gpio_set_level(GPIO_NUM_22, led_on);
}

static const char* BT_COMMAND = "bt";
static const char* WF_COMMAND = "wf";
static const char* TOGGLE_LED_COMMAND = "tled";

static void callback(const char* command) {
    int c_end;
    if (str_starts_with(command, TOGGLE_LED_COMMAND) > -1) toggle_led();
    else if ((c_end = str_starts_with(command, BT_COMMAND)) > -1) on_bt_command(command+c_end);
    else if ((c_end = str_starts_with(command, WF_COMMAND)) > -1) on_wf_command(command+c_end);
    else {ESP_LOGI(TAG, "Unknown command: %s",command);}

}

void app_main(void) {
    config_led();

    uart_cli_set_command_callback(callback);
    xTaskCreate(uart_cli_task, "uart_cli_task", TRILAT_TASK_STACK_SIZE, NULL, 10, NULL);
    ble_scanner_task();
    ESP_LOGI(TAG, "Function completed!");
}
