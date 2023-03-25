#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include "core/net.h"
#include "network.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

//Second Wi-Fi interface (AP mode) configuration
#define APP_IF2_NAME "wlan1"
#define APP_IF2_HOST_NAME "http-server-demo"
#define APP_IF2_MAC_ADDR "00-00-00-00-00-00"

#define APP_IF2_USE_DHCP_SERVER ENABLED
#define APP_IF2_IPV4_HOST_ADDR "192.168.8.1"
#define APP_IF2_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IF2_IPV4_DEFAULT_GATEWAY "0.0.0.0"
#define APP_IF2_IPV4_PRIMARY_DNS "0.0.0.0"
#define APP_IF2_IPV4_SECONDARY_DNS "0.0.0.0"
#define APP_IF2_IPV4_ADDR_RANGE_MIN "192.168.8.10"
#define APP_IF2_IPV4_ADDR_RANGE_MAX "192.168.8.99"

#define APP_IF2_USE_ROUTER_ADV ENABLED
#define APP_IF2_IPV6_LINK_LOCAL_ADDR "fe80::32:2"
#define APP_IF2_IPV6_PREFIX "fd00:1:2:3::"
#define APP_IF2_IPV6_PREFIX_LENGTH 64
#define APP_IF2_IPV6_GLOBAL_ADDR "fd00:1:2:3::32:2"

//Wi-Fi parameters (AP mode)
#define APP_WIFI_AP_SSID "TEST_ESP32_AP"
#define APP_WIFI_AP_PASSWORD "1122334455"

// ********************************************************************************************
// global variables

static DhcpServerSettings dhcpServerSettings;
static DhcpServerContext dhcpServerContext;

static NdpRouterAdvSettings ndpRouterAdvSettings;
static NdpRouterAdvPrefixInfo ndpRouterAdvPrefixInfo[1];
static NdpRouterAdvContext ndpRouterAdvContext;

// ********************************************************************************************

error_t wifiApInterfaceInit()
{
   error_t error;
   MacAddr macAddr;

   // third network interface (Wi-Fi AP mode)
   NetInterface *interface = &netInterface[2];

   netSetInterfaceName(interface, APP_IF2_NAME);
   netSetHostname(interface, APP_IF2_HOST_NAME);
   macStringToAddr(APP_IF2_MAC_ADDR, &macAddr);
   netSetMacAddr(interface, &macAddr);

   // select the relevant network adapter
   netSetDriver(interface, &esp32WifiApDriver);

   error = netConfigInterface(interface);
   if (error) {
      TRACE_ERROR("Failed to configure interface %s!\r\n",
        interface->name);
      return error;
   }

#if (IPV4_SUPPORT == ENABLED)

    Ipv4Addr ipv4Addr;

   ipv4StringToAddr(APP_IF2_IPV4_HOST_ADDR, &ipv4Addr);
   ipv4SetHostAddr(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF2_IPV4_SUBNET_MASK, &ipv4Addr);
   ipv4SetSubnetMask(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF2_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
   ipv4SetDefaultGateway(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF2_IPV4_PRIMARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 0, ipv4Addr);
   ipv4StringToAddr(APP_IF2_IPV4_SECONDARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 1, ipv4Addr);

#if (APP_IF2_USE_DHCP_SERVER == ENABLED)

   dhcpServerGetDefaultSettings(&dhcpServerSettings);
   dhcpServerSettings.interface = interface;
   // lease time, in seconds, assigned to the DHCP clients
   dhcpServerSettings.leaseTime = 3600;

   // lowest and highest IP addresses in the pool that are available
   // for dynamic address assignment
   ipv4StringToAddr(APP_IF2_IPV4_ADDR_RANGE_MIN,
      &dhcpServerSettings.ipAddrRangeMin);
   ipv4StringToAddr(APP_IF2_IPV4_ADDR_RANGE_MAX,
      &dhcpServerSettings.ipAddrRangeMax);

   ipv4StringToAddr(APP_IF2_IPV4_SUBNET_MASK,
      &dhcpServerSettings.subnetMask);

   ipv4StringToAddr(APP_IF2_IPV4_DEFAULT_GATEWAY,
      &dhcpServerSettings.defaultGateway);

   ipv4StringToAddr(APP_IF2_IPV4_PRIMARY_DNS,
      &dhcpServerSettings.dnsServer[0]);
   ipv4StringToAddr(APP_IF2_IPV4_SECONDARY_DNS,
      &dhcpServerSettings.dnsServer[1]);

   error = dhcpServerInit(&dhcpServerContext, &dhcpServerSettings);
   if (error) {
      TRACE_ERROR("Failed to initialize DHCP server!\r\n");
      return error;
   }

   error = dhcpServerStart(&dhcpServerContext);
   if (error) {
      TRACE_ERROR("Failed to start DHCP server!\r\n");
      return error;
   }

#endif
#endif

#if (IPV6_SUPPORT == ENABLED)

   Ipv6Addr ipv6Addr;

   ipv6StringToAddr(APP_IF2_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
   ipv6SetLinkLocalAddr(interface, &ipv6Addr);

   ipv6StringToAddr(APP_IF2_IPV6_PREFIX, &ipv6Addr);
   ipv6SetPrefix(interface, 0, &ipv6Addr, APP_IF2_IPV6_PREFIX_LENGTH);

   ipv6StringToAddr(APP_IF2_IPV6_GLOBAL_ADDR, &ipv6Addr);
   ipv6SetGlobalAddr(interface, 0, &ipv6Addr);

#if (APP_IF2_USE_ROUTER_ADV == ENABLED)

   // list of IPv6 prefixes to be advertised
   ipv6StringToAddr(APP_IF2_IPV6_PREFIX,
      &ndpRouterAdvPrefixInfo[0].prefix);

   ndpRouterAdvPrefixInfo[0].length = APP_IF2_IPV6_PREFIX_LENGTH;
   ndpRouterAdvPrefixInfo[0].onLinkFlag = TRUE;
   ndpRouterAdvPrefixInfo[0].autonomousFlag = TRUE;
   ndpRouterAdvPrefixInfo[0].validLifetime = 3600;
   ndpRouterAdvPrefixInfo[0].preferredLifetime = 1800;

   ndpRouterAdvGetDefaultSettings(&ndpRouterAdvSettings);
   ndpRouterAdvSettings.interface = interface;

   // maximum time interval between unsolicited Router Advertisements
   ndpRouterAdvSettings.maxRtrAdvInterval = 60000;
   // minimum time interval between unsolicited Router Advertisements
   ndpRouterAdvSettings.minRtrAdvInterval = 20000;

   ndpRouterAdvSettings.defaultLifetime = 0;
   ndpRouterAdvSettings.prefixList = ndpRouterAdvPrefixInfo;
   ndpRouterAdvSettings.prefixListLength =
      arraysize(ndpRouterAdvPrefixInfo);

   error = ndpRouterAdvInit(&ndpRouterAdvContext, &ndpRouterAdvSettings);
   if (error) {
      TRACE_ERROR("Failed to initialize RA service!\r\n");
      return error;
   }

   error = ndpRouterAdvStart(&ndpRouterAdvContext);
   if (error) {
      TRACE_ERROR("Failed to start RA service!\r\n");
      return error;
   }

#endif
#endif

   //Successful initialization
   return NO_ERROR;
}

// ********************************************************************************************

esp_err_t wifiEnableAp()
{
   esp_err_t ret;
   wifi_config_t config;

   TRACE_INFO("ESP32: Creating Wi-Fi network (%s)...\r\n",
      APP_WIFI_AP_SSID);

   memset(&config, 0, sizeof(wifi_config_t));

   strcpy((char_t *) config.ap.ssid, APP_WIFI_AP_SSID);
   strcpy((char_t *) config.ap.password, APP_WIFI_AP_PASSWORD);
   config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
   config.ap.max_connection = 4;

   ret = esp_wifi_set_mode(WIFI_MODE_AP);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_set_config(ESP_IF_WIFI_AP, &config);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_start();
   return ret;
}

// ********************************************************************************************
