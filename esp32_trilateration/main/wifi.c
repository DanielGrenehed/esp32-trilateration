#include "esp_wifi.h"
#include "wifi.h"
#include "str.h"
#include "store.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_MAXIMUM_RETRY CONFIG_TRILAT_WIFI_MAXIMUM_RETRY
#define WIFI_MAX_SSID_LENGTH 32
#define WIFI_MAX_PASSWORD_LENGHT 64
#define WIFI_MAC_LENGTH 6

static const char * TAG = "WIFI";
static int wifi_connected = 0, wifi_active = 0;

static wifi_config_t wifi_config = {.sta = {.ssid = "", .password = "",},};

static void (*on_connect_callback)(void*) = NULL;
void set_wifi_connect_callback(void (*callback)(void*)) {
    on_connect_callback = callback;
}

static void (*on_disconnect_callback)(void*) = NULL;
void set_wifi_disconnect_callback(void (*callback)(void*)) {
    on_disconnect_callback = callback;
}

static void (*wifi_log_output)(const char*, int, int) = NULL;
void set_wifi_log_output(void (*callback)(const char*, int, int)) {
    wifi_log_output = callback;
}

int is_wifi_connected() {
    return wifi_connected;
}

void wifi_connect() {
    wifi_active = 1;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());   
}
void wifi_disconnect() {
    if (wifi_active) {
        wifi_active = 0;
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
}

static int wf_retry_count = 0;

static void wifi_event_handler(int32_t event_id, void* event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START: 
            ESP_LOGI(TAG, "WIFI_START_EVENT");
            ESP_ERROR_CHECK(esp_wifi_connect());
        break;
        case WIFI_EVENT_STA_DISCONNECTED:
            wifi_connected = 0;
            if (wifi_active && wf_retry_count < WIFI_MAXIMUM_RETRY) {
                ESP_ERROR_CHECK(esp_wifi_connect());
                wf_retry_count++;
                ESP_LOGW(TAG, "retrying to connect to AP");
            } else {
                wf_retry_count = 0;
                wifi_active = 0;
                ESP_LOGI(TAG, "Disconnected");
                if (on_disconnect_callback != NULL) on_disconnect_callback(NULL);
                esp_wifi_stop();
            }
        break;
        default:
        break;
    }
}

static void ip_event_handler(int32_t event_id, void* event_data) {
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            wf_retry_count = 0;
            if (on_connect_callback != NULL) {
                wifi_connected = 1;
                on_connect_callback(NULL);
            }
        break;
        default:
        break;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        wifi_event_handler(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        ip_event_handler(event_id, event_data);
    }
}

int wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    esp_event_handler_instance_t instance_any_id, instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    esp_wifi_set_mode(WIFI_MODE_STA);
    
    /*
        Get stored wifi ssid and password and try to connect
    */
    if (store_get_str("wf_ap", (char*)wifi_config.sta.ssid, WIFI_MAX_SSID_LENGTH) &&
        store_get_str("wf_pw", (char*)wifi_config.sta.password, WIFI_MAX_PASSWORD_LENGHT)) wifi_connect();
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return 1;
}

static void send_info() {
    if (wifi_log_output!=NULL) {
        wifi_log_output((char*)wifi_config.sta.ssid, WIFI_MAX_SSID_LENGTH, 1);
    } else {
        ESP_LOGI(TAG, "Wifi ap: %s", wifi_config.sta.ssid);
    }
}

static void send_mac() {
    char body[WIFI_MAC_LENGTH+1] = "w";
    esp_read_mac((uint8_t*)body+1, ESP_MAC_WIFI_STA);
    if (wifi_log_output!=NULL) {
        wifi_log_output(body, WIFI_MAC_LENGTH+1, 0);
    }
    ESP_LOG_BUFFER_HEX(TAG, body+1, WIFI_MAC_LENGTH);
}

static void set_ssid(const char *ssid) {
    str_copy(ssid, (char*)wifi_config.sta.ssid, WIFI_MAX_SSID_LENGTH);
}

static void set_password(const char *password) {
    str_copy(password, (char*)wifi_config.sta.password, WIFI_MAX_PASSWORD_LENGHT);
}

static void save_config() {
    store_set_str("wf_ap", (const char*) wifi_config.sta.ssid);
    store_set_str("wf_pw", (const char*) wifi_config.sta.password); 
}

void on_wifi_command(const char *arguments) {
    int pos = 0;
    if (str_starts_with(arguments, "c") > -1) wifi_connect();
    else if (str_starts_with(arguments, "d") > -1) wifi_disconnect();
    else if ((pos = str_starts_with(arguments, "a")) > -1) set_ssid(arguments+pos+1);
    else if ((pos = str_starts_with(arguments, "p")) > -1) set_password(arguments+pos+1);
    else if (str_starts_with(arguments, "s") > -1) save_config();
    else if (str_starts_with(arguments, "i") > -1) send_info();
    else if (str_starts_with(arguments, "m") > -1) send_mac();
    else ESP_LOGW(TAG, "unknown argument: %s", arguments);
}