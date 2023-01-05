#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "bluetooth.h"
#include "str.h"

static const char* TAG = "BT";

static int should_log_closest_bt_device = 0;
static void (*device_found_callback)(struct bt_scan_device_t) = NULL;
static struct bt_scan_device_t closest_bt_device = {.rssi = -127};

void set_bt_device_found_callback(void (*callback)(struct bt_scan_device_t)) {
    device_found_callback = callback;
}

static void log_bt_device(struct bt_scan_device_t *device) {
    if (should_log_closest_bt_device) {
        ESP_LOG_BUFFER_HEX(TAG, (*device).mac, BT_MAC_LENGTH);
        ESP_LOGI(TAG, "Device rssi: %d", (*device).rssi);
    }
}


static void on_bt_device_found(struct bt_scan_device_t device) {
    if (device.rssi > closest_bt_device.rssi) {
        closest_bt_device.rssi = device.rssi;
        str_copy(device.mac, closest_bt_device.mac, BT_MAC_LENGTH);
        log_bt_device(&closest_bt_device);
    } else if (str_are_equal_to(closest_bt_device.mac, device.mac, BT_MAC_LENGTH-1) > -1){
        closest_bt_device.rssi = device.rssi;
        log_bt_device(&closest_bt_device);
    }
}


static void bt_device_scan_result_callback(struct ble_scan_result_evt_param param) {
    struct bt_scan_device_t scan_device;
    scan_device.rssi = param.rssi;
    for (int i = 0; i < BT_MAC_LENGTH; i++) {
        scan_device.mac[i] = param.bda[i];
    }

    if (device_found_callback!=NULL) device_found_callback(scan_device);
    else {
        ESP_LOG_BUFFER_HEX(TAG, scan_device.mac, BT_MAC_LENGTH);
        ESP_LOGI(TAG, "Device rssi: %d", scan_device.rssi);
    }
}

static void bt_scan_result_evt_callback(struct ble_scan_result_evt_param scan_result) {
    switch (scan_result.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            bt_device_scan_result_callback(scan_result);
            break;
        case ESP_GAP_SEARCH_DISC_RES_EVT:
            ESP_LOGI(TAG, "ESP_GAP_SEARCH_DISC_RES_EVT");
            break;
        case ESP_GAP_SEARCH_DISC_BLE_RES_EVT:
            ESP_LOGI(TAG, "ESP_GAP_SEARCH_DISC_BLE_RES_EVT");
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            ESP_LOGI(TAG, "Scan complete, %i devices found", scan_result.num_resps);
            break;

        default: 
            break;
    } 
}
 
static void esp_gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_err_t err;
    switch (event) {
        case ESP_GAP_BLE_SCAN_RESULT_EVT: 
            bt_scan_result_evt_callback(param->scan_rst);
            break;   
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: 
            esp_ble_gap_start_scanning(0);
            break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) 
                ESP_LOGE(TAG, "Start scan failed: %s", esp_err_to_name(err));
            break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) 
                ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
            else 
                ESP_LOGI(TAG, "Stopped scan successfully");
            break;
        default: 
            break;
    }
}

void ble_scanner_task() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_err_t status = esp_ble_gap_register_callback(esp_gap_callback);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
    }

    esp_ble_scan_params_t ble_scan_params = {
        .scan_type          = BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval      = 0x50,
        .scan_window        = 0x30,
        .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
    };
    set_bt_device_found_callback(on_bt_device_found);
    esp_ble_gap_set_scan_params(&ble_scan_params);
}

void on_bt_command(const char * arguments) {
     if (str_starts_with(arguments, "log") > -1) {
        should_log_closest_bt_device = 1;
    } else if (str_starts_with(arguments, "nolog") > -1) {
        should_log_closest_bt_device = 0;
    } else if (str_starts_with(arguments, "scanstart") > -1) {
        esp_ble_gap_start_scanning(0);
    } else if (str_starts_with(arguments, "scanstop") > -1) {
        esp_ble_gap_stop_scanning();
    } else ESP_LOGW(TAG, "Unknown bt arguments: %s", arguments);
}

