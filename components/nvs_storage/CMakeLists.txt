
idf_component_register(SRCS "nvs_storage.c"
                       INCLUDE_DIRS "."
                       REQUIRES "driver" "freertos" "Config" "nvs_flash")
