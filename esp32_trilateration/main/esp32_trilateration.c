#include <stdio.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "uart.h"
#include "bluetooth.h"
#include "wifi.h"
#include "str.h"
#include "led.h"
#include "websocket.h"
#include "store.h"

static const char * TAG = "MAIN";

#define HEX_PARSE_BUFFER_SIZE 6
static char parse_hex[HEX_PARSE_BUFFER_SIZE];
static void on_parse_hex_command(const char* arguments) {
    int success = str_parse_hex(arguments, HEX_PARSE_BUFFER_SIZE*2, parse_hex);
    ESP_LOG_BUFFER_HEX(TAG, parse_hex, success/2);
    if (success != HEX_PARSE_BUFFER_SIZE*2) ESP_LOGE(TAG, "Failed to read %d nibbles", HEX_PARSE_BUFFER_SIZE*2);
}

static void send_data() {
    bt_send_scan_buffer(ws_send_blob);
    /*
        get data from scan
        send data over websocket
    */
}

static void process_command(const char* command) {
    int c_end;
    if ((c_end = str_starts_with(command, "led")) > -1) {on_led_command(command+c_end);}
    else if ((c_end = str_starts_with(command, "bt")) > -1) {on_bt_command(command+c_end);}
    else if ((c_end = str_starts_with(command, "wf")) > -1) {on_wifi_command(command+c_end);}
    else if ((c_end = str_starts_with(command, "hex")) > -1) {on_parse_hex_command(command+c_end);}
    else if ((c_end = str_starts_with(command, "ws")) > -1) {on_ws_command(command+c_end);}
    else if (str_starts_with(command, "sd") > -1) {send_data();}
    else if (str_starts_with(command, "reset") > -1) {esp_restart();}
    else ESP_LOGI(TAG, "Unknown command: %s",command);
}

static const char ws_handshake_initializer[8] = {0xde, 0xad, 0xbe, 0xaf, 0xde, 0xca, 0xfe, 0x00};
static void send_handshake() {
    ws_send_blob(ws_handshake_initializer, 7);
}

static void ws_handshake_on_connect() {
    send_handshake();
}

char ds_blob[1+SCAN_DEVICE_STRUCT_SIZE] = "s";
static void send_direct_scan_on_ws(struct bt_scan_device_t device) {
    for (int i = 0; i < SCAN_DEVICE_STRUCT_SIZE; i++) ds_blob[i+1] = ((char*)&device)+i;
    ws_send_blob(ds_blob, 1+SCAN_DEVICE_STRUCT_SIZE);
}

void app_main(void) {
    store_init();
    led_init();

    ws_set_on_message_callback(process_command);
    ws_set_on_connect_callback(ws_handshake_on_connect);
    
    set_wifi_log_output(ws_send);
    set_wifi_connect_callback(ws_connect);
    set_bt_log_output(ws_send);
    set_bt_scan_direct_callback(send_direct_scan_on_ws);

    uart_cli_set_command_callback(process_command);
    xTaskCreate(uart_cli_task, "uart_cli_task", TRILAT_TASK_STACK_SIZE, NULL, 10, NULL);
    
    ble_scanner_task();
    
    wifi_init();
}
