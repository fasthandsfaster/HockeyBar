#include "driver/spi_master.h"
#include "esp_websocket_client.h"

esp_websocket_client_handle_t websocket_init(char* ip4_addr_str);
void send_text_to_server(esp_websocket_client_handle_t client_handle, char* data);
