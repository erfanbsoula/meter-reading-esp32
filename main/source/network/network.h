#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "os_port.h"
#include "esp_err.h"
#include "error.h"

typedef struct _NetInterfaseSetting NetInterfaseSetting;

struct _NetInterfaseSetting
{
   char_t *interfaceName;
   char_t *hostName;
   char_t *macAddress;
   bool_t useDhcpClient;
   char_t *hostAddr;
   char_t *subnetMask;
   char_t *defaultGateway;
   char_t *primaryDns;
   char_t *secondaryDns;
};

void initializeNetworks();

// private functions
error_t ethInterfaceInit();
error_t wifiStaInterfaceInit();
esp_err_t wifiConnect();
error_t wifiApInterfaceInit();
esp_err_t wifiEnableAp();

#endif