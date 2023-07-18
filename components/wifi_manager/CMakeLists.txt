
idf_component_register(SRCS "wifi_manager_portal.c" "dns_server.c" "WiFi_Manager.c" "wifi_manager_portal.c"
                       INCLUDE_DIRS "."
                       REQUIRES "driver"  "freertos" "fatfs" "Config" "lwip" "esp_wifi" "nvs_flash" "esp_http_server" "json" 
                       EMBED_FILES portal.html)
