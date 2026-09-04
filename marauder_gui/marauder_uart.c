#include "marauder_uart.h"

#include <furi_hal_serial_control.h>
#include <furi_hal_serial.h>
#include <expansion/expansion.h>
#include <string.h>
#include <stdlib.h>

/* Flipper GPIO 13 (TX) / 14 (RX), matching ESP32 Marauder's Serial.begin(115200) on UART0 */
#define MARAUDER_UART_BAUD 115200
#define MARAUDER_UART_RX_STREAM_SIZE 2048

struct MarauderUart {
    FuriHalSerialHandle* serial_handle;
    FuriStreamBuffer* rx_stream;
    Expansion* expansion;
};

static void marauder_uart_rx_callback(
    FuriHalSerialHandle* handle,
    FuriHalSerialRxEvent event,
    void* context) {
    MarauderUart* uart = context;

    if(event & FuriHalSerialRxEventData) {
        uint8_t buf[32];
        size_t len = 0;
        while(furi_hal_serial_async_rx_available(handle) && len < sizeof(buf)) {
            buf[len++] = furi_hal_serial_async_rx(handle);
        }
        if(len > 0) {
            furi_stream_buffer_send(uart->rx_stream, buf, len, 0);
        }
    }
}

MarauderUart* marauder_uart_alloc(void) {
    MarauderUart* uart = malloc(sizeof(MarauderUart));

    uart->rx_stream = furi_stream_buffer_alloc(MARAUDER_UART_RX_STREAM_SIZE, 1);

    /* Expansion module detection listens on the same USART pins (13/14) by default and must
       release them before we can acquire the port, or furi_hal_serial_control_acquire()
       returns NULL. */
    uart->expansion = furi_record_open(RECORD_EXPANSION);
    expansion_disable(uart->expansion);

    uart->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_check(uart->serial_handle);

    furi_hal_serial_init(uart->serial_handle, MARAUDER_UART_BAUD);
    furi_hal_serial_async_rx_start(uart->serial_handle, marauder_uart_rx_callback, uart, false);

    return uart;
}

void marauder_uart_free(MarauderUart* uart) {
    furi_hal_serial_async_rx_stop(uart->serial_handle);
    furi_hal_serial_deinit(uart->serial_handle);
    furi_hal_serial_control_release(uart->serial_handle);

    expansion_enable(uart->expansion);
    furi_record_close(RECORD_EXPANSION);

    furi_stream_buffer_free(uart->rx_stream);

    free(uart);
}

void marauder_uart_send(MarauderUart* uart, const char* data) {
    furi_hal_serial_tx(uart->serial_handle, (const uint8_t*)data, strlen(data));
}

void marauder_uart_send_line(MarauderUart* uart, const char* line) {
    marauder_uart_send(uart, line);
    marauder_uart_send(uart, "\n");
}

size_t marauder_uart_receive(MarauderUart* uart, uint8_t* buffer, size_t max_len) {
    return furi_stream_buffer_receive(uart->rx_stream, buffer, max_len, 0);
}
