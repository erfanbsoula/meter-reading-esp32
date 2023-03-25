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


//First Wi-Fi interface (STA mode) configuration
#define APP_IF1_NAME "wlan0"
#define APP_IF1_HOST_NAME "http-server"
#define APP_IF1_MAC_ADDR "00-00-00-00-00-00"

#define APP_IF1_USE_DHCP_CLIENT ENABLED
#define APP_IF1_IPV4_HOST_ADDR "192.168.0.20"
#define APP_IF1_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IF1_IPV4_DEFAULT_GATEWAY "192.168.0.254"
#define APP_IF1_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IF1_IPV4_SECONDARY_DNS "8.8.4.4"

#define APP_IF1_USE_SLAAC ENABLED
#define APP_IF1_IPV6_LINK_LOCAL_ADDR "fe80::32:1"

//Wi-Fi parameters (STA mode)
#define APP_WIFI_STA_SSID "IBMCO_Official_plus"
#define APP_WIFI_STA_PASSWORD "@88281228@ibmco"

// ********************************************************************************************
// global variables

static DhcpClientSettings dhcpClientSettings;
static DhcpClientContext dhcpClientContext;

static SlaacSettings slaacSettings;
static SlaacContext slaacContext;

// ********************************************************************************************

error_t wifiStaInterfaceInit()
{
   error_t error;
   MacAddr macAddr;

   // second network interface (Wi-Fi STA mode)
   NetInterface *interface = &netInterface[1];

   netSetInterfaceName(interface, APP_IF1_NAME);
   netSetHostname(interface, APP_IF1_HOST_NAME);
   macStringToAddr(APP_IF1_MAC_ADDR, &macAddr);
   netSetMacAddr(interface, &macAddr);

   // select the relevant network adapter
   netSetDriver(interface, &esp32WifiStaDriver);

   error = netConfigInterface(interface);
   if(error) {
      TRACE_ERROR("Failed to configure interface %s!\r\n",
         interface->name);
      return error;
   }

#if (IPV4_SUPPORT == ENABLED)
#if (APP_IF1_USE_DHCP_CLIENT == ENABLED)

   dhcpClientGetDefaultSettings(&dhcpClientSettings);
   dhcpClientSettings.interface = interface;
   // disable rapid commit option
   dhcpClientSettings.rapidCommit = FALSE;

   error = dhcpClientInit(&dhcpClientContext, &dhcpClientSettings);
   if (error) {
      TRACE_ERROR("Failed to initialize DHCP client!\r\n");
      return error;
   }

   error = dhcpClientStart(&dhcpClientContext);
   if (error) {
      TRACE_ERROR("Failed to start DHCP client!\r\n");
      return error;
   }

#else

   Ipv4Addr ipv4Addr;

   ipv4StringToAddr(APP_IF1_IPV4_HOST_ADDR, &ipv4Addr);
   ipv4SetHostAddr(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF1_IPV4_SUBNET_MASK, &ipv4Addr);
   ipv4SetSubnetMask(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF1_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
   ipv4SetDefaultGateway(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 0, ipv4Addr);
   ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 1, ipv4Addr);

#endif
#endif

#if (IPV6_SUPPORT == ENABLED)
#if (APP_IF1_USE_SLAAC == ENABLED)

   slaacGetDefaultSettings(&slaacSettings);
   slaacSettings.interface = interface;

   error = slaacInit(&slaacContext, &slaacSettings);
   if(error) {
      TRACE_ERROR("Failed to initialize SLAAC!\r\n");
      return error;
   }

   // start IPv6 address autoconfiguration process
   error = slaacStart(&slaacContext);
   if(error) {
      TRACE_ERROR("Failed to start SLAAC!\r\n");
      return error;
   }

#else

   Ipv6Addr ipv6Addr;

   // set link-local address
   ipv6StringToAddr(APP_IF1_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
   ipv6SetLinkLocalAddr(interface, &ipv6Addr);

#endif
#endif

   //Successful initialization
   ESP_LOGI("wifi", "connected succesfully!");
   netSetLinkState(interface, NIC_LINK_STATE_UP);
   return NO_ERROR;
}

// ********************************************************************************************

esp_err_t wifiConnect()
{
   esp_err_t ret;
   wifi_config_t config;

   TRACE_INFO("ESP32: Connecting to Wi-Fi network (%s)...\r\n",
      APP_WIFI_STA_SSID);

   memset(&config, 0, sizeof(wifi_config_t));

   strcpy((char_t *) config.sta.ssid, APP_WIFI_STA_SSID);
   strcpy((char_t *) config.sta.password, APP_WIFI_STA_PASSWORD);

   // set Wi-Fi operating mode
   ret = esp_wifi_set_mode(WIFI_MODE_STA);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_start();
   return ret;
}

// ********************************************************************************************
