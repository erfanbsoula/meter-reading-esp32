#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "netConfigParser.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *LOG_TAG = "netConfig";

// ********************************************************************************************
// forward declaration of functions

bool_t parseNetConfig(NetworkConfig *netConfig, char_t *data,
   NetworkType interface);

bool_t netConfigParseHelper(NetworkConfig *netConfig, cJSON *root,
   NetworkType interface);

static void freeNetConfigStrs(NetworkConfig *netConfig);
static char_t* strCopy(char_t *str);

// ********************************************************************************************


bool_t parseNetConfig(NetworkConfig *netConfig, char_t *data,
    NetworkType interface)
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

   osMemset(netConfig, 0, sizeof(NetworkConfig));
   bool_t res = netConfigParseHelper(netConfig, root, interface);
   if (!res) freeNetConfigStrs(netConfig);

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t netConfigParseHelper(NetworkConfig *netConfig, cJSON *root,
    NetworkType interface)
{
   cJSON *hostName, *macAddress, *enableDHCP, *hostAddr;
   cJSON *subnetMask, *defaultGateway, *primaryDns;
   cJSON *secondaryDns, *minAddrRange, *maxAddrRange;

   hostName = cJSON_GetObjectItem(root, "hostName");
   macAddress = cJSON_GetObjectItem(root, "macAddress");
   enableDHCP = cJSON_GetObjectItem(root, "enableDHCP");

   if(!cJSON_IsString(hostName) ||
      !cJSON_IsString(macAddress) ||
      !cJSON_IsNumber(enableDHCP))
   {
      return FALSE;
   }

   netConfig->hostName = strCopy(hostName->valuestring);
   if (!netConfig->hostName) return FALSE;
   netConfig->macAddress = strCopy(macAddress->valuestring);
   if (!netConfig->macAddress) return FALSE;

   netConfig->enableDHCP = cJSON_GetNumberValue(enableDHCP);

   if (!netConfig->enableDHCP)
   {
      return TRUE;
   }

   hostAddr = cJSON_GetObjectItem(root, "hostAddr");
   subnetMask = cJSON_GetObjectItem(root, "subnetMask");
   defaultGateway = cJSON_GetObjectItem(root, "defaultGateway");
   primaryDns = cJSON_GetObjectItem(root, "primaryDns");
   secondaryDns = cJSON_GetObjectItem(root, "secondaryDns");

   if(!cJSON_IsString(hostAddr) ||
      !cJSON_IsString(subnetMask) ||
      !cJSON_IsString(defaultGateway) ||
      !cJSON_IsString(primaryDns) ||
      !cJSON_IsString(secondaryDns))
   {
      return FALSE;
   }

   netConfig->hostAddr = strCopy(hostAddr->valuestring);
   if (!netConfig->hostAddr) return FALSE;
   netConfig->subnetMask = strCopy(subnetMask->valuestring);
   if (!netConfig->subnetMask) return FALSE;
   netConfig->defaultGateway = strCopy(defaultGateway->valuestring);
   if (!netConfig->defaultGateway) return FALSE;
   netConfig->primaryDns = strCopy(primaryDns->valuestring);
   if (!netConfig->primaryDns) return FALSE;
   netConfig->secondaryDns = strCopy(secondaryDns->valuestring);
   if (!netConfig->secondaryDns) return FALSE;

   if (interface == apWifi)
   {
      minAddrRange = cJSON_GetObjectItem(root, "minAddrRange");
      maxAddrRange = cJSON_GetObjectItem(root, "maxAddrRange");

      if(!cJSON_IsString(minAddrRange) ||
         !cJSON_IsString(maxAddrRange))
      {
         return FALSE;
      }

      netConfig->minAddrRange = strCopy(minAddrRange->valuestring);
      if (!netConfig->minAddrRange) return FALSE;
      netConfig->maxAddrRange = strCopy(maxAddrRange->valuestring);
      if (!netConfig->maxAddrRange) return FALSE;
   }

   return TRUE;
}

// ********************************************************************************************

static void freeNetConfigStrs(NetworkConfig *netConfig)
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
}

// ********************************************************************************************

static char_t* strCopy(char_t *str)
{
   char_t *strCopied = malloc(strlen(str) + 1);
   if (strCopied == NULL) {
      ESP_LOGE(LOG_TAG, "memory allocation failed!");
      return NULL;
   }
   strcpy(strCopied, str);
   return strCopied;
}

// ********************************************************************************************
