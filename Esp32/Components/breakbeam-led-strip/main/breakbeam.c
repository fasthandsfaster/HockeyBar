/* FROM e:\udv\esp32\esp-idf-v4.4\examples\peripherals\gpio\generic_gpio\main\gpio_example_main.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "led_strip.h"

#include "websocket.h"
#include "laso_util.h"


#define GPIO_INPUT_IO_A     4
#define GPIO_INPUT_IO_B    17
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_A)) //  | (1ULL<<GPIO_INPUT_IO_B))
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static char macstr[18];

static bool trigger_running = false;

typedef struct TRIGGER_DATA {
    esp_websocket_client_handle_t websocket_client_handle;
    spi_device_handle_t spi_device_handle;
    led_strip_t *strip_one;
    char* ip4_addr_str;
} trigger_data_t;


static void IRAM_ATTR breakbeam_gpio_isr_handler(void* arg) {
    int zero = 0;
    if (trigger_running) {
        xQueueSendFromISR(gpio_evt_queue, &zero, NULL);
    }
}



/*  Skal flyttes */

#define STRIP_COLOR_RED 0
#define STRIP_COLOR_GREEN 1
#define STRIP_COLOR_BLUE 2

void setStripColor(led_strip_t *strip_one, int color) {
    for (int i = 0; i < 11; i++) {

        if (color == STRIP_COLOR_RED) {
            ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i, 255, 0, 0,  3));
        }
        if (color == STRIP_COLOR_GREEN) {
            ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i, 0, 255, 0,  3));
        }
        if (color == STRIP_COLOR_BLUE) {
            ESP_ERROR_CHECK(strip_one->set_pixel(strip_one, i, 0, 0, 255,  3));
        }
    }
    ESP_ERROR_CHECK(strip_one->refresh(strip_one, 100));
}

static void breakbeam_trigger(void* arg) {

    trigger_data_t* trigger_data = (trigger_data_t*)arg;

    char json[100];

    printf("%s:%d: portMAX_DELAY: %d\n", __FILE__, __LINE__, portMAX_DELAY); /* !DEBUG! */
    printf("%s:%d: portTICK_PERIOD_MS: %d\n", __FILE__, __LINE__, portTICK_PERIOD_MS); /* !DEBUG! */

    while(true) {
        int maxWaitForBeamMillis;

        /* Wait for the trigger to be started */
        if(xQueueReceive(gpio_evt_queue, &maxWaitForBeamMillis, portMAX_DELAY)) {

            if (maxWaitForBeamMillis > 0) {
                if (trigger_running) {
                    //max7219_sendText(trigger_data->spi_device_handle, "FIRE");
                    setStripColor(trigger_data->strip_one, STRIP_COLOR_GREEN);

                    long start = xTaskGetTickCount();

                    /* Wait for hit or time out*/
                    int value;
                    if(xQueueReceive(gpio_evt_queue, &value, maxWaitForBeamMillis / portTICK_PERIOD_MS)) {
                        //max7219_sendText(trigger_data->spi_device_handle, "GOAL");
                        setStripColor(trigger_data->strip_one, STRIP_COLOR_BLUE);

                        /* Beam was hit */
                        long stop = xTaskGetTickCount();
                        int hitTimeMillis = (stop-start) * portTICK_PERIOD_MS;

                        /* Make a small delay so we can read the display */
                        vTaskDelay(500 / portTICK_PERIOD_MS);

                        json_object_begin(json);
                        json_object_add_string(json, "mac", macstr);
                        json_object_add_string(json, "ipv4", trigger_data->ip4_addr_str);
                        json_object_add_bool(json, "hit", true);
                        json_object_add_int(json, "time", hitTimeMillis);
                        json_object_end(json);
                        send_text_to_server(trigger_data->websocket_client_handle, json);
                    }
                    else {
                        //max7219_sendText(trigger_data->spi_device_handle, "MISS");
                        setStripColor(trigger_data->strip_one, STRIP_COLOR_RED);

                        /* Make a small delay so we can read the display */
                        vTaskDelay(500 / portTICK_PERIOD_MS);

                        /* Beam was NOT hit */
                        json_object_begin(json);
                        json_object_add_string(json, "mac", macstr);
                        json_object_add_string(json, "ipv4", trigger_data->ip4_addr_str);
                        json_object_add_bool(json, "hit", false);
                        json_object_end(json);
                        send_text_to_server(trigger_data->websocket_client_handle, json);
                    }
                    trigger_running = false;
                }
            }
            else {
                //max7219_sendText(trigger_data->spi_device_handle, "OVER");
                printf("game over");
            }
        }
    }
}

void breakbeam_start_trigger(int maxWaitForBeamMillis) {
    trigger_running = true;
    xQueueSendToBack(gpio_evt_queue, &maxWaitForBeamMillis, portMAX_DELAY);
}

void breakbeam_stop_trigger() {
    trigger_running = false;
    int noWait = -1;
    xQueueSendToBack(gpio_evt_queue, &noWait, portMAX_DELAY);
}

void breakbeam_start(esp_websocket_client_handle_t websocket_client_handle, spi_device_handle_t spi_device_handle, char* ip4_addr_str, led_strip_t *strip_one) {
    // Read MAC so we can tell who we are
    getMacAddressStr(macstr);

    // Zero-initialize the config structure.
    gpio_config_t io_conf = {};

    // Interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    // Bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;

    // Set as input mode
    io_conf.mode = GPIO_MODE_INPUT;

    // Enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // Set gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_A, GPIO_INTR_POSEDGE);

    // Create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Start trigger task
    static trigger_data_t trigger_data;
    trigger_data.websocket_client_handle = websocket_client_handle;
    trigger_data.spi_device_handle = spi_device_handle;
    trigger_data.strip_one = strip_one;
    trigger_data.ip4_addr_str = ip4_addr_str;
    xTaskCreate(breakbeam_trigger, "breakbeam_trigger", 4096, &trigger_data, 10, NULL);

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // Hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_A, breakbeam_gpio_isr_handler, (void*) GPIO_INPUT_IO_A);
}

