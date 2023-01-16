#include "store.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "STORE";

static nvs_handle_t store_handle;

void store_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    if (nvs_open("store",NVS_READWRITE, &store_handle) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get store handle");
        store_handle = 0;
    }
}

void store_deinit() {
    nvs_close(store_handle);
}

void store_erase_all() {
    ESP_ERROR_CHECK(nvs_flash_erase);
}

int store_get_str(const char* key, char* out, int length) {
    if (store_handle == 0) return 0;
    unsigned int read = length;
    if (nvs_get_str(store_handle, key, out, &read) != ESP_OK) {
        ESP_LOGW(TAG, "Unable to get key: %s", key);
        return 0;
    } 
    return read>0?read:1;
}

int store_set_str(const char* key, const char* value) {
    if (store_handle == 0) return 0;
    if (nvs_set_str(store_handle, key, value) != ESP_OK) {
        ESP_LOGW(TAG, "Unable to set key: %s", key);
        return 0;
    }
    if (nvs_commit(store_handle) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to commit changes to key: %s", key);
        return 0;
    }
    return 1;
}