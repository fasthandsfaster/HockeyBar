#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_system.h"

#include "cp437font.h"
#include "laso_util.h"

#include "max7219.h"


// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html
// https://www.glennklockwood.com/electronics/max7219.html

// e:\udv\esp32\esp-idf-v4.4\examples\peripherals\spi_master\lcd\main\spi_master_example_main.c


// max7219 registers
#define MAX7219_REG_NOOP         0x0
#define MAX7219_REG_DIGIT0       0x1
#define MAX7219_REG_DIGIT1       0x2
#define MAX7219_REG_DIGIT2       0x3
#define MAX7219_REG_DIGIT3       0x4
#define MAX7219_REG_DIGIT4       0x5
#define MAX7219_REG_DIGIT5       0x6
#define MAX7219_REG_DIGIT6       0x7
#define MAX7219_REG_DIGIT7       0x8
#define MAX7219_REG_DECODEMODE   0x9
#define MAX7219_REG_INTENSITY    0xA
#define MAX7219_REG_SCANLIMIT    0xB
#define MAX7219_REG_SHUTDOWN     0xC
#define MAX7219_REG_DISPLAYTEST  0xF

#define LCD_HOST    VSPI_HOST

#define LOGF printf

/*
  The MAX7219CNG has a command protocol, built on SPI, where you send 12-bit packet that encodes 
  both a 4-bit command and an 8-bit value. In this packet, the Most Significant Bits (MSBs),
  or the ones that are sent first, encode the command, and the Least Significant Bits (LSBs)
  encode the data. The specific packet is constructed such that:

  bits 0-3 encode a command
  bits 4-11 encode the value that should be passed to the command

  Two bytes are used to send the packet
*/
esp_err_t sendToMax7219(spi_device_handle_t handle, uint8_t command, uint8_t value) {
    esp_err_t err;

    /* LOGF("command: 0x%X, value: 0x%02X\n", command, value); */

    if (command>0xF) {
        printf("*** Error: invalid command: %X - must be [0..0xF]\n", command);
        return -1;
    }

    uint8_t data[] = {
        command,
        value
    };

    /* e:\udv\esp32\esp-idf-v4.4\components\driver\include\driver\spi_master.h:115 */
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(spi_transaction_t));
    transaction.tx_buffer = &data;
    transaction.length = sizeof(data) * 8;
    err = spi_device_transmit(handle, &transaction);

    if (err) {
        printf("*** Error writing data\n");
    }
    return err;
}

void dumpMatrix(const uint8_t matrix[8]) {
    for (int line=0; line<8; line++) {
        char linebuf[9];
        for (int bit=0; bit<8; bit++) {
            linebuf[7-bit] = ((1<<bit) & matrix[line]) ? 'x' : '.';
        }
        linebuf[8] = 0;
        LOGF("%02X %s\n", matrix[line], linebuf);
    }
}

void rotateMatrix(const uint8_t in [8], uint8_t *out) {
    /* Rotate 90 deg */
    uint8_t mask = 0x80;
    for (int n=0; n<8; n++) {
        *(out+7-n) = 0;
        for (int p=0; p<8; p++) {
            *(out+7-n) |= (((in[p] & mask) >> (7-n)) << (7-p));
        }
        mask = (mask >> 1);
    }
}

esp_err_t max7219_sendBytesInTransaction(spi_device_handle_t handle, uint8_t line,  uint8_t tx_data[MATRIX_COUNT]) {
    hexdump("  TX-DATA", tx_data, MATRIX_COUNT);
    
    uint8_t data[MATRIX_COUNT*2]; 
    
    for (int n=0; n<MATRIX_COUNT; n++) {
        data[n*2] = line;
        data[n*2+1] = reverseUint8(tx_data[MATRIX_COUNT-n-1]);
    }
    hexdump("BYTE-DATA", data, MATRIX_COUNT*2);

    /* e:\udv\esp32\esp-idf-v4.4\components\driver\include\driver\spi_master.h:115 */
    spi_transaction_t transaction; 
    memset(&transaction, 0, sizeof(spi_transaction_t));
    transaction.tx_buffer = data;
    transaction.length = MATRIX_COUNT * 2 * 8; // number of matrices times 2 bytes by 8 bits
        
    /* Source: e:\udv\esp32\esp-idf-v4.4\components\driver\spi_master.c:854 */
    esp_err_t err = spi_device_transmit(handle, &transaction);
    
    if (err) {
        printf("*** Error writing data\n");
    }
    return err;
}

