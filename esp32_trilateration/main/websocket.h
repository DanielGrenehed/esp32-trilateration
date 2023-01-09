#ifndef TRILAT_WEBSOCKET_H
#define TRILAT_WEBSOCKET_H
#include <stdint.h>
void ws_connect();

void ws_disconnect();

uint8_t ws_is_connected();

void ws_set_server(const char*, int);

void ws_set_on_message_callback(void (*callback)(const char*));

void ws_send(const char* data, int len);

void on_ws_command(const char*);

#endif /* TRILAT_WEBSOCKET_H*/