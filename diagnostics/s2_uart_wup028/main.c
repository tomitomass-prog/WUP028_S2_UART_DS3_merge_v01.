/* DS3 UART receiver transplanted into the working ESP-IDF WUP-028 USB diagnostic.
 * Expects LOLIN TX -> S2 GPIO16, common GND; 115200 8N1, 20-byte packets.
 * No DS3 USB descriptors, no I2C, no Joybus.
 */
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "usb_descriptors.h"
#include "pad_gc.h"

#define RX_UART UART_NUM_1
#define RX_GPIO 16
#define RX_BAUD 115200
#define PACKET_TIMEOUT_US 120000LL

extern void bridge_usb_poll(void);
extern void bridge_submit_port(const uint8_t port[9]);

static const char *TAG="wup028_uart";
static void usb_report_timer(void *arg) { (void)arg; bridge_usb_poll(); }

static void uart_task(void *arg) {
    (void)arg;
    uint8_t buf[PAD_PACKET_SIZE];
    size_t pos=0;
    int64_t last_valid=0;
    bool online=false;
    uint8_t neutral[9];
    gc_neutral(neutral);
    for (;;) {
        uint8_t c;
        int n=uart_read_bytes(RX_UART, &c, 1, pdMS_TO_TICKS(5));
        if (n==1) {
            if (pos==0 && c!=PAD_SYNC0) continue;
            if (pos==1 && c!=PAD_SYNC1) { pos=(c==PAD_SYNC0)?1:0; continue; }
            buf[pos++]=c;
            if (pos==PAD_PACKET_SIZE) {
                if (pad_valid(buf)) {
                    uint8_t p[9];
                    gc_from_pad_packet(buf,p);
                    bridge_submit_port(p);
                    online=(buf[12]&PAD_CONNECTED)!=0;
                    last_valid=esp_timer_get_time();
                }
                pos=0;
            }
        }
        if (online && esp_timer_get_time()-last_valid>PACKET_TIMEOUT_US) {
            online=false;
            bridge_submit_port(neutral);
        }
    }
}

void app_main(void) {
    tinyusb_config_t cfg=TINYUSB_DEFAULT_CONFIG();
    cfg.descriptor.device=&gcca_device_desc;
    cfg.descriptor.full_speed_config=gcca_configuration_desc;
    cfg.descriptor.string=gcca_string_desc;
    cfg.descriptor.string_count=gcca_string_desc_count;
    ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));

    const uart_config_t uc={
        .baud_rate=RX_BAUD,
        .data_bits=UART_DATA_8_BITS,
        .parity=UART_PARITY_DISABLE,
        .stop_bits=UART_STOP_BITS_1,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,
        .source_clk=UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(RX_UART,1024,0,0,NULL,0));
    ESP_ERROR_CHECK(uart_param_config(RX_UART,&uc));
    ESP_ERROR_CHECK(uart_set_pin(RX_UART,UART_PIN_NO_CHANGE,RX_GPIO,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
    BaseType_t started=xTaskCreate(uart_task,"pad_uart",4096,NULL,5,NULL);
    ESP_ERROR_CHECK(started==pdPASS?ESP_OK:ESP_ERR_NO_MEM);

    const esp_timer_create_args_t tc={.callback=usb_report_timer,.name="wup028_reports"};
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&tc,&timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer,1000));
    ESP_LOGI(TAG,"WUP-028 + UART ready (RX GPIO%d, %d 8N1)",RX_GPIO,RX_BAUD);
}
