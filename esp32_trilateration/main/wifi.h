#ifndef TRILAT_WIFI_H
#define TRILAT_WIFI_H

void set_wifi_connect_callback(void (*callback)(void*));
void set_wifi_disconnect_callback(void (*callback)(void*));

int wifi_init();
int is_wifi_connected();

void wifi_connect();
void wifi_disconnect();
void on_wifi_command(const char*);

#endif /* TRILAT_WIFI_H */