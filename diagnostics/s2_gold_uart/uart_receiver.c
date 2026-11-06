/* UART receiver intentionally isolated from proven WUP-028 USB implementation.
 * LOLIN32 Lite Bluepad32: GPIO17 TX -> ESP32-S2 GPIO16 RX, shared GND.
 * Protocol: A5 5A + 18 bytes, CRC8 over first 19 bytes, 115200 8N1.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "pad_gc.h"

#define RX_UART UART_NUM_1
#define RX_GPIO 16
#define RX_BAUD 115200
#define RX_TIMEOUT_US 150000LL

static const char *TAG = "wup028_uart";
static portMUX_TYPE port_lock = portMUX_INITIALIZER_UNLOCKED;
static uint8_t latest_port[9] = {0x10, 0, 0, 128, 128, 128, 128, 0, 0};
static bool last_online = false;

/* Called once a millisecond by the same callback as the proven USB diagnostic.
 * Critical section only protects a 9-byte copy, no UART or other blocking work.
 */
void uart_receiver_snapshot(uint8_t port[9]) {
    portENTER_CRITICAL(&port_lock);
    memcpy(port, latest_port, 9);
    portEXIT_CRITICAL(&port_lock);
}

static void submit(const uint8_t port[9]) {
    portENTER_CRITICAL(&port_lock);
    memcpy(latest_port, port, 9);
    portEXIT_CRITICAL(&port_lock);
}

static void uart_task(void *arg) {
    (void)arg;
    uint8_t frame[PAD_PACKET_SIZE];
    size_t pos = 0;
    int64_t last_packet_us = esp_timer_get_time();
    bool online = false;
    uint8_t neutral[9];
    gc_neutral(neutral);
    for (;;) {
        uint8_t byte = 0;
        const int n = uart_read_bytes(RX_UART, &byte, 1, pdMS_TO_TICKS(5));
        if (n == 1) {
            if (pos == 0 && byte != PAD_SYNC0) continue;
            if (pos == 1 && byte != PAD_SYNC1) {
                pos = (byte == PAD_SYNC0) ? 1 : 0;
                continue;
            }
            frame[pos++] = byte;
            if (pos == PAD_PACKET_SIZE) {
                if (pad_valid(frame)) {
                    uint8_t gc_port[9];
                    gc_from_pad_packet(frame, gc_port);
                    submit(gc_port);
                    last_packet_us = esp_timer_get_time();
                    online = (frame[12] & PAD_CONNECTED) != 0;
                    if (last_online != online) {
                        ESP_LOGI(TAG, "Controller %s", online ? "connected" : "disconnected");
                        last_online = online;
                    }
                }
                pos = 0;
            }
        }
        if (esp_timer_get_time() - last_packet_us > RX_TIMEOUT_US) {
            if (online || last_online) {
                submit(neutral);
                ESP_LOGW(TAG, "UART timeout: neutral GC state");
                online = false;
                last_online = false;
            }
            last_packet_us = esp_timer_get_time();
            /* Also discard a partial packet when sender stops. */
            pos = 0;
        }
    }
}

void uart_receiver_start(void) {
    const uart_config_t config = {
        .baud_rate = RX_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install(RX_UART, 1024, 0, 0, NULL, 0);
    if (err == ESP_OK) err = uart_param_config(RX_UART, &config);
    if (err == ESP_OK) err = uart_set_pin(RX_UART, UART_PIN_NO_CHANGE, RX_GPIO,
                                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed (%s); USB remains active", esp_err_to_name(err));
        return; /* no ESP_ERROR_CHECK() / restart in USB path */
    }
    if (xTaskCreate(uart_task, "pad_uart", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "UART task creation failed; USB remains active");
        return;
    }
    ESP_LOGI(TAG, "UART receiver ready: 115200 8N1 RX GPIO16");
}
