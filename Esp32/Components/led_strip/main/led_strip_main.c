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

static const char *TAG = "example";

#define RMT_TX_CHANNEL_ONE RMT_CHANNEL_0

#define GPIO_OUTPUT_IO_0 (39)
// #define GPIO_OUTPUT_IO_40 (40)

#define EXAMPLE_CHASE_SPEED_MS (10)


void app_main(void)
{
    //uint32_t red = 50;
    //int fadingred = 255;
    uint32_t red = 0;
    uint32_t blue = 0;
    printf("Starting main!!!!!!!!!!!!\n");

    rmt_config_t config_one = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO_ONE, RMT_TX_CHANNEL_ONE);
    // set counter clock to 40MHz
    config_one.clk_div = 2;
    printf("main:-1\n");
    ESP_ERROR_CHECK(rmt_config(&config_one));
    printf("main:0\n");
    ESP_ERROR_CHECK(rmt_driver_install(config_one.channel, 0, 0));
    printf("main:1\n");
    // install ws2812 driver
    led_strip_config_t strip_config_one = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config_one.channel);
    led_strip_t *strip_one = led_strip_new_rmt_ws2812(&strip_config_one);
    printf("main:2\n");
    if (!strip_one) {
        ESP_LOGE(TAG, "install first WS2812 driver failed");
    }

    ESP_ERROR_CHECK(strip_one->clear(strip_one, 100));
    printf("main:3\n");
    

    //fadingred = red;
    //for (int green = 255; green >= 0 ; green -= 15) {
    //    printf("in main loop\n");
    //while (true) {
        //if (green < 100) {
        //    red += 40;
        //}
        for (int i = 0; i < 44; i++) {
            printf("in led loop\n");
            //led_strip_hsv2rgb(10, 100, 100, &red, &green, &blue);
                // Write RGB values to strip driver
                printf("led: %d\n", i);
                printf("buffers: %d\n", 4 - (i % 2)); 
            ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i,0, 255, 0,  4 - (i % 2)));
            printf("pixel set\n");
            // Flush RGB values to LEDs 
        }
        ESP_ERROR_CHECK(strip_one->refresh(strip_one, 100));
        printf("pixel refresh\n");
    //}
    vTaskDelay(15);
}