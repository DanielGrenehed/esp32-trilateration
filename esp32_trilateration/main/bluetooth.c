#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "bluetooth.h"
#include "str.h"

static const char* TAG = "BT";

static void (*device_found_callback)(struct bt_scan_device_t) = NULL;
void (*bt_scan_direct_callback)(struct bt_scan_device_t) = NULL;
static void (*bt_log_output)(const char*, int, int) = NULL;

static char bt_wl_mac[BT_MAC_LENGTH];
enum bt_states {off, scanning, scanning_closest, scanning_whitelist, scanning_direct, advertising};
static enum bt_states bt_state = off;
static int bt_verbose_log = 0;

static void set_bt_device_found_callback(void (*callback)(struct bt_scan_device_t)) {
    device_found_callback = callback;
}


void set_bt_log_output(void (*callback)(const char*, int, int)) {
    bt_log_output = callback;
}

void set_bt_scan_direct_callback(void (*callback)(struct bt_scan_device_t)) {
    bt_scan_direct_callback = callback;
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
        case scanning_direct:
            if (bt_scan_direct_callback != NULL) {
                bt_scan_direct_callback(device); 
            }
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


 static int scan_params_set = 0;

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type          = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
};

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST,
};

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
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&ble_adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) 
                ESP_LOGE(TAG, "Advertising start failed: %s", esp_err_to_name(err)); 
        default: 
            break;
    }
}



static void clear_scan_buffer() {
    for (int i = 0; i < BT_SCAN_BUFFER_SIZE; i++) {
        bt_scan_buffer[i].addr_type = 0;
        bt_scan_buffer[i].rssi = -127;
    }
    bt_scan_buffer_end = 0;
    bt_scan_buffer_ptr = 0;
}

void bt_send_scan_buffer(void (*destination)(const char*, int)) {
    if (bt_scan_buffer_end == 0) {
        ESP_LOGW(TAG, "No scan data collected");
        return;
    }
    destination((const char*)bt_scan_buffer, bt_scan_buffer_end*SCAN_DEVICE_STRUCT_SIZE);
    clear_scan_buffer();
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

static void send_mac() {
    char body[BT_MAC_LENGTH+1] = "b";
    esp_read_mac((uint8_t*)body+1, ESP_MAC_BT);
    if (bt_log_output != NULL) bt_log_output(body, BT_MAC_LENGTH+1, 0);
    ESP_LOG_BUFFER_HEX(TAG, body+1, BT_MAC_LENGTH);
}
static void stop_scan() {
    if (bt_state == scanning || bt_state == scanning_closest || bt_state == scanning_whitelist || bt_state == scanning_direct) {
        esp_ble_gap_stop_scanning();
        bt_state = off;
        ESP_LOGI(TAG, "Scan stopped");
    }
}

static void stop_advertising() {
    if (bt_state == advertising) {
        esp_ble_gap_stop_advertising();
        bt_state = off;
        ESP_LOGI(TAG, "Advertising stopped");
    }
}

static void start_advertising() {
    if (bt_state == scanning || bt_state == scanning_closest || bt_state == scanning_whitelist || bt_state == scanning_direct) stop_scan();
    if (bt_state == off) {
        esp_ble_gap_start_advertising(&ble_adv_params);
        bt_state = advertising;
        ESP_LOGI(TAG, "Started advertising");
    }
}

static void start_scan() {
    if (bt_state == advertising) stop_advertising();
    if (bt_state == scanning || bt_state == scanning_closest || bt_state == scanning_whitelist || bt_state == scanning_direct) return;
    if (scan_params_set) esp_ble_gap_start_scanning(0);
    else {
        esp_ble_gap_set_scan_params(&ble_scan_params); 
        scan_params_set = 1;
    }
    bt_state = scanning;
    ESP_LOGI(TAG, "Started scanning");
}

static void whitelist_mac(const char* arguments) {
    int success = str_parse_hex(arguments, BT_MAC_LENGTH*2, bt_wl_mac);
    if (success == BT_MAC_LENGTH*2) bt_state = scanning_whitelist;
    else ESP_LOGW(TAG, "Could not get mac from: %s", arguments);
}

static void log_closest_device() {
    int closest = get_closest_device_buffer_index();
        log_bt_device(&bt_scan_buffer[closest]);
}

static void set_direct_scanning() {
    start_scan();
    bt_state= scanning_direct;
}

static void set_buffer_scanning() {
    start_scan();
    bt_state = scanning;
}

void on_bt_command(const char * arguments) {
    int pos;
    if (str_starts_with(arguments, "scan") > -1) start_scan();
    else if (str_starts_with(arguments, "noscan") > -1) stop_scan();
    else if (str_starts_with(arguments, "ds") > -1) set_direct_scanning();
    else if (str_starts_with(arguments, "bs") > -1) set_buffer_scanning();
    else if (str_starts_with(arguments, "adv") > -1) start_advertising();
    else if (str_starts_with(arguments, "noadv") > -1) stop_advertising();
    else if ((pos = str_starts_with(arguments, "sw")) > -1) whitelist_mac(arguments+pos); 
    else if (str_starts_with(arguments, "b") > -1) show_scan_buffer();
    else if (str_starts_with(arguments, "lv") > -1) bt_verbose_log = 1; // log verbose
    else if (str_starts_with(arguments, "ll") > -1) bt_verbose_log = 0;// log less
    else if (str_starts_with(arguments, "cb") > -1) clear_scan_buffer();
    else if ((pos = str_starts_with(arguments, "srf")) > -1) bt_rssi_filter = str_parse_int(arguments+pos);
    else if (str_starts_with(arguments, "c") > -1) log_closest_device();
    else if (str_starts_with(arguments, "m") > -1) send_mac(); 
    else ESP_LOGW(TAG, "Unknown argument: %s", arguments);
}

