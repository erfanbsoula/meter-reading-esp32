#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "netConfigParser.h"
#include "esp_log.h"
#include "cJSON.h"

// static const char *LOG_TAG = "netConfig";

// ********************************************************************************************
// forward declaration of functions

bool_t parseNetConfig(NetInterfaceConfig *netConfig, char_t *data,
   NetInterfaceType interface);

bool_t netConfigParseHelper(NetInterfaceConfig *netConfig, cJSON *root,
   NetInterfaceType interface);

static bool_t getStrItem(char_t **str, cJSON *root,
   const char_t *item);

void freeNetConfigStrs(NetInterfaceConfig *netConfig);
// static char_t* strCopy(char_t *str);

// ********************************************************************************************


bool_t parseNetConfig(NetInterfaceConfig *netConfig, char_t *data,
    NetInterfaceType interface)
{
   if (netConfig == NULL)
      return FALSE;

   cJSON* root = cJSON_Parse(data);
   if (root == NULL)
   {
      const char_t *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
         ESP_LOGE("CJSON", "error before: %s", error_ptr);

      cJSON_Delete(root);
      return FALSE;
   }

   osMemset(netConfig, 0, sizeof(NetInterfaceConfig));
   bool_t res = netConfigParseHelper(netConfig, root, interface);
   if (!res) freeNetConfigStrs(netConfig);

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t netConfigParseHelper(NetInterfaceConfig *netConfig, cJSON *root,
    NetInterfaceType interface)
{
   cJSON *child;

   child = cJSON_GetObjectItem(root, "enableInterface");
   if (!cJSON_IsNumber(child)) return FALSE;
   netConfig->enableInterface = cJSON_GetNumberValue(child);
   if (!netConfig->enableInterface)
   {
      return TRUE;
   }

   if(!getStrItem(&netConfig->hostName, root, "hostName") ||
      !getStrItem(&netConfig->macAddress, root, "macAddress"))
   {
      return FALSE;
   }

   child = cJSON_GetObjectItem(root, "enableDHCP");
   if (!cJSON_IsNumber(child)) return FALSE;
   netConfig->enableDHCP = cJSON_GetNumberValue(child);
   if (!netConfig->enableDHCP)
   {
      return TRUE;
   }


   if(!getStrItem(&netConfig->hostAddr, root, "hostAddr") ||
      !getStrItem(&netConfig->subnetMask, root, "subnetMask") ||
      !getStrItem(&netConfig->defaultGateway, root, "defaultGateway") ||
      !getStrItem(&netConfig->primaryDns, root, "primaryDns") ||
      !getStrItem(&netConfig->secondaryDns, root, "secondaryDns"))
   {
      return FALSE;
   }

   if (interface == AP_WIFI_INTERFACE)
   {
      if(!getStrItem(&netConfig->minAddrRange, root, "minAddrRange") ||
         !getStrItem(&netConfig->maxAddrRange, root, "maxAddrRange"))
      {
         return FALSE;
      }
   }

   if(interface == AP_WIFI_INTERFACE ||
      interface == STA_WIFI_INTERFACE)
   {
      if(!getStrItem(&netConfig->SSID, root, "SSID") ||
         !getStrItem(&netConfig->password, root, "password"))
      {
         return FALSE;
      }
   }

   return TRUE;
}

// ********************************************************************************************

static bool_t getStrItem(char_t **str, cJSON *root,
   const char_t *item)
{
   cJSON *child;
   child = cJSON_GetObjectItem(root, item);

   if (!cJSON_IsString(child)) return FALSE;

   *str = child->valuestring;
   child->valuestring = NULL;
   return TRUE;
}

// ********************************************************************************************

void freeNetConfigStrs(NetInterfaceConfig *netConfig)
{
   free(netConfig->hostName);
   free(netConfig->macAddress);
   free(netConfig->hostAddr);
   free(netConfig->subnetMask);
   free(netConfig->defaultGateway);
   free(netConfig->primaryDns);
   free(netConfig->secondaryDns);
   free(netConfig->minAddrRange);
   free(netConfig->maxAddrRange);
   free(netConfig->SSID);
   free(netConfig->password);
}

// ********************************************************************************************

// static char_t* strCopy(char_t *str)
// {
//    char_t *strCopied = malloc(strlen(str) + 1);
//    if (strCopied == NULL) {
//       ESP_LOGE(LOG_TAG, "memory allocation failed!");
//       return NULL;
//    }
//    strcpy(strCopied, str);
//    return strCopied;
// }

// ********************************************************************************************
