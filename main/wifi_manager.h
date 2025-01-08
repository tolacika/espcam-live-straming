#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_event.h"

#define WIFI_CONNECTED_BIT BIT0

extern EventGroupHandle_t wifi_event_group;

void wifi_connect_init(void);

#endif // WIFI_MANAGER_H
