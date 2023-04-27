#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "netConfigParser.h"
#include "esp_log.h"
#include "source/utils/cJSON.h"

// ********************************************************************************************
// forward declaration of functions

bool_t parseLanConfig(LanConfig *config, char_t *data);
bool_t parseLanConfigHelper(LanConfig *config, cJSON *root);

char_t* lanConfigToJson(LanConfig *config);
bool_t lanConfigToJsonHelper(LanConfig *config, cJSON *root);

bool_t parseStaWifiConfig(StaWifiConfig *config, char_t *data);
bool_t parseStaWifiConfigHelper(StaWifiConfig *config, cJSON *root);

char_t* staWifiConfigToJson(StaWifiConfig *config);
bool_t staWifiConfigToJsonHelper(StaWifiConfig *config, cJSON *root);

bool_t parseApWifiConfig(ApWifiConfig *config, char_t *data);
bool_t parseApWifiConfigHelper(ApWifiConfig *config, cJSON *root);

char_t* apWifiConfigToJson(ApWifiConfig *config);
bool_t apWifiConfigToJsonHelper(ApWifiConfig *config, cJSON *root);

// ********************************************************************************************

bool_t parseLanConfig(LanConfig *config, char_t *data)
{
   if (config == NULL)
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

   bool_t res = parseLanConfigHelper(config, root);

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t parseLanConfigHelper(LanConfig *config, cJSON *root)
{
   cJSON *child;
   error_t error;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostName");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_HOSTNAME_LENGTH)
      return FALSE;
   strcpy(config->hostName, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "macAddress");
   if (!cJSON_IsString(child)) return FALSE;
   error = macStringToAddr(
      child->valuestring, &config->macAddress);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "enableDhcp");
   if (!cJSON_IsNumber(child)) return FALSE;
   config->enableDhcp = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostAddr");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->hostAddr);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "subnetMask");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->subnetMask);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "defaultGateway");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->defaultGateway);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "primaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->primaryDns);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "secondaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->secondaryDns);
   if (error) return FALSE;

   // ************************************************************

   return TRUE;
}

// ********************************************************************************************

char_t* lanConfigToJson(LanConfig *config)
{
   if (config == NULL)
      return NULL;

   char_t *jsonStr = NULL;
   cJSON *root = cJSON_CreateObject();
   bool_t result = lanConfigToJsonHelper(config, root);
   if (result) jsonStr = cJSON_Print(root);
   cJSON_Delete(root);
   return jsonStr;
}

// ********************************************************************************************

bool_t lanConfigToJsonHelper(LanConfig *config, cJSON *root)
{
   cJSON *child;

   child = cJSON_AddStringToObject(root,
      "hostName", config->hostName);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "macAddress",
      macAddrToString(&config->macAddress, NULL));
   if (!child) return FALSE;

   child = cJSON_AddNumberToObject(root,
      "enableDhcp", config->enableDhcp);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "hostAddr",
      ipv4AddrToString(config->hostAddr, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "subnetMask",
      ipv4AddrToString(config->subnetMask, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "defaultGateway",
      ipv4AddrToString(config->defaultGateway, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "primaryDns",
      ipv4AddrToString(config->primaryDns, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "secondaryDns",
      ipv4AddrToString(config->secondaryDns, NULL));
   if (!child) return FALSE;

   return TRUE;
}

// ********************************************************************************************

bool_t parseStaWifiConfig(StaWifiConfig *config, char_t *data)
{
   if (config == NULL)
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

   bool_t res = parseStaWifiConfigHelper(config, root);

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t parseStaWifiConfigHelper(StaWifiConfig *config, cJSON *root)
{
   cJSON *child;
   error_t error;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "enableInterface");
   if (!cJSON_IsNumber(child)) return FALSE;
   config->enableInterface = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostName");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_HOSTNAME_LENGTH)
      return FALSE;
   strcpy(config->hostName, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "macAddress");
   if (!cJSON_IsString(child)) return FALSE;
   error = macStringToAddr(
      child->valuestring, &config->macAddress);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "useDhcpClient");
   if (!cJSON_IsNumber(child)) return FALSE;
   config->useDhcpClient = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostAddr");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->hostAddr);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "subnetMask");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->subnetMask);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "defaultGateway");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->defaultGateway);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "primaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->primaryDns);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "secondaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->secondaryDns);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "ssid");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_SSID_LENGTH)
      return FALSE;
   strcpy(config->ssid, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "password");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_PASSWORD_LENGTH)
      return FALSE;
   strcpy(config->password, child->valuestring);

   // ************************************************************

   return TRUE;
}

