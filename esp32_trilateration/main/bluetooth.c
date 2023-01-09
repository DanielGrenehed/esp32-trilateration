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

static void (*device_found_callback)(struct bt_scan_device_t) = NULL;

static char bt_wl_mac[BT_MAC_LENGTH];
enum bt_states {scanning, scanning_closest, scanning_whitelist};
static enum bt_states bt_state = scanning;
static int bt_verbose_log = 0;

void set_bt_device_found_callback(void (*callback)(struct bt_scan_device_t)) {
    device_found_callback = callback;
}

static void log_bt_device(struct bt_scan_device_t *device) {
    ESP_LOG_BUFFER_HEX(TAG, (*device).mac, BT_MAC_LENGTH);
    ESP_LOGI(TAG, "Address type: %i, device type: %d, rssi: %d", device->addr_type&0b11, (device->addr_type>>2)&0b11, (*device).rssi);
}

/*

*/

#define BT_SCAN_BUFFER_SIZE 50
static struct bt_scan_device_t bt_scan_buffer[BT_SCAN_BUFFER_SIZE];
static int bt_scan_buffer_end = 0;
static int bt_scan_buffer_ptr = 0;
static int bt_rssi_filter = -127;

static int is_in_scan_buffer(const char* mac) {
    for (int i = 0; i < bt_scan_buffer_end; i++) {
        if (str_are_equal_to(mac, bt_scan_buffer[i].mac, BT_MAC_LENGTH) == BT_MAC_LENGTH) return i;
    }
    return -1;
}

static void set_in_scan_buffer(struct bt_scan_device_t *device) {
    str_copy(device->mac, bt_scan_buffer[bt_scan_buffer_ptr].mac, BT_MAC_LENGTH);
    bt_scan_buffer[bt_scan_buffer_ptr].addr_type = device->addr_type;
    bt_scan_buffer[bt_scan_buffer_ptr].rssi = device->rssi;
}

static void show_scan_buffer() {
    for (int i = 0; i< bt_scan_buffer_end;i++) {
        //ESP_LOGI(TAG, "Buffer pos: %d", i);
        log_bt_device(&bt_scan_buffer[i]);
    }
    ESP_LOGI(TAG, "Buffer size: %d, next entry: %d, rssi filter: x > %d", bt_scan_buffer_end, bt_scan_buffer_ptr, bt_rssi_filter); 
}

static int add_to_buffer(struct bt_scan_device_t *device) {
    if (device->rssi <= bt_rssi_filter) return -1;
    int pos = is_in_scan_buffer(device->mac); 
    if (pos == -1) { // device does not exist in buffer
        set_in_scan_buffer(device);
        pos = bt_scan_buffer_ptr;
        if (++bt_scan_buffer_ptr < BT_SCAN_BUFFER_SIZE) {
            if (bt_scan_buffer_ptr > bt_scan_buffer_end) bt_scan_buffer_end = bt_scan_buffer_ptr;
        } else {
            bt_scan_buffer_ptr = 0;
            bt_scan_buffer_end = BT_SCAN_BUFFER_SIZE;
        }
        return pos;
    } // device does exist in buffer, update rssi, return -1, adding failed
    bt_scan_buffer[pos].rssi = device->rssi; 
    return -1;
}

static int get_closest_device_buffer_index() {
    int closest = 0;
    for (int i = 0; i < bt_scan_buffer_end; i++) {
        if (bt_scan_buffer[i].rssi > bt_scan_buffer[closest].rssi) {
            closest = i;
        }
    }
    return closest;
}

static void on_bt_device_found(struct bt_scan_device_t device) {
    switch (bt_state) {
        case scanning:
            if (add_to_buffer(&device) != -1 && bt_verbose_log) log_bt_device(&device);
        break;
        case scanning_whitelist:
            if (str_are_equal_to(bt_wl_mac, device.mac, BT_MAC_LENGTH-1) > -1) ESP_LOGI(TAG, "Device rssi: %d", device.rssi);
            break;
        default:
            break;
    }
    
}


static void bt_device_scan_result_callback(struct ble_scan_result_evt_param param) {
    struct bt_scan_device_t scan_device;
    scan_device.rssi = param.rssi;
    scan_device.addr_type = param.ble_addr_type | (param.dev_type << 2);
    str_copy((const char*)param.bda, scan_device.mac, BT_MAC_LENGTH);

    if (device_found_callback!=NULL) device_found_callback(scan_device);
    else {
        ESP_LOG_BUFFER_HEX(TAG, scan_device.mac, BT_MAC_LENGTH);
        ESP_LOGI(TAG, "Device rssi: %d", scan_device.rssi);
    }
    //ESP_LOG_BUFFER_CHAR(TAG, param.ble_adv, param.adv_data_len);
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

static int scan_params_set = 0;

static volatile esp_ble_scan_params_t ble_scan_params = {
        .scan_type          = BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval      = 0x50,
        .scan_window        = 0x30,
        .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
};

static void clear_scan_buffer() {
    for (int i = 0; i < BT_SCAN_BUFFER_SIZE; i++) {
        bt_scan_buffer[i].addr_type = 0;
        bt_scan_buffer[i].rssi = -127;
    }
}

void ble_scanner_task() {
    clear_scan_buffer();

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
    set_bt_device_found_callback(on_bt_device_found);
}

void on_bt_command(const char * arguments) {
    int pos;
    if (str_starts_with(arguments, "scan") > -1) {
        if (scan_params_set) esp_ble_gap_start_scanning(0);
        else {
            esp_ble_gap_set_scan_params(&ble_scan_params); 
            scan_params_set = 1;
        }
    } 
    else if (str_starts_with(arguments, "noscan") > -1) esp_ble_gap_stop_scanning();
    else if ((pos = str_starts_with(arguments, "sw")) > -1) {
        int success = str_parse_hex(arguments, BT_MAC_LENGTH*2, bt_wl_mac);
        if (success == BT_MAC_LENGTH*2) bt_state = scanning_whitelist;
        else ESP_LOGW(TAG, "Could not get mac from: %s", arguments+pos);
    } 
    else if (str_starts_with(arguments, "b") > -1) show_scan_buffer();
    else if (str_starts_with(arguments, "lv") > -1) bt_verbose_log = 1; // log verbose
    else if (str_starts_with(arguments, "ll") > -1) bt_verbose_log = 0;// log less
    else if (str_starts_with(arguments, "sn") > -1) bt_state=scanning;
    else if ((pos = str_starts_with(arguments, "srf")) > -1) bt_rssi_filter = str_parse_int(arguments+pos);
    else if (str_starts_with(arguments, "c") > -1) { // print closest device info
        int closest = get_closest_device_buffer_index();
        log_bt_device(&bt_scan_buffer[closest]);
    } else ESP_LOGW(TAG, "Unknown bt arguments: %s", arguments);
}

