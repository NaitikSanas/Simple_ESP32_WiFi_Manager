#include <sys/param.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "dns_server.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <cJSON.h>
#include "nvs.h"

int start_wifi_manager(void);
bool is_valid_credentials_available(void);
void open_wifi_setup_poral(void);
esp_err_t read_data(char* key, char* value, size_t* len) ;
void wifi_sta_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);