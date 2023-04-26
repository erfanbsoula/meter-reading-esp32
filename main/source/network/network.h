#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "core/net.h"
#include "esp_err.h"

typedef struct _LanConfig LanConfig;
typedef struct _StaWifiConfig StaWifiConfig;
typedef struct _ApWifiConfig ApWifiConfig;

struct _LanConfig
{
   char_t hostname[16];
   MacAddr macAddress;
   uint8_t enableDhcp;
   Ipv4Addr hostAddr;
   Ipv4Addr subnetMask;
   Ipv4Addr defaultGateway;
   Ipv4Addr primaryDns;
   Ipv4Addr secondaryDns;
};

struct _StaWifiConfig
{
   uint8_t enableInterface;
   char_t hostName[16];
   MacAddr macAddress;
   uint8_t useDhcpClient;
   Ipv4Addr hostAddr;
   Ipv4Addr subnetMask;
   Ipv4Addr defaultGateway;
   Ipv4Addr primaryDns;
   Ipv4Addr secondaryDns;
   char_t ssid[32];
   char_t password[32];
};

struct _ApWifiConfig
{
   uint8_t enableInterface;
   char_t hostName[16];
   MacAddr macAddress;
   uint8_t useDhcpServer;
   Ipv4Addr hostAddr;
   Ipv4Addr subnetMask;
   Ipv4Addr defaultGateway;
   Ipv4Addr primaryDns;
   Ipv4Addr secondaryDns;
   Ipv4Addr minAddrRange;
   Ipv4Addr maxAddrRange;
   char_t ssid[32];
   char_t password[32];
};

void initializeNetworks();

void lanSetDefaultConfig(LanConfig *config);
void staWifiSetDefaultConfig(StaWifiConfig *config);
void apWifiSetDefaultConfig(ApWifiConfig *config);

// private functions
error_t lanInterfaceInit(LanConfig *config);
error_t staWifiInit(StaWifiConfig *config);
error_t apWifiInit(ApWifiConfig *config);
esp_err_t wifiConnect(StaWifiConfig *ifConfig);
esp_err_t wifiEnableAp(ApWifiConfig *ifConfig);

#endif