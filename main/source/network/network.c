#include "esp_wifi.h"
#include "esp_event.h"
#include "core/net.h"
#include "network.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "resource_manager.h"
#include "esp_log.h"
#include "debug.h"

#include "appEnv.h"
#include "source/server/server.h"

void wifiEventHandler(void *arg, esp_event_base_t eventBase,
   int32_t eventId, void *eventData);

// ********************************************************************************************

void initializeNetworks()
{
   error_t error;

   // create default event loop
   esp_event_loop_create_default();

   // register Wi-Fi event handler
   esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
      wifiEventHandler, NULL);

   // initialize TCP/IP stack
   error = netInit();
   if(error)
      TRACE_ERROR("Failed to initialize TCP/IP stack!\r\n");

   // lanInterfaceInit(&appEnv.lanConfig);
   staWifiInit(&appEnv.staWifiConfig);
   apWifiInit(&appEnv.apWifiConfig);

   initializeHttpServer();

   wifiConnect(&appEnv.staWifiConfig);
   wifiEnableAp(&appEnv.apWifiConfig);
}

// ********************************************************************************************

void wifiEventHandler(void *arg, esp_event_base_t eventBase,
   int32_t eventId, void *eventData)
{
   TRACE_INFO("ESP32: Wi-Fi event handler (event = %d)\r\n", eventId);

   if(eventId == WIFI_EVENT_STA_START)
   {
      TRACE_INFO("ESP32: Station started\r\n");
      // connect station to the access-point
      esp_wifi_connect();
   }
   else if(eventId == WIFI_EVENT_STA_CONNECTED)
   {
      // successful connection
      TRACE_INFO("ESP32: Station connected\r\n");
   }
   else if(eventId == WIFI_EVENT_STA_DISCONNECTED)
   {
      TRACE_INFO("ESP32: Station disconnected\r\n");
      // reconnect station to the AP
      esp_wifi_connect();
   }
   else if(eventId == WIFI_EVENT_AP_STACONNECTED)
   {
      MacAddr macAddr;
      wifi_event_ap_staconnected_t *event;

      // point the event-specific data
      event = (wifi_event_ap_staconnected_t *) eventData;
      // retrieve the MAC address of the station
      macCopyAddr(&macAddr, event->mac);

      TRACE_INFO("ESP32: Station %s joining AP\r\n",
         macAddrToString(&macAddr, NULL));
   }
   else if(eventId == WIFI_EVENT_AP_STADISCONNECTED)
   {
      MacAddr macAddr;
      wifi_event_ap_stadisconnected_t *event;

      // point the event-specific data
      event = (wifi_event_ap_stadisconnected_t *) eventData;
      // retrieve the MAC address of the station
      macCopyAddr(&macAddr, event->mac);

      TRACE_INFO("ESP32: Station %s leaving AP\r\n",
         macAddrToString(&macAddr, NULL));
   }
}
