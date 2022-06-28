/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "driver/gpio.h"
#include "7seg.h"

static const char *TAG = "example";

#define RMT_TX_CHANNEL_ONE RMT_CHANNEL_0

#define EXAMPLE_STRIP_LED_NUMBER 44
#define EXAMPLE_RMT_TX_GPIO_ONE 35

uint32_t red = 0;
uint32_t green = 0;
uint32_t blue = 0;


void app_main(void)
{
    shownumber(888);

    rmt_config_t config_one = RMT_DEFAULT_CONFIG_TX(EXAMPLE_RMT_TX_GPIO_ONE, RMT_TX_CHANNEL_ONE);
    // set counter clock to 40MHz
    config_one.clk_div = 2;
   
    ESP_ERROR_CHECK(rmt_config(&config_one));
    
    ESP_ERROR_CHECK(rmt_driver_install(config_one.channel, 0, 0));
    
    // install ws2812 driver
    led_strip_config_t strip_config_one = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config_one.channel);
    led_strip_t *strip_one = led_strip_new_rmt_ws2812(&strip_config_one);
    
    if (!strip_one) {
        ESP_LOGE(TAG, "install first WS2812 driver failed");
    }

    ESP_ERROR_CHECK(strip_one->clear(strip_one, 100));
    printf("main:3\n");
    
    for (int c = 0; c < 5; c++) {
        red = 0;
        green = 0;
        blue = 0;
        switch(c) {
            case 1 :
                red = 80;
                break;
            case 2 :
                green = 80;
                break;
            case 3 :
                blue = 80;
                break;
        }
        for (int i = 0; i < EXAMPLE_STRIP_LED_NUMBER; i++) {
            printf("in led loop\n");
            // led_strip_hsv2rgb(10, 100, 100, &red, &green, &blue);
            // Write RGB values to strip driver
            ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i, red, green, blue,  4 - (i % 2)));
            printf("pixel set\n");
            // Flush RGB values to LEDs 
        }
        ESP_ERROR_CHECK(strip_one->refresh(strip_one, 100));
        vTaskDelay(100);
    }
    
    vTaskDelay(15);
}
