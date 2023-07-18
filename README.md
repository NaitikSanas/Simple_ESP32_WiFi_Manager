# Simple_WiFi_Manager
 
This repository contains a simple wifi manager. This wifi manager features HTTP Server over Access points which can scan nearby networks and also ability to manually enter credentials. 

Upon calling the `start_wifi()` function it first checks if there are valid credentials in nvs. if not we start wifi provisioning portal.
When connected to the network successfully, the User can invoke the wifi provisioning portal by pushbutton by executing the following sequence. 

```c
esp_wifi_disconnect();
wifi_portal_state = 1;
open_wifi_setup_poral(); 
```


### TODO : 
* Add management of multiple credentials. 
* Managing Connection to the network by prioritizing preferred NetworkName or RSSI.
