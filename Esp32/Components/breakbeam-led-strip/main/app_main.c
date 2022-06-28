/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp_websocket_client.h"
#include "websocket.h"
#include "driver/rmt.h"
#include "led_strip.h"

#include "laso_util.h"
#include "max7219.h"
#include "breakbeam.h"
#include "websocket.h"


#define PIN_NUM_MISO  0 // NOT_USED
#define PIN_NUM_MOSI 23 // ORANGE
#define PIN_NUM_CLK  18 // BROWN
#define PIN_NUM_CS    5 // RED

#define RMT_TX_CHANNEL_ONE RMT_CHANNEL_0
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 11
#define CONFIG_EXAMPLE_RMT_TX_GPIO_ONE 19

static const char *TAG = "BLS_MAIN";

spi_device_handle_t spi;

void runCounter(void* arg) {
    char numstr[5];
    int n = 1;
    while(1) {
        sprintf(numstr, "%04X", n++);
        //max7219_sendText(spi, numstr);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

led_strip_t * install_led_strip() {
    printf("%s:%d: RMT_CHANNEL_0: %d\n", __FILE__, __LINE__, RMT_CHANNEL_0); /* !DEBUG! */ 
    printf("%s:%d: CONFIG_EXAMPLE_RMT_TX_GPIO_ONE: %d\n", __FILE__, __LINE__, CONFIG_EXAMPLE_RMT_TX_GPIO_ONE); /* !DEBUG! */ 

    rmt_config_t config_one = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO_ONE, RMT_TX_CHANNEL_ONE);
    printf("%s:%d: config_one.channel: %d\n", __FILE__, __LINE__, config_one.channel); /* !DEBUG! */

    config_one.clk_div = 2;
    printf("main:-1\n");
    ESP_ERROR_CHECK(rmt_config(&config_one));
    printf("main:0\n");
    ESP_ERROR_CHECK(rmt_driver_install(config_one.channel, 0, 0));


    // install ws2812 driver
    led_strip_config_t strip_config_one = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config_one.channel);
    led_strip_t *strip_one = led_strip_new_rmt_ws2812(&strip_config_one);
    printf("main:2\n");
    if (!strip_one) {
        ESP_LOGE(TAG, "install first WS2812 driver failed");
    }

    ESP_ERROR_CHECK(strip_one->clear(strip_one, 100));
    printf("main:3\n");


    for (int i = 0; i < CONFIG_EXAMPLE_STRIP_LED_NUMBER; i++) {

        // Write RGB values to strip driver
        printf("led: %d\n", i);
        printf("buffers: %d\n", 4 - (i % 2)); 
        ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i, 255, 0, 0,  3));
        printf("pixel set\n");
        
    }
    ESP_ERROR_CHECK(strip_one->refresh(strip_one, 100));

    return strip_one;

}

void app_main(void) {
    printf("PIN_NUM_MOSI: %d\n", PIN_NUM_MOSI);
    printf("PIN_NUM_CLK:  %d\n", PIN_NUM_CLK);
    printf("PIN_NUM_CS:   %d\n", PIN_NUM_CS);

    //spi = max7219_initSpiBus(PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

    //max7219_sendNumber(spi, 0);

    //max7219_init(spi);
    //max7219_sendText(spi, "BOOT");

    led_strip_t *strip_one = install_led_strip();

    static char ip4_addr_str[16];
    esp_websocket_client_handle_t websocket_client_handle = websocket_init(ip4_addr_str);
    printf("%s:%d: ip4_addr_str: %s\n", __FILE__, __LINE__, ip4_addr_str); /* !DEBUG! */ 
   
    breakbeam_start(websocket_client_handle, spi, ip4_addr_str, strip_one);
    
    //max7219_sendText(spi, "PLAY");
    
    // Start counter task with lower priority than the breakbeam
    /* xTaskCreate(runCounter, "runCounter", 2048, NULL, 1, NULL);   */
}
