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

#include "project_config.h"
#include "nvs_storage.h"
#include <driver/gpio.h>
#include "wifi_manager_portal.h"
#include "wifi_manager.h"
extern int wifi_portal_state;

void push_buttons_init(void);
void wait_on_btn_released();
bool is_btn_pressed(int btn);

void main_task(){
  printf("Main Task Started\n");
  start_wifi();
  while (1)
  {
      /***        SET UP WiFi             ***/
      if(is_btn_pressed(BTN_SET_WIFI)){
          wait_on_btn_released();
          if(!wifi_portal_state){
              esp_wifi_disconnect();
              wifi_portal_state = 1;
              open_wifi_setup_poral(); 
          }
          else {
              esp_restart();         
          }
      }
      vTaskDelay(10);
  }  
}

void app_main(void)
{
    nvs_init();
    push_buttons_init();
   

    xTaskCreate(&main_task, "main_task", 1024 * 4, NULL, 1, NULL);
    while (1)
    {
      /* code */
      vTaskDelay(20);
    }
    
 }




void push_buttons_init(void){
    gpio_pad_select_gpio(BTN_SET_WIFI);
    gpio_set_direction(BTN_SET_WIFI,GPIO_MODE_INPUT);
    gpio_pullup_en(BTN_SET_WIFI);

}

int last_pressed_btn = -1;

void wait_on_btn_released(){
    if(last_pressed_btn == -1)return;
    while(!gpio_get_level(last_pressed_btn))vTaskDelay(10);
    last_pressed_btn = -1;
}

bool is_btn_pressed(int btn){
    last_pressed_btn = btn;
    return !gpio_get_level(btn);
}