/* Captive Portal Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include "wifi_manager_portal.h"
#include "project_config.h"

wifi_config_t wifi_configs;
int state = 0;
int s_retry_num = 0;
bool wifi_cred = false;


#define min(a, b) (((a) < (b)) ? (a) : (b))

extern const char portal_html_start[] asm("_binary_portal_html_start");
extern const char portal_html_end[] asm("_binary_portal_html_end");


static const char *TAG = "WIFI";
int wifi_portal_state = 0;
#define storage_namespace "storage"

esp_err_t read_data(char* key, char* value, size_t* len) {
    nvs_handle my_handle;
    //open
    nvs_open(storage_namespace, NVS_READWRITE, &my_handle);


    nvs_get_str(my_handle, key, value, len);
    //close
    nvs_close(my_handle);
    // printf("Key:  %d Value: %s \n", type ,value);
    return ESP_OK;
}


esp_err_t save_data(char* key, char* value) {
    nvs_handle my_handle;
    //open
    nvs_open(storage_namespace, NVS_READWRITE, &my_handle);

    nvs_set_str(my_handle, key, value);
    //close
    nvs_commit(my_handle);
    nvs_close(my_handle);
    //printf("Key: %d Updated to Value: %s \n", type, value);
    return ESP_OK;
}


static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",MAC2STR(event->mac), event->aid);
        wifi_portal_state = 0;
    }
}



void wifi_sta_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
   // esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
    if (s_retry_num < 5)
    {
      state = 10;
     // esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP %d", s_retry_num);
    }
    else
    {
      state = 2;
      ESP_LOGI(TAG, "connect to the AP fail");
    }
    
   }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    state = 1;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;

    save_data("WSSID", (char*)wifi_configs.sta.ssid);
    save_data("WPWD", (char*) wifi_configs.sta.password);
    save_data("CREDVLD", "true");
    ESP_LOGI(TAG, " \n CREDENTIALS STORED NOW...\n" );

    char fssid [32]  = {0};
    size_t ssidlen = 32;
    read_data("WSSID", fssid, &ssidlen);

    ESP_LOGI(TAG, "\n retrived SSID: %s | %d \n", fssid, ssidlen);
  }
}

esp_err_t submit_handler(httpd_req_t *req)
{
  // Check if it's a POST request
   state = 5;
   char content[100];
    memset(content, 0, sizeof(content));
    
    // Read the request payload
    int content_length = req->content_len;
    if (content_length > 0) 
    {
        int read_len = httpd_req_recv(req, content, content_length);
        if (read_len <= 0)
        {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read request payload");
            return ESP_FAIL;
        }
    }

    // Parse the JSON payload
    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to parse JSON payload");
        return ESP_FAIL;
    }
  
    // Extract the network and password values
    cJSON *network_json = cJSON_GetObjectItem(json, "network");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");
    if (network_json == NULL || password_json == NULL)
    {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing network or password field");
        return ESP_FAIL;
    }

    // Get the network and password strings
    const char *network = network_json->valuestring;
    const char *password = password_json->valuestring;


    printf("RECEIVED : %s, %s\n",network,password);
    // Handle the received network and password values
    // You can perform any necessary operations or store the data as needed
    strcpy((char *)wifi_configs.sta.ssid, network);

    // Copy the contents of pass into wifi_configs.sta.password
    strcpy((char *)wifi_configs.sta.password, password);

    
    wifi_cred = true;
    // Cleanup cJSON
    cJSON_Delete(json);

    // Send a response
    const char *response = "Button click received successfully";
    httpd_resp_send(req, response, strlen(response));

    return ESP_OK;
  // Handle the received data (network and password)
  // You can perform any necessary operations or store the data as needed

  // If it's not a POST request, return an error status
  return ESP_FAIL;
}

char last_scan_result[1024] = {0};

esp_err_t update_list_get_handler(httpd_req_t *req) {

  state = 5;
  wifi_ap_record_t ap_info[15];
  memset(ap_info, 0, sizeof(ap_info));

  uint16_t ap_count = 15;
  if (esp_wifi_scan_get_ap_records(&ap_count, ap_info) == ESP_OK) {
    cJSON *root = cJSON_CreateArray();
    
    for (int i = 0; i < ap_count; i++) {
        cJSON *network =  cJSON_CreateString((const char *)ap_info[i].ssid);
        cJSON_AddItemToArray(root, network);
    }
    
    char *json_str = cJSON_Print(root);
    printf("Scan Results: %s\n", json_str);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    state = 6;
    cJSON_Delete(root);
    free(json_str);
  } 
  printf("starting..\n");
  esp_wifi_scan_start(NULL, false);
    printf("done..\n");
  state = 6;
  return ESP_OK;
}


static void wifi_init_softap(void)
{
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL));

  wifi_config_t wifi_config = {
      .ap = {
          .ssid = AP_SSID,
          .ssid_len = strlen(AP_SSID),
          .password = AP_PASSWORD,
          .max_connection = AP_MAX_CONN,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK
      },
  };
  if (strlen(AP_PASSWORD) == 0) {
      wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  

  esp_netif_ip_info_t ip_info;
  esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

  char ip_addr[16];
  inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
  ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
            AP_SSID, AP_PASSWORD);


  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &wifi_sta_event_handler,
                                                    NULL,
                                                    NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &wifi_sta_event_handler,
                                                    NULL,
                                                    NULL));

  ESP_ERROR_CHECK(esp_wifi_start());

}



void connect_wifi(char *ssid, char *pass)
{
  printf("Connecting to wifi now");
  esp_wifi_disconnect();

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = "",
          .password = ""
      },
  };

  sprintf((char *)wifi_config.sta.ssid, "%s", ssid);
  sprintf((char *)wifi_config.sta.password, "%s", pass);

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_wifi_connect();

  ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void connect_from_credential_storage(void){
  if(is_valid_credentials_available()){
    size_t len = 64;
    char ssid [64];
    char password [64];
    read_data("WSSID",ssid, &len);
    read_data("WPASS",password, &len);

    printf("retrived ssid %s, password %s\n",ssid, password);
    connect_wifi(ssid, password);
  }
}

void WiFi_conect_task(void *pvParameters)
{

  while (1)
  {

    if (wifi_cred)
    {

      printf("WiFi Name: %s\n", wifi_configs.sta.ssid);
      printf("Pass: %s\n", wifi_configs.sta.password);
      connect_wifi((char *)wifi_configs.sta.ssid, (char *)wifi_configs.sta.password);
      wifi_cred = false;
    }
    vTaskDelay(10);
  }
}

// HTTP GET Handler

int first = true;
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = portal_html_end - portal_html_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, portal_html_start, root_len);
    return ESP_OK;
}


static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static esp_err_t  get_status_handler(httpd_req_t *req){
    if(state == 0){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : Ready";
      httpd_resp_send(req, str, strlen(str));
    }
    if(state == 1){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : WiFi Connected";
      httpd_resp_send(req, str, strlen(str));
    }
    if(state == 2){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : Credentials invalid";
      httpd_resp_send(req, str, strlen(str));
    }
    if(state == 3){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : unknown error";
      httpd_resp_send(req, str, strlen(str));
    }
     if(state == 5){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : processing your request...";
      httpd_resp_send(req, str, strlen(str));
    }

     if(state == 6){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : Fetched Available networks list..";
      httpd_resp_send(req, str, strlen(str));
      }
    else {
       httpd_resp_set_type(req, "application/text");
      char* str = "Status : unknown error";
      httpd_resp_send(req, str, strlen(str));
    }
    if(state == 10){
      httpd_resp_set_type(req, "application/text");
      char* str = "Status : Retrying to connect..";
      httpd_resp_send(req, str, strlen(str));
    }
    return ESP_OK;
}



// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_open_sockets = 13;
  config.lru_purge_enable = true;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
      // Set URI handlers
      ESP_LOGI(TAG, "Registering URI handlers");
      httpd_register_uri_handler(server, &root);
      httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
  }

  /* Register URI handlers */
  httpd_uri_t update_list_get_uri = {
      .uri      = "/update-list",
      .method   = HTTP_GET,
      .handler  = update_list_get_handler,
      .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &update_list_get_uri);


    httpd_uri_t get_wifi_status_uri = {
      .uri      = "/get_wifi_status",
      .method   = HTTP_GET,
      .handler  = get_status_handler,
      .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &get_wifi_status_uri);

  httpd_uri_t form_uri = {
    .uri = "/button-click",
    .method = HTTP_POST,
    .handler = submit_handler,
    .user_ctx = NULL};

  httpd_register_uri_handler(server, &form_uri);

    return server;
}


