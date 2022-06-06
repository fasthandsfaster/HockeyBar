
#ifndef LASO_UTIL_H
#define LASO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

void hexdump(char* prefix, const uint8_t *data, int data_len);
uint8_t reverseUint8(uint8_t data);
void getMacAddressStr(char* macstr);

int json_object_begin(char* json);
int json_object_add_string(char* json, char* key,  char* value);
int json_object_add_int(char* json, char* key,  int value);
int json_object_add_bool(char* json, char* key,  bool value);
int json_object_end(char* json);


#ifdef __cplusplus
}
#endif


#endif
