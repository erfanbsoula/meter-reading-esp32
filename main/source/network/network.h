#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "os_port.h"
#include "esp_err.h"
#include "error.h"

typedef enum _NetworkType NetworkType;
typedef struct _NetworkConfig NetworkConfig;

enum _NetworkType
{
   staWifi,
   apWifi,
   ethernet
};

struct _NetworkConfig
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
};


void initializeNetworks();

// private functions
error_t ethInterfaceInit();
error_t wifiStaInterfaceInit();
esp_err_t wifiConnect();
error_t wifiApInterfaceInit();
esp_err_t wifiEnableAp();

#endif