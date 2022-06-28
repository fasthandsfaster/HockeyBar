/* FROM e:\udv\esp32\esp-idf-v4.4\examples\protocols\websocket\main\websocket_example.c */

/* SERVER https://github.com/espressif/esp-idf/pull/4306 */

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "cJSON.h"

#include "breakbeam.h"

#define NO_DATA_TIMEOUT_SEC 10
#define WEBSOCKET_SERVER_URI "ws://192.168.192.149:8081/target"
//#define WEBSOCKET_SERVER_URI "ws://172.20.10.3:8080/target" // LARS

static const char *TAG = "BLS_WS";

static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;

static void shutdown_signaler(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

/* FROM rfc6455 */
/* IANA has added initial values to the registry as follows. */

/*  |Opcode  | Meaning                             | Reference | */
/* -+--------+-------------------------------------+-----------| */
/*  | 0      | Continuation Frame                  | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
/*  | 1      | Text Frame                          | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
/*  | 2      | Binary Frame                        | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
/*  | 8      | Connection Close Frame              | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
/*  | 9      | Ping Frame                          | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
/*  | 10     | Pong Frame                          | RFC 6455  | */
/* -+--------+-------------------------------------+-----------| */
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    printf("\n");
  
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d", data->payload_len, data->data_len, data->payload_offset);

        switch (data->op_code) {
        case 0:
            ESP_LOGW(TAG, "Continuation Frame");
            break;
            
        case 1:
            ESP_LOGW(TAG, "Received Text=%.*s", data->data_len, (char *)data->data_ptr);
            cJSON *root = cJSON_Parse((char *)data->data_ptr);
            char* command = cJSON_GetObjectItem(root, "command")->valuestring;
            int maxWaitForBeamMillis = cJSON_GetObjectItem(root, "maxWait")->valueint;
            printf("%s:%d: maxWaitForBeamMillis: %d\n", __FILE__, __LINE__, maxWaitForBeamMillis); /* !DEBUG! */

            if (!strcmp(command, "start")) {
                breakbeam_start_trigger(maxWaitForBeamMillis);
            }
            else if (!strcmp(command, "over")) {
                breakbeam_stop_trigger();
            }
            else {
                ESP_LOGW(TAG, "Unknown command %s", command);
            }
            cJSON_Delete(root);
            break;
            
        case 2:
            ESP_LOGW(TAG, "Binary Frame");
            break;
            
        case 8:
            if (data->data_len == 2) {
                ESP_LOGW(TAG, "Received closed message with code=%d", 256*data->data_ptr[0] + data->data_ptr[1]);
            }
            break;
            
        case 9:
            ESP_LOGW(TAG, "Ping Frame");
            break;
            
        case 10:
            ESP_LOGW(TAG, "Pong Frame");
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown opcode: %d", data->op_code);
        }
        
        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
        
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

esp_websocket_client_handle_t websocket_create_client() {
    esp_websocket_client_config_t websocket_cfg = {};

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS, pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    websocket_cfg.uri = WEBSOCKET_SERVER_URI;

    ESP_LOGI(TAG, "Connecting to %s", websocket_cfg.uri);
    esp_websocket_client_handle_t websocket_client_handle = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(websocket_client_handle, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
    esp_websocket_client_start(websocket_client_handle);
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);

    return websocket_client_handle;
}

void send_text_to_server(esp_websocket_client_handle_t client_handle, char* data) {
    
    if (esp_websocket_client_is_connected(client_handle)) {
        int len = strlen(data);
        ESP_LOGI(TAG, "Sending %s", data);
        esp_websocket_client_send_text(client_handle, data, len, portMAX_DELAY);
    }
    else {
        ESP_LOGE(TAG, "Not connected to server");
    }
}

void websocket_close_client(esp_websocket_client_handle_t client_handle) {
    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_websocket_client_close(client_handle, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(client_handle);
}

esp_websocket_client_handle_t websocket_init(char* ip4_addr_str) {

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("WEBSOCKET_CLIENT", ESP_LOG_DEBUG);
    esp_log_level_set("TRANSPORT_WS", ESP_LOG_DEBUG);
    esp_log_level_set("TRANS_TCP", ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    ESP_ERROR_CHECK(example_connect(ip4_addr_str));

    esp_websocket_client_handle_t client_handle = websocket_create_client();
    
    /* websocket_close_client(client_handle); */

    return client_handle;
}
