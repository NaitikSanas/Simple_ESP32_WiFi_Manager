#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void start_wifi();
void start_smart_config(void);
void stop_smartconfig_task();
int is_wifi_connected(void);
#endif