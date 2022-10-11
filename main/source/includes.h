#ifndef __INCLUDES_H__
#define __INCLUDES_H__

// All Dependencies
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "core/net.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "drivers/mac/esp32_eth_driver.h"
#include "drivers/phy/lan8720_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "http/http_server.h"
#include "http/mime.h"
#include "path.h"
#include "date_time.h"
#include "resource_manager.h"
#include "debug.h"
#include "esp_log.h"
#include "cyclone_tcp/http/ssi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>

#include "cJSON.h"

typedef struct {
    uint_t x;
    uint_t y;
    uint_t width;
    uint_t height;
} Position;

typedef struct {
    uint_t digitCount;
    bool_t invert;
    Position *positions;
} K210config;

#endif