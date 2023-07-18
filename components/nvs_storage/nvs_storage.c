#include "nvs_storage.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int nvs_init(void){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    return err;
}

int read_nvs(char* key, int32_t* value){
    nvs_handle_t my_handle;
    esp_err_t err ;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
       
    } else {
        err = nvs_get_i32(my_handle, key, value);
        nvs_close(my_handle);    
    }
    return err;
}

int write_nvs(char* key, int32_t value){
    nvs_handle_t my_handle;
    esp_err_t    err;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
    } else {
        err = nvs_set_i32(my_handle, key, value);
        err = nvs_commit(my_handle);
        nvs_close(my_handle);
    }
    return err;
}