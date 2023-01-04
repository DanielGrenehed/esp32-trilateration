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

static const char* TAG = "BT";

static void esp_gap_callback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_err_t err;
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Start scan failed: %s", esp_err_to_name(err));
            } break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) {
                case ESP_GAP_SEARCH_INQ_RES_EVT:
                    ESP_LOG_BUFFER_HEX(TAG, scan_result->scan_rst.bda, 6);
                    ESP_LOGI(TAG, "Device found: rssi: %d", scan_result->scan_rst.rssi);
                    break;
                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    ESP_LOGI(TAG, "Scan complete, %i devices found", scan_result->scan_rst.num_resps);
                    break;
                default: break;
            }break;   
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
            else ESP_LOGI(TAG, "Stopped scan successfully");
            break;
        default: break;
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

    esp_err_t status;
    if ((status = esp_ble_gap_register_callback(esp_gap_callback)) != ESP_OK) {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
    }
esp_ble_scan_params_t ble_scan_params = {
    .scan_type          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
};
    esp_ble_gap_set_scan_params(&ble_scan_params);
}