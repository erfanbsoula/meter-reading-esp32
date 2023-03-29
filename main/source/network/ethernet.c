#include <stdlib.h>
#include "core/net.h"
#include "network.h"
#include "drivers/mac/esp32_eth_driver.h"
#include "drivers/phy/lan8720_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

// ethernet interface configuration
#define APP_IF0_NAME "eth"
#define APP_IF0_HOST_NAME "http-server"
#define APP_IF0_MAC_ADDR "00-AB-CD-EF-32-00"

#define APP_IF0_USE_DHCP_CLIENT ENABLED
#define APP_IF0_IPV4_HOST_ADDR "192.168.0.20"
#define APP_IF0_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IF0_IPV4_DEFAULT_GATEWAY "192.168.0.254"
#define APP_IF0_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IF0_IPV4_SECONDARY_DNS "8.8.4.4"

#define APP_IF0_USE_SLAAC ENABLED
#define APP_IF0_IPV6_LINK_LOCAL_ADDR "fe80::32:0"

// ********************************************************************************************
// global variables

static DhcpClientSettings dhcpClientSettings;
static DhcpClientContext dhcpClientContext;

#if (IPV6_SUPPORT == ENABLED)

static SlaacSettings slaacSettings;
static SlaacContext slaacContext;

#endif

// ********************************************************************************************

error_t ethInterfaceInit()
{
   error_t error;
   MacAddr macAddr;

   // first network interface (Ethernet 10/100)
   NetInterface *interface = &netInterface[0];

   netSetInterfaceName(interface, APP_IF0_NAME);
   netSetHostname(interface, APP_IF0_HOST_NAME);
   macStringToAddr(APP_IF0_MAC_ADDR, &macAddr);
   netSetMacAddr(interface, &macAddr);

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
#if (APP_IF0_USE_DHCP_CLIENT == ENABLED)

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

   ipv4StringToAddr(APP_IF0_IPV4_HOST_ADDR, &ipv4Addr);
   ipv4SetHostAddr(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF0_IPV4_SUBNET_MASK, &ipv4Addr);
   ipv4SetSubnetMask(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF0_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
   ipv4SetDefaultGateway(interface, ipv4Addr);

   ipv4StringToAddr(APP_IF0_IPV4_PRIMARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 0, ipv4Addr);
   ipv4StringToAddr(APP_IF0_IPV4_SECONDARY_DNS, &ipv4Addr);
   ipv4SetDnsServer(interface, 1, ipv4Addr);

#endif
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
