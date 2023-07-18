#include "nvs_flash.h"
#include "nvs.h"

int nvs_init(void);
int read_nvs(char* key, int32_t* value);
int write_nvs(char* key, int32_t value);