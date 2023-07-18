#include "WiFi_Manager.h"
#include "esp_smartconfig.h"
#include "wifi_manager_portal.h"

EventGroupHandle_t s_wifi_event_group_sta;

const int CONNECTED_BIT = BIT0;
const int ESPTOUCH_DONE_BIT = BIT1;
int s_retry_num2 = 0;
static const char *TAG = "WiFi Manager";
static void normal_sta_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num2 < 4) {
            esp_wifi_connect();
            s_retry_num2++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group_sta, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip-->:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num2 = 0;
        xEventGroupSetBits(s_wifi_event_group_sta, WIFI_CONNECTED_BIT);
    }
}



bool evt_loop_created = false;
void start_wifi()
{
    printf("starting wifi from here...\n");
    s_wifi_event_group_sta = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    if(!evt_loop_created){
        evt_loop_created = true;
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();
    }
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &normal_sta_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &normal_sta_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    if(is_valid_credentials_available()){
        wifi_config_t wifi_config = {
            .sta = {
                .ssid = "",
                .password = ""
            },
        };
        size_t len = 64;
        read_data("WSSID",(char *)wifi_config.sta.ssid,     &len);
        len = 64;
        read_data("WPWD",(char *)wifi_config.sta.password, &len);
        printf("Retrived : %s, %s\n",(char *)wifi_config.sta.ssid,(char *)wifi_config.sta.password);
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    }
    ESP_ERROR_CHECK(esp_wifi_start() );
    esp_wifi_connect();
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group_sta,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            200);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to ap");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
   // vEventGroupDelete(s_wifi_event_group);
}



int is_wifi_connected(void){
    EventBits_t evt =  xEventGroupGetBits(s_wifi_event_group_sta);
    int state =  (evt & WIFI_CONNECTED_BIT);
    return state;       
}