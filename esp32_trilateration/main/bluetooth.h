#ifndef TRILAT_BLUETOOTH_H
#define TRILAT_BLUETOOTH_H

// Number of bytes in bluetooth mac address
#define BT_MAC_LENGTH 6
#define SCAN_DEVICE_STRUCT_SIZE (2+(1*BT_MAC_LENGTH))

struct bt_scan_device_t{
    int8_t rssi, addr_type;
    char mac[BT_MAC_LENGTH];
};

void set_bt_device_found_callback(void (*callback)(struct bt_scan_device_t));

void set_bt_log_output(void (*callback)(const char*, int, int));

void bt_send_scan_buffer(void (*destination)(const char*, int));

void ble_scanner_task();

void on_bt_command(const char*);

#endif /* TRILAT_BLUETOOTH_H */