#pragma once

#include <furi.h>

typedef struct MarauderUart MarauderUart;

MarauderUart* marauder_uart_alloc(void);
void marauder_uart_free(MarauderUart* uart);

/* Send raw bytes as-is (no newline appended) */
void marauder_uart_send(MarauderUart* uart, const char* data);

/* Send a CLI command followed by '\n', matching ESP32 Marauder's line-delimited protocol */
void marauder_uart_send_line(MarauderUart* uart, const char* line);

/* Non-blocking read of whatever bytes have arrived since the last call */
size_t marauder_uart_receive(MarauderUart* uart, uint8_t* buffer, size_t max_len);
