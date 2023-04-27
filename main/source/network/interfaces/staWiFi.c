#include <stdlib.h>
#include <string.h>
#include "esp_wifi.h"
#include "source/network/network.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

// first Wi-Fi interface (STA mode)
#define APP_IF1_NAME "wlan0"
#define LOG_TAG "staWiFi"

#if (IPV6_SUPPORT == ENABLED)

#define APP_IF1_USE_SLAAC ENABLED
#define APP_IF1_IPV6_LINK_LOCAL_ADDR "fe80::32:1"

#endif

// ********************************************************************************************
// global variables

static DhcpClientSettings dhcpClientSettings;
static DhcpClientContext dhcpClientContext;

#if (IPV6_SUPPORT == ENABLED)

static SlaacSettings slaacSettings;
static SlaacContext slaacContext;

#endif

// ********************************************************************************************

void staWifiSetDefaultConfig(StaWifiConfig *config)
{
   config->enableInterface = FALSE;
   strcpy(config->hostName, "http-server");
   macStringToAddr("00-00-00-00-00-00", &config->macAddress);
   config->useDhcpClient = TRUE;
   ipv4StringToAddr("192.168.2.20", &config->hostAddr);
   ipv4StringToAddr("255.255.255.0", &config->subnetMask);
   ipv4StringToAddr("192.168.2.1", &config->defaultGateway);
   ipv4StringToAddr("8.8.8.8", &config->primaryDns);
   ipv4StringToAddr("8.8.4.4", &config->secondaryDns);
   strcpy(config->ssid, "IBMCO_Official_plus");
   strcpy(config->password, "@88281228@ibmco");
}

// ********************************************************************************************

error_t staWifiInit(StaWifiConfig *config)
{
   error_t error;

   if (!config->enableInterface)
   {
      ESP_LOGI(LOG_TAG,
         "interface %s is disabled!", APP_IF1_NAME);

      return NO_ERROR;
   }

   // second network interface (Wi-Fi STA mode)
   NetInterface *interface = &netInterface[1];

   netSetInterfaceName(interface, APP_IF1_NAME);
   netSetHostname(interface, config->hostName);
   netSetMacAddr(interface, &config->macAddress);

   // select the relevant network adapter
   netSetDriver(interface, &esp32WifiStaDriver);

   error = netConfigInterface(interface);
   if(error) {
      TRACE_ERROR("Failed to configure interface %s!\r\n",
         interface->name);
      return error;
   }

#if (IPV4_SUPPORT == ENABLED)

   if (config->useDhcpClient)
   {
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
   }
   else
   {
      ipv4SetHostAddr(interface, config->hostAddr);
      ipv4SetSubnetMask(interface, config->subnetMask);
      ipv4SetDefaultGateway(interface, config->defaultGateway);
      ipv4SetDnsServer(interface, 0, config->primaryDns);
      ipv4SetDnsServer(interface, 1, config->secondaryDns);
   }

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
   return NO_ERROR;
}

// ********************************************************************************************

esp_err_t wifiConnect(StaWifiConfig *ifConfig)
{
   if (!ifConfig->enableInterface)
      return ESP_FAIL;

   esp_err_t ret;
   wifi_config_t config;

   TRACE_INFO("ESP32: Connecting to Wi-Fi network (%s)...\r\n",
      ifConfig->ssid);

   memset(&config, 0, sizeof(wifi_config_t));

   strcpy((char_t *) config.sta.ssid, ifConfig->ssid);
   strcpy((char_t *) config.sta.password, ifConfig->password);

   // set Wi-Fi operating mode
   ret = esp_wifi_set_mode(WIFI_MODE_STA);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
   if (ret != ESP_OK) return ret;

   ret = esp_wifi_start();
   return ret;
}

// ********************************************************************************************
