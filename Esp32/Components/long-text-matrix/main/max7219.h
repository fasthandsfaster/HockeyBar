#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_system.h"

#define MATRIX_COUNT 8

void max7219_init(spi_device_handle_t spi);
spi_device_handle_t max7219_initSpiBus(int mosi_pin, int clk_pin, int cs_pin);
void max7219_sendText(spi_device_handle_t spi, char* text);
void max7219_sendNumber(spi_device_handle_t spi, int number);

esp_err_t max7219_sendBytesInTransaction(spi_device_handle_t handle, uint8_t line,  uint8_t tx_data[MATRIX_COUNT]);