void max7219_sendText(spi_device_handle_t spi, char* text) {
    printf("%s:%d: text: %s\n", __FILE__, __LINE__, text); /* !DEBUG! */ 
    int len = strlen(text);
    if (len>MATRIX_COUNT) len = MATRIX_COUNT;
    printf("%s:%d: len: %d\n", __FILE__, __LINE__, len); /* !DEBUG! */ 
    uint8_t matrices[MATRIX_COUNT][8];
    uint8_t rotated_matrices[MATRIX_COUNT][8];

    // Build dots
    for (int line=0; line<8; line++) {
        for (int c=0; c<len; c++) {
            matrices[c][line] = cp437_font[(int)(*(text+c))][line];
        }
    }

    // Rotate: TODO make a rotated version and store in a file or memory
    for (int c=0; c<len; c++) {
        rotateMatrix(matrices[c], rotated_matrices[c]);
    }

    // Send
    for (int line=0; line<8; line++) {
        uint8_t bytes[MATRIX_COUNT];
        for (int c=0; c<len; c++) {
            bytes[c] = rotated_matrices[c][line];
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); // !DEBUG!
        max7219_sendBytesInTransaction(spi, 8-line, bytes);
    }
}

void max7219_sendNumber(spi_device_handle_t spi, int number) {
    char numstr[5];
    sprintf(numstr, "%4d", number);
    max7219_sendText(spi, numstr);
}

void max7219_init(spi_device_handle_t spi) {
    sendToMax7219(spi, MAX7219_REG_SCANLIMIT, 7);  // No scan limit
    sendToMax7219(spi, MAX7219_REG_DECODEMODE, 0);  // Not using digits
    sendToMax7219(spi, MAX7219_REG_DISPLAYTEST, 0); // No display test
    sendToMax7219(spi, MAX7219_REG_INTENSITY, 1);   // Min character intensity
    sendToMax7219(spi, MAX7219_REG_SHUTDOWN, 1);    // Not in shutdown mode (ie. start it up)

    // Clear first matrix
    for (int n=0; n<8; n++) {
        sendToMax7219(spi, n+1, 0x00);
    }
    /* Shift away to the others */
    for (int n=0; n<MATRIX_COUNT; n++) {
        sendToMax7219(spi, 0, 0x00);
    }
}

spi_device_handle_t max7219_initSpiBus(int mosi_pin, int clk_pin, int cs_pin) {
  
    esp_err_t ret;
    spi_device_handle_t spi;

    /* e:\udv\esp32\esp-idf-v4.4\components\driver\include\driver\spi_common.h:98 */
    spi_bus_config_t buscfg = {
        .miso_io_num=0,  // not used
        .mosi_io_num=mosi_pin,
        .sclk_io_num=clk_pin,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=SPI_MAX_DMA_LEN
    };
  
    /* e:\udv\esp32\esp-idf-v4.4\components\driver\include\driver\spi_master.h:54 */
    spi_device_interface_config_t devcfg = {
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz=10*1000*1000, // Clock out at 10 MHz
        .mode=0,                      // SPI mode 0
        .spics_io_num=cs_pin,         // CS pin
        .flags=0,
        .queue_size=1,                // We want to be able to queue 7 transactions at a time
        .pre_cb=NULL,
        .post_cb=NULL
    };
  
    //Initialize the SPI bus
    ret=spi_bus_initialize(LCD_HOST, &buscfg, 0);
    ESP_ERROR_CHECK(ret);
    
    //Attach the device to the SPI bus
    ret=spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    return spi;
}
