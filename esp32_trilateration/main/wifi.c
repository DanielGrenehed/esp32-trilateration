#include "wifi.h"
#include "esp_log.h"

static const char * TAG = "WIFI";

void on_wf_command(const char *arguments) {
    ESP_LOGW(TAG, "unknown argument: %s", arguments);
}