// ********************************************************************************************

char_t* staWifiConfigToJson(StaWifiConfig *config)
{
   if (config == NULL)
      return NULL;

   char_t *jsonStr = NULL;
   cJSON *root = cJSON_CreateObject();
   bool_t result = staWifiConfigToJsonHelper(config, root);
   if (result) jsonStr = cJSON_Print(root);
   cJSON_Delete(root);
   return jsonStr;
}

// ********************************************************************************************

bool_t staWifiConfigToJsonHelper(StaWifiConfig *config, cJSON *root)
{
   cJSON *child;

   child = cJSON_AddNumberToObject(root,
      "enableInterface", config->enableInterface);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "hostName", config->hostName);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "macAddress",
      macAddrToString(&config->macAddress, NULL));
   if (!child) return FALSE;

   child = cJSON_AddNumberToObject(root,
      "useDhcpClient", config->useDhcpClient);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "hostAddr",
      ipv4AddrToString(config->hostAddr, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "subnetMask",
      ipv4AddrToString(config->subnetMask, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "defaultGateway",
      ipv4AddrToString(config->defaultGateway, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "primaryDns",
      ipv4AddrToString(config->primaryDns, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "secondaryDns",
      ipv4AddrToString(config->secondaryDns, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "ssid", config->ssid);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "password", config->password);
   if (!child) return FALSE;

   return TRUE;
}

// ********************************************************************************************

bool_t parseApWifiConfig(ApWifiConfig *config, char_t *data)
{
   if (config == NULL)
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

   bool_t res = parseApWifiConfigHelper(config, root);

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t parseApWifiConfigHelper(ApWifiConfig *config, cJSON *root)
{
   cJSON *child;
   error_t error;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "enableInterface");
   if (!cJSON_IsNumber(child)) return FALSE;
   config->enableInterface = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostName");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_HOSTNAME_LENGTH)
      return FALSE;
   strcpy(config->hostName, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "macAddress");
   if (!cJSON_IsString(child)) return FALSE;
   error = macStringToAddr(
      child->valuestring, &config->macAddress);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "useDhcpServer");
   if (!cJSON_IsNumber(child)) return FALSE;
   config->useDhcpServer = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "hostAddr");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->hostAddr);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "subnetMask");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->subnetMask);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "defaultGateway");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->defaultGateway);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "primaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->primaryDns);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "secondaryDns");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->secondaryDns);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "minAddrRange");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->minAddrRange);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "maxAddrRange");
   if (!cJSON_IsString(child)) return FALSE;
   error = ipv4StringToAddr(
      child->valuestring, &config->maxAddrRange);
   if (error) return FALSE;

   // ************************************************************

   child = cJSON_GetObjectItem(root, "ssid");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_SSID_LENGTH)
      return FALSE;
   strcpy(config->ssid, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "password");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > MAX_PASSWORD_LENGTH)
      return FALSE;
   strcpy(config->password, child->valuestring);

   // ************************************************************

   return TRUE;
}

// ********************************************************************************************

char_t* apWifiConfigToJson(ApWifiConfig *config)
{
   if (config == NULL)
      return NULL;

   char_t *jsonStr = NULL;
   cJSON *root = cJSON_CreateObject();
   bool_t result = apWifiConfigToJsonHelper(config, root);
   if (result) jsonStr = cJSON_Print(root);
   cJSON_Delete(root);
   return jsonStr;
}

// ********************************************************************************************

bool_t apWifiConfigToJsonHelper(ApWifiConfig *config, cJSON *root)
{
   cJSON *child;

   child = cJSON_AddNumberToObject(root,
      "enableInterface", config->enableInterface);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "hostName", config->hostName);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "macAddress",
      macAddrToString(&config->macAddress, NULL));
   if (!child) return FALSE;

   child = cJSON_AddNumberToObject(root,
      "useDhcpServer", config->useDhcpServer);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "hostAddr",
      ipv4AddrToString(config->hostAddr, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "subnetMask",
      ipv4AddrToString(config->subnetMask, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "defaultGateway",
      ipv4AddrToString(config->defaultGateway, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "primaryDns",
      ipv4AddrToString(config->primaryDns, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "secondaryDns",
      ipv4AddrToString(config->secondaryDns, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "minAddrRange",
      ipv4AddrToString(config->minAddrRange, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root, "maxAddrRange",
      ipv4AddrToString(config->maxAddrRange, NULL));
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "ssid", config->ssid);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "password", config->password);
   if (!child) return FALSE;

   return TRUE;
}

// ********************************************************************************************
