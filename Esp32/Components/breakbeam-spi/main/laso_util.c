#include <stdio.h>
#include <time.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"

#include "laso_util.h"

static const char *TAG = "LASO_UTIL";

void hexdump(char* prefix, const uint8_t *data, int data_len) {
  const int width = 16;

  uint8_t buffer[(width + 1) * 3]; /* A char can be up to 8 hex digits on a 64-bit system */
  uint8_t *p = buffer;
  *p = '\0';
  
  for (int i=0; i<data_len; i++) {
    if (i>0) {
      if (i%width == 0) {
        /* sprintf(p, "\r\n"); */
        printf("%s: %s\n", prefix, buffer);
        p = buffer;
      }
      else {
        *p++ = ' ';
      }
    }
    int w = sprintf((char*)p, "%02X", *data);
    p += w;
    data++;
  }
  printf("%s: %s\n", prefix, buffer);
}

uint8_t reverseUint8(uint8_t data) {
  return
    ((data & 0x01) << 7) |
    ((data & 0x02) << 5) |
    ((data & 0x04) << 3) |
    ((data & 0x08) << 1) |
    ((data & 0x10) >> 1) |
    ((data & 0x20) >> 3) |
    ((data & 0x40) >> 5) |
    ((data & 0x80) >> 7);
}

// Puts 18 chars in macstr
void getMacAddressStr(char* macstr) {
    uint8_t mac[6];
    esp_err_t result = esp_efuse_mac_get_default(mac);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Reading MAC address failed, error=%d", result);
        return;
    }
    sprintf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int utf8len(const char *s) {
    int len = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) len++ ;
        s++;
    }
    return len;
}

/* JSON */

int json_object_begin(char* json) {
    return sprintf(json, "{");
}

int json_object_add_string(char* json, char* key,  char* value) {
    return sprintf(json + strlen(json), "%s\"%s\":\"%s\"", (strlen(json) <= 1 ? "" : ","), key, value);
}

int json_object_add_int(char* json, char* key,  int value) {
    return sprintf(json + strlen(json), "%s\"%s\":%d", (strlen(json) <= 1 ? "" : ","), key, value);
}

int json_object_add_bool(char* json, char* key,  bool value) {
    return sprintf(json + strlen(json), "%s\"%s\":%s", (strlen(json) <= 1 ? "" : ","), key, (value ? "true" : "false"));
}

int json_object_end(char* json) {
    return sprintf(json + strlen(json), "}");
}
