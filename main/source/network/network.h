#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "os_port.h"
#include "esp_err.h"
#include "error.h"

typedef enum _NetInterfaceType NetInterfaceType;
typedef struct _NetInterfaceConfig NetInterfaceConfig;

enum _NetInterfaceType
{
   ETHERNET_INTERFACE,
   STA_WIFI_INTERFACE,
   AP_WIFI_INTERFACE,
};

struct _NetInterfaceConfig
{
   char_t *hostName;
   char_t *macAddress;
   bool_t enableDHCP;
   char_t *hostAddr;
   char_t *subnetMask;
   char_t *defaultGateway;
   char_t *primaryDns;
   char_t *secondaryDns;
   char_t *minAddrRange;
   char_t *maxAddrRange;
   char_t *SSID;
   char_t *password;
};

void initializeNetworks();

// private functions
error_t ethInterfaceInit();
error_t wifiStaInterfaceInit();
esp_err_t wifiConnect();
error_t wifiApInterfaceInit();
esp_err_t wifiEnableAp();

#endif