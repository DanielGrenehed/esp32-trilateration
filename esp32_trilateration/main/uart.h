
/*
s
    1. Set Communication Parameters - Setting baud rate, data bits, stop bits, etc.
    2. Set Communication Pins - Assigning pins for connection to a device
    3. Install Drivers - Allocating ESP32’s resources for the UART driver
    4. Run UART Communication - Sending/receiving data
    5. Use Interrupts - Triggering interrupts on specific communication events
    6. Deleting a Driver - Freeing allocated resources if a UART communication is no longer required

*/
#ifndef TRILAT_UART_H
#define TRILAT_UART_H
#include "sdkconfig.h"

#define TRILAT_UART_TXD (CONFIG_TRILAT_UART_TXD)
#define TRILAT_UART_RXD (CONFIG_TRILAT_UART_RXD)
#define TRILAT_UART_RTS (UART_PIN_NO_CHANGE)
#define TRILAT_UART_CTS (UART_PIN_NO_CHANGE)

#define TRILAT_UART_PORT_NUM (CONFIG_TRILAT_UART_PORT_NUM)
#define TRILAT_UART_BAUD_RATE (CONFIG_TRILAT_UART_BAUD_RATE)
#define TRILAT_TASK_STACK_SIZE (CONFIG_TRILAT_TASK_STACK_SIZE)

void uart_cli_task(void *arg);

void uart_cli_set_command_callback(void (*)(const char*));

void uart_write_line(const char*);

#endif /* TRILAT_UART_H */