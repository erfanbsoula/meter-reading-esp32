#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "core/net.h"
#include "esp_err.h"

#define MAX_HOSTNAME_LENGTH 15
#define MAX_SSID_LENGTH 31
#define MAX_PASSWORD_LENGTH 31

typedef struct _LanConfig LanConfig;
typedef struct _StaWifiConfig StaWifiConfig;
typedef struct _ApWifiConfig ApWifiConfig;

struct _LanConfig
{
   char_t hostName[MAX_HOSTNAME_LENGTH+1];
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
   char_t hostName[MAX_HOSTNAME_LENGTH+1];
   MacAddr macAddress;
   uint8_t useDhcpClient;
   Ipv4Addr hostAddr;
   Ipv4Addr subnetMask;
   Ipv4Addr defaultGateway;
   Ipv4Addr primaryDns;
   Ipv4Addr secondaryDns;
   char_t ssid[MAX_SSID_LENGTH+1];
   char_t password[MAX_PASSWORD_LENGTH+1];
};

struct _ApWifiConfig
{
   uint8_t enableInterface;
   char_t hostName[MAX_HOSTNAME_LENGTH+1];
   MacAddr macAddress;
   uint8_t useDhcpServer;
   Ipv4Addr hostAddr;
   Ipv4Addr subnetMask;
   Ipv4Addr defaultGateway;
   Ipv4Addr primaryDns;
   Ipv4Addr secondaryDns;
   Ipv4Addr minAddrRange;
   Ipv4Addr maxAddrRange;
   char_t ssid[MAX_SSID_LENGTH+1];
   char_t password[MAX_PASSWORD_LENGTH+1];
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