#include "websocket.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "str.h"
#include "store.h"

#define WEBSOCKET_MESSAGE_BUFFER_SIZE CONFIG_TRILAT_WEBSOCKET_MESSAGE_BUFFER_SIZE
#define WEBSOCKET_URI_MAX_LENGTH CONFIG_TRILAT_WEBSOCKET_URI_MAX_LENGTH
static const char *TAG = "WebSocket";

static esp_websocket_client_config_t websocket_config = {};
static esp_websocket_client_handle_t websocket_client = NULL;

static uint8_t ws_log_level = 0; // default to not logging
static uint8_t uri_is_set = 0;

static void (*on_message_callback)(const char*);
void ws_set_on_message_callback(void (*callback)(const char*)) {
    on_message_callback = callback;
}
static void (*on_connect_callback)() = NULL;
void ws_set_on_connect_callback(void (*callback)()) {
    on_connect_callback = callback;
}

static char websocket_message_buffer[WEBSOCKET_MESSAGE_BUFFER_SIZE];

static void websocket_data_event_handler(esp_websocket_event_data_t* data) {
    if (on_message_callback != NULL && data->payload_len > 0) {
        if (data->data_len < WEBSOCKET_MESSAGE_BUFFER_SIZE-1) {
            str_copy(data->data_ptr, websocket_message_buffer, data->data_len);
            websocket_message_buffer[data->data_len] = 0;
        } else {
            str_copy(data->data_ptr, websocket_message_buffer, WEBSOCKET_MESSAGE_BUFFER_SIZE-2);
            websocket_message_buffer[WEBSOCKET_MESSAGE_BUFFER_SIZE-1] = 0;
        }
        on_message_callback(websocket_message_buffer);
    } else {
        if (ws_log_level) {
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
            ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
            ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
            ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
        }
        
    }
}

static void ws_on_connect() {
   if (on_connect_callback!=NULL) on_connect_callback();
}

static void ws_on_disconnect() {}

static void websocket_event_handler(void *args, esp_event_base_t base, int32_t event_id, void * event_data) {
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            if (ws_log_level) ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            ws_on_connect();
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            if (ws_log_level) ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            ws_on_disconnect();
            break;
        case WEBSOCKET_EVENT_DATA:
            esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;
            websocket_data_event_handler(data);
            break;
        case WEBSOCKET_EVENT_ERROR:
            if (ws_log_level) ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
        default:
            break;
    }
}

static char ws_uri[WEBSOCKET_URI_MAX_LENGTH];
static void set_uri(const char *uri) {
    str_copy_str(uri, ws_uri);
    websocket_config.uri = ws_uri;
    store_set_str("ws_uri", ws_uri);
}

void ws_connect() {
    if (ws_is_connected()) return;
    websocket_client = esp_websocket_client_init(&websocket_config);
    if (!uri_is_set) {
        if (store_get_str("ws_uri", ws_uri, WEBSOCKET_URI_MAX_LENGTH)) {
            websocket_config.uri = ws_uri;
            uri_is_set = 1;
        }
    }
    if (websocket_config.uri == NULL) return;
    else {
        if (esp_websocket_client_set_uri(websocket_client, ws_uri)!= ESP_OK) return;
    }
    
    ESP_LOGI(TAG, "client created");
    esp_websocket_register_events(websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*)websocket_client);
    ESP_ERROR_CHECK(esp_websocket_client_start(websocket_client));
}

void ws_disconnect() {
    esp_websocket_client_stop(websocket_client);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(websocket_client);
}

uint8_t ws_is_connected() {
    if (websocket_client == NULL) return 0;
    return esp_websocket_client_is_connected(websocket_client);
}

void ws_send_text(const char* data, int len) {
    if (esp_websocket_client_is_connected(websocket_client)) {
        esp_websocket_client_send_text(websocket_client, data, len, portMAX_DELAY);
    }
}

void ws_send_blob(const char* data, int len) {
    if (esp_websocket_client_is_connected(websocket_client)) {
        esp_websocket_client_send_bin(websocket_client, data, len, portMAX_DELAY);
    }
}

void ws_send(const char* data, int len, int encoded_text) {
    if (encoded_text) ws_send_text(data, len);
    else ws_send_blob(data, len);
}

static void send_uri() {
    ws_send_text(websocket_config.uri, WEBSOCKET_URI_MAX_LENGTH);
}

void on_ws_command(const char* arguments) {
    int pos = 0;
    if (str_starts_with(arguments, "c") > -1) ws_connect();
    else if (str_starts_with(arguments, "d") > -1) ws_disconnect();
    else if ((pos = str_starts_with(arguments, "u")) > -1) set_uri(arguments+pos+1);
    else if (str_starts_with(arguments, "lv") > -1) ws_log_level = 1;
    else if (str_starts_with(arguments, "ll") > -1) ws_log_level = 0;
    else if (str_starts_with(arguments, "i") > -1) send_uri();
    else ESP_LOGW(TAG, "unknown argument: %s", arguments);
}