void open_wifi_setup_poral(void){
  // Initialize networking stack
      //ESP_ERROR_CHECK(esp_netif_init());

      // Create default event loop needed by the  main app
      //ESP_ERROR_CHECK(esp_event_loop_create_default());

      // Initialize NVS needed by Wi-Fi
     // ESP_ERROR_CHECK(nvs_flash_init());
      tcpip_adapter_init();
      // Initialize Wi-Fi including netif with default config
      esp_netif_create_default_wifi_ap();

      // Initialise ESP32 in SoftAP mode
      wifi_init_softap();

      // Start the server for the first time
      start_webserver();

      // Start the DNS server that will redirect all queries to the softAP IP
      #if USE_DNS_SERVER 
        start_dns_server();
      #endif

      xTaskCreate(&WiFi_conect_task, "WiFi_conect_task", 1024 * 4, NULL, 1, NULL);
     // xTaskCreatePinnedToCore(&wifi_scanner_task, "wifi_scanner_task", 1024 * 3, NULL, 1, NULL,1);
      esp_wifi_scan_start(NULL, false);
  }
  

int start_wifi_manager(void)
{
    /*
        Turn of warnings from HTTP server as redirecting traffic will yield
        lots of invalid requests
    */
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
    open_wifi_setup_poral();

    while (1)
    {
      printf("WFM Running\n");
      vTaskDelay(1000);
    }
  return 0; 
}

bool is_valid_credentials_available(void){
  char wifi_prov_state[16] = {0};
  size_t len = 16;
  read_data("CREDVLD", wifi_prov_state, &len);
  printf("cred validation state : %s\n",wifi_prov_state);
  if(!strcmp(wifi_prov_state,"true")) return true;
  else return false;
}