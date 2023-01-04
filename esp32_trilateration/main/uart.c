
#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart.h"


static const char *TAG = "UART TEST";
#define BUF_SIZE (1024)

static void (*command_callback)(const char*) = NULL;
void uart_cli_set_command_callback(void (*callback)(const char*)) {
    command_callback = callback;
}

static void uart_write_string(const char *chr) {
    int len = 0;
    char * end = chr;
    while ((*end++) != 0) len++;
    uart_write_bytes(TRILAT_UART_PORT_NUM, chr, len); 
}

static void uart_newline() {
    static const char * end = "\n\r";
    uart_write_string(end);
}

static void uart_cr() {
    static const char* cr = "\r";
    uart_write_string(cr);
}

void uart_write_line(const char* line) {
    uart_write_string(line);
    uart_newline();
}

static int input_buffer_end = 0;
static char input_buffer[1024];

static void add_to_buffer(char chr) {
    if (chr == 0b1000 && input_buffer_end > 0) {
        input_buffer[--input_buffer_end] = 0;
    } else {
        input_buffer[input_buffer_end++] = chr;
        input_buffer[input_buffer_end] = 0;
    }
}

static void clear_buffer() {
    input_buffer_end = 0;
    input_buffer[input_buffer_end] = 0;
}

static void evaluate_input_buffer() {
    if (command_callback != NULL) {
        command_callback(input_buffer);
    }
}

void uart_cli_task(void *arg) {
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = TRILAT_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int interrupt_allocation_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(TRILAT_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, interrupt_allocation_flags));
    ESP_ERROR_CHECK(uart_param_config(TRILAT_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(TRILAT_UART_PORT_NUM, TRILAT_UART_TXD, TRILAT_UART_RXD, TRILAT_UART_RTS, TRILAT_UART_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    ESP_LOGI(TAG, "Staring UART CLI");
    int input_length = 0;
    while (1) {
        int len = uart_read_bytes(TRILAT_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        
        
        if (len) {
            input_length += len;

            if (data[len-1] == 0b1101) { // On Enter
                
                //clear line with space
                uart_cr();
                for (int i = 0; i < input_length; i++) data[i] = 0x20;
                data[input_length] = 0;
                uart_write_string((const char*)data);
                uart_cr();  
                
                // write input buffer, what is actually going to be processed
                uart_write_string(input_buffer);
                uart_newline();
                
                // evaluate string, process input
                evaluate_input_buffer();

                // clear input
                clear_buffer();
                input_length = 0;
            } else {
                // Add input to buffers and echo back to serial
                for (int i = 0; i < len; i++) add_to_buffer(data[i]);
                data[len] = '\0';
                uart_write_string((const char*) data);
            }
        } 
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
}