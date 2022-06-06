/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_websocket_client.h"

#include "laso_util.h"
#include "max7219.h"
#include "breakbeam.h"
#include "websocket.h"


#define PIN_NUM_MISO  0 // NOT_USED
#define PIN_NUM_MOSI 23 // ORANGE
#define PIN_NUM_CLK  18 // BROWN
#define PIN_NUM_CS    5 // RED

spi_device_handle_t spi;

void runCounter(void* arg) {
    char numstr[5];
    int n = 1;
    while(1) {
        sprintf(numstr, "%04X", n++);
        max7219_sendText(spi, numstr);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    printf("PIN_NUM_MOSI: %d\n", PIN_NUM_MOSI);
    printf("PIN_NUM_CLK:  %d\n", PIN_NUM_CLK);
    printf("PIN_NUM_CS:   %d\n", PIN_NUM_CS);

    spi = max7219_initSpiBus(PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

    max7219_sendNumber(spi, 0);

    max7219_init(spi);
    max7219_sendText(spi, "BOOT");

    static char ip4_addr_str[16];
    esp_websocket_client_handle_t websocket_client_handle = websocket_init(ip4_addr_str);
    printf("%s:%d: ip4_addr_str: %s\n", __FILE__, __LINE__, ip4_addr_str); /* !DEBUG! */ 
   
    breakbeam_start(websocket_client_handle, spi, ip4_addr_str);
    
    max7219_sendText(spi, "PLAY");
    
    // Start counter task with lower priority than the breakbeam
    /* xTaskCreate(runCounter, "runCounter", 2048, NULL, 1, NULL);   */
}
