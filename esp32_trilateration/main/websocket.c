#include "websocket.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "str.h"

#define WEBSOCKET_MESSAGE_BUFFER_SIZE CONFIG_TRILAT_WEBSOCKET_MESSAGE_BUFFER_SIZE
#define WEBSOCKET_URI_MAX_LENGTH CONFIG_TRILAT_WEBSOCKET_URI_MAX_LENGTH
static const char *TAG = "WebSocket";

static esp_websocket_client_config_t websocket_config = {};
static esp_websocket_client_handle_t websocket_client = NULL;


static void (*on_message_callback)(const char*);
void ws_set_on_message_callback(void (*callback)(const char*)) {
    on_message_callback = callback;
}

static char websocket_message_buffer[WEBSOCKET_MESSAGE_BUFFER_SIZE];

static void websocket_data_event_handler(esp_websocket_event_data_t* data) {
    if (on_message_callback != NULL) {
        if (data->data_len < WEBSOCKET_MESSAGE_BUFFER_SIZE-1) {
            str_copy(data->data_ptr, websocket_message_buffer, data->data_len);
            websocket_message_buffer[data->data_len] = 0;
        } else {
            str_copy(data->data_ptr, websocket_message_buffer, WEBSOCKET_MESSAGE_BUFFER_SIZE-2);
            websocket_message_buffer[WEBSOCKET_MESSAGE_BUFFER_SIZE-1] = 0;
        }
        on_message_callback(websocket_message_buffer);
    } else {
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
    }
}

static void websocket_event_handler(void *args, esp_event_base_t base, int32_t event_id, void * event_data) {
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            break;
        case WEBSOCKET_EVENT_DATA:
            esp_websocket_event_data_t* data = (esp_websocket_event_data_t*) event_data;
            websocket_data_event_handler(data);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
        default:
            break;
    }
}


void ws_connect() {
    if (websocket_config.uri == NULL) return;
    websocket_client = esp_websocket_client_init(&websocket_config);
    esp_websocket_register_events(websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*)websocket_client);
    esp_websocket_client_start(websocket_client);
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

void ws_send(const char* data, int len) {
    if (esp_websocket_client_is_connected(websocket_client)) {
        esp_websocket_client_send_text(websocket_client, data, len, portMAX_DELAY);
    }
}
static char ws_uri[WEBSOCKET_URI_MAX_LENGTH];
static void set_uri(const char *uri) {
    str_copy_str(uri, ws_uri);
    websocket_config.uri = ws_uri;
}


void on_ws_command(const char* arguments) {
    int pos = 0;
    if (str_starts_with(arguments, "c") > -1) ws_connect();
    else if (str_starts_with(arguments, "d") > -1) ws_disconnect();
    else if ((pos = str_starts_with(arguments, "u")) > -1) set_uri(arguments+pos+1);
    else ESP_LOGW(TAG, "unknown argument: %s", arguments);
}