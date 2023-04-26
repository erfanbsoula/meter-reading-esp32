#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include "core/net.h"
#include "network.h"
#include "source/storage/storage.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

#define LOG_TAG "apWiFi"

// second Wi-Fi interface (AP mode)
#define APP_IF2_NAME "wlan1"

#if (IPV6_SUPPORT == ENABLED)

#define APP_IF2_USE_ROUTER_ADV ENABLED
#define APP_IF2_IPV6_LINK_LOCAL_ADDR "fe80::32:2"
#define APP_IF2_IPV6_PREFIX "fd00:1:2:3::"
#define APP_IF2_IPV6_PREFIX_LENGTH 64
#define APP_IF2_IPV6_GLOBAL_ADDR "fd00:1:2:3::32:2"

#endif

// ********************************************************************************************
// global variables

static DhcpServerSettings dhcpServerSettings;
static DhcpServerContext dhcpServerContext;

#if (IPV6_SUPPORT == ENABLED)

static NdpRouterAdvSettings ndpRouterAdvSettings;
static NdpRouterAdvPrefixInfo ndpRouterAdvPrefixInfo[1];
static NdpRouterAdvContext ndpRouterAdvContext;

#endif

// ********************************************************************************************

void apWifiSetDefaultConfig(ApWifiConfig *config)
{
   config->enableInterface = TRUE;
   strcpy(config->hostName, "http-server");
   macStringToAddr("00-00-00-00-00-00", &config->macAddress);
   config->useDhcpServer = TRUE;
   ipv4StringToAddr("192.168.3.1", &config->hostAddr);
   ipv4StringToAddr("255.255.255.0", &config->subnetMask);
   ipv4StringToAddr("192.168.3.1", &config->defaultGateway);
   ipv4StringToAddr("8.8.8.8", &config->primaryDns);
   ipv4StringToAddr("8.8.4.4", &config->secondaryDns);
   ipv4StringToAddr("192.168.3.2", &config->minAddrRange);
   ipv4StringToAddr("192.168.3.9", &config->maxAddrRange);
   strcpy(config->ssid, "ESP32_AP");
   strcpy(config->password, "test1234");
}

// ********************************************************************************************

error_t apWifiInit(ApWifiConfig *config)
{
   error_t error;

   if (!config->enableInterface)
   {
      ESP_LOGI(LOG_TAG,
         "interface %s is disabled!", APP_IF2_NAME);

      return NO_ERROR;
   }

   // third network interface (Wi-Fi AP mode)
   NetInterface *interface = &netInterface[2];

   netSetInterfaceName(interface, APP_IF2_NAME);
   netSetHostname(interface, config->hostName);
   netSetMacAddr(interface, &config->macAddress);

   // select the relevant network adapter
   netSetDriver(interface, &esp32WifiApDriver);

   error = netConfigInterface(interface);
   if (error) {
      TRACE_ERROR("Failed to configure interface %s!\r\n",
        interface->name);
      return error;
   }

#if (IPV4_SUPPORT == ENABLED)

   ipv4SetHostAddr(interface, config->hostAddr);
   ipv4SetSubnetMask(interface, config->subnetMask);
   ipv4SetDefaultGateway(interface, config->defaultGateway);
   ipv4SetDnsServer(interface, 0, config->primaryDns);
   ipv4SetDnsServer(interface, 1, config->secondaryDns);

   if (config->useDhcpServer)
   {
      dhcpServerGetDefaultSettings(&dhcpServerSettings);
      dhcpServerSettings.interface = interface;
      // lease time, in seconds, assigned to the DHCP clients
      dhcpServerSettings.leaseTime = 3600;

      // lowest and highest IP addresses in the pool that are available
      // for dynamic address assignment
      dhcpServerSettings.ipAddrRangeMin = config->minAddrRange;
      dhcpServerSettings.ipAddrRangeMax = config->maxAddrRange;
   
      dhcpServerSettings.subnetMask = config->subnetMask;
      dhcpServerSettings.defaultGateway = config->defaultGateway;
      dhcpServerSettings.dnsServer[0] = config->primaryDns;
      dhcpServerSettings.dnsServer[1] = config->secondaryDns;

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
   }

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

esp_err_t wifiEnableAp(ApWifiConfig *ifConfig)
{
   if (!ifConfig->enableInterface)
      return ESP_FAIL;

   esp_err_t ret;
   wifi_config_t config;

   TRACE_INFO("ESP32: Creating Wi-Fi network (%s)...\r\n",
      ifConfig->ssid);

   memset(&config, 0, sizeof(wifi_config_t));

   strcpy((char_t *) config.ap.ssid, ifConfig->ssid);
   strcpy((char_t *) config.ap.password, ifConfig->password);
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
