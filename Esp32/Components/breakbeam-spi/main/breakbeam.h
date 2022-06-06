#include "driver/spi_master.h"
void breakbeam_start(esp_websocket_client_handle_t client_handle, spi_device_handle_t spi_device_handle, char* ip4_addr_str);
void breakbeam_start_trigger(int maxWaitForBeamMillis);
void breakbeam_stop_trigger();
