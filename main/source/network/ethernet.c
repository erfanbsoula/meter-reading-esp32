#include <stdlib.h>
#include "network.h"
#include "drivers/mac/esp32_eth_driver.h"
#include "drivers/phy/lan8720_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

// ethernet interface configuration
#define APP_IF0_NAME "lan"
#define LOG_TAG "lan"

#if (IPV6_SUPPORT == ENABLED)

#define APP_IF0_USE_SLAAC ENABLED
#define APP_IF0_IPV6_LINK_LOCAL_ADDR "fe80::32:0"

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

void lanSetDefaultConfig(LanConfig *config)
{
   strcpy(config->hostName, "http-server");
   macStringToAddr("00-AB-CD-EF-32-00", &config->macAddress);
   config->enableDhcp = TRUE;
   ipv4StringToAddr("192.168.1.2", &config->hostAddr);
   ipv4StringToAddr("255.255.255.0", &config->subnetMask);
   ipv4StringToAddr("192.168.1.1", &config->defaultGateway);
   ipv4StringToAddr("8.8.8.8", &config->primaryDns);
   ipv4StringToAddr("8.8.4.4", &config->secondaryDns);
}

// ********************************************************************************************

error_t lanInterfaceInit(LanConfig *config)
{
   error_t error;

   // first network interface (Ethernet 10/100)
   NetInterface *interface = &netInterface[0];

   netSetInterfaceName(interface, APP_IF0_NAME);
   netSetHostname(interface, config->hostname);
   netSetMacAddr(interface, &config->macAddress);

   // select the relevant network adapter
   netSetDriver(interface, &esp32EthDriver);
   netSetPhyDriver(interface, &lan8720PhyDriver);

   // initialize network interface
   error = netConfigInterface(interface);
   if (error) {
      TRACE_ERROR("Failed to configure interface %s!\r\n",
         interface->name);
      return error;
   }

#if (IPV4_SUPPORT == ENABLED)

   if (config->enableDhcp)
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
#if (APP_IF0_USE_SLAAC == ENABLED)

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
   ipv6StringToAddr(APP_IF0_IPV6_LINK_LOCAL_ADDR, &ipv6Addr);
   ipv6SetLinkLocalAddr(interface, &ipv6Addr);

#endif
#endif

   // successful initialization
   return NO_ERROR;
}

// ********************************************************************************************
