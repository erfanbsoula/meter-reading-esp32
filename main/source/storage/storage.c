#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "storage.h"
#include "source/utils/imgConfigParser.h"
#include "source/mqtt/mqttConfigParser.h"
#include "source/utils/netConfigParser.h"
#include "esp_log.h"

#define LOG_TAG "storage"

// NVS variable names
#define NVS_imgConfig_VAR "imgConfig"
#define NVS_username_VAR "username#"
#define NVS_password_VAR "password#"
#define NVS_meterCounter_VAR "meterCounter"
#define NVS_mqttConfig_VAR "mqttConfig"
#define NVS_staWifiConfig_VAR "staWifiConfig"
#define NVS_apWifiConfig_VAR "apWifiConfig"

#define DEFAULT_USERNAME "admin#"
#define DEFAULT_PASSWORD "12345678"

// ********************************************************************************************
// forward declaration of functions

void retrieveEnvironment(Environment *appEnv);
void retrieveMqttConfig(MqttConfig *mqttConfig);
void retrieveUsers(Environment *appEnv);
void setDefaultUser(User *user, uint_t indx,
   char_t *usernameVar, char_t *passwordVar);

void retrieveImgConfig(ImgConfig *imgConfig);
void retrieveMeterCounter(Environment *appEnv);
bool_t saveImgConfigJson(char_t *imgConfigJson);
bool_t saveMqttConfigJson(char_t *mqttConfigJson);
static char_t* strCopy(char_t *str, size_t strLen);

// ********************************************************************************************

void retrieveEnvironment(Environment *appEnv)
{
   retrieveUsers(appEnv);
   retrieveImgConfig(&(appEnv->imgConfig));
   retrieveMeterCounter(appEnv);
}

// ********************************************************************************************

void retrieveUsers(Environment *appEnv)
{
   static const size_t usernameVarLen = sizeof(NVS_username_VAR);
   static const size_t passwordVarLen = sizeof(NVS_password_VAR);

   char_t *usernameVar = strCopy(NVS_username_VAR, usernameVarLen);
   char_t *passwordVar = strCopy(NVS_password_VAR, passwordVarLen);

   for (uint_t i = 0; i < USER_COUNT; i++)
   {
      User *user = &(appEnv->users[i]);
      usernameVar[usernameVarLen-2] = '0' + i;
      passwordVar[passwordVarLen-2] = '0' + i;

      bool_t uFound, pFound;
      uFound = nvsReadString(usernameVar, &(user->username));
      pFound = nvsReadString(passwordVar, &(user->password));

      if (!uFound || !pFound)
         setDefaultUser(user, i, usernameVar, passwordVar);
   }

   free(usernameVar);
   free(passwordVar);
}

// ********************************************************************************************

void setDefaultUser(User *user, uint_t indx,
   char_t *usernameVar, char_t *passwordVar)
{
   static const size_t defaultUsernameLen = sizeof(DEFAULT_USERNAME);
   static const size_t defaultPasswordLen = sizeof(DEFAULT_PASSWORD);

   user->username = strCopy(DEFAULT_USERNAME, defaultUsernameLen);
   user->username[defaultUsernameLen-2] = '0' + indx;

   user->password = strCopy(DEFAULT_PASSWORD, defaultPasswordLen);

   bool_t uSaved, pSaved;
   uSaved = nvsSaveString(usernameVar, user->username);
   pSaved = nvsSaveString(passwordVar, user->password);

   if (uSaved && pSaved)
      ESP_LOGI(LOG_TAG, "saved default username and password");
}

// ********************************************************************************************

void retrieveImgConfig(ImgConfig *imgConfig)
{
   imgConfig->isConfigured = FALSE;
   imgConfig->positions = NULL;

   char_t *data;
   bool_t result = nvsReadString(NVS_imgConfig_VAR, &data);
   if (!result) return;

   result = parseImgConfig(imgConfig, data);
   if (!result)
   {
      ESP_LOGE(LOG_TAG,
         "couldn't parse imgConfig during retrieval!");
   }
   else imgConfig->isConfigured = TRUE;

   free(data);
}

// ********************************************************************************************

void retrieveMeterCounter(Environment *appEnv)
{
   if (!appEnv->imgConfig.isConfigured)
      return;

   bool_t result = nvsReadString(
      NVS_meterCounter_VAR, &(appEnv->meterCounter));
   if (!result) {
      appEnv->meterCounter = NULL;
      return;
   }

   if (strlen(appEnv->meterCounter) != appEnv->imgConfig.digitCount)
   {
      ESP_LOGE(LOG_TAG,
         "retrieved meterCounter doesn't match imgConfig");
      free(appEnv->meterCounter);
      appEnv->meterCounter = NULL;
      return;
   }

   ESP_LOGI(LOG_TAG,
      "meterCounter retrieved successfully (%s)",
      appEnv->meterCounter);
}

// ********************************************************************************************

void retrieveMqttConfig(MqttConfig *mqttConfig)
{
   mqttConfig->isConfigured = FALSE;
   mqttConfig->mqttEnable = FALSE;
   mqttConfig->serverIP = NULL;
   mqttConfig->messageTopic = NULL;
   mqttConfig->statusTopic = NULL;

   char_t *data;
   bool_t result = nvsReadString(NVS_mqttConfig_VAR, &data);
   if (!result) return;

   result = parseMqttConfig(mqttConfig, data);
   if (!result)
   {
      ESP_LOGE(LOG_TAG,
         "couldn't parse mqttConfig during retrieval!");
   }
   else mqttConfig->isConfigured = TRUE;

   free(data);
}

// ********************************************************************************************

bool_t retrieveNetConfig(NetInterfaceConfig *netConfig,
   NetInterfaceType interface)
{
   char_t *data;
   bool_t result, deallocate = TRUE;

   if (interface == STA_WIFI_INTERFACE) {
      result = nvsReadString(NVS_staWifiConfig_VAR, &data);
      if (!result) {
         data = DEFAULT_STA_CONFIG_JSON;
         nvsSaveString(NVS_staWifiConfig_VAR, data);
         deallocate = FALSE;
      }
   }
   else if (interface == AP_WIFI_INTERFACE) {
      result = nvsReadString(NVS_apWifiConfig_VAR, &data);
      if (!result) {
         data = DEFAULT_AP_CONFIG_JSON;
         nvsSaveString(NVS_apWifiConfig_VAR, data);
         deallocate = FALSE;
      }
   }
   else return FALSE;

   result = parseNetConfig(netConfig, data, interface);
   if (deallocate) free(data);

   if (!result)
   {
      ESP_LOGE(LOG_TAG,
         "couldn't parse netConfig during retrieval!");
      
      return FALSE;
   }
   return TRUE;
}

// ********************************************************************************************

bool_t saveImgConfigJson(char_t *imgConfigJson)
{
   return nvsSaveString(NVS_imgConfig_VAR, imgConfigJson);
}

bool_t saveMqttConfigJson(char_t *mqttConfigJson)
{
   return nvsSaveString(NVS_mqttConfig_VAR, mqttConfigJson);
}

bool_t saveNetConfigJson(char_t *netConfigJson,
   NetInterfaceType interface)
{
   if (interface == STA_WIFI_INTERFACE)
      return nvsSaveString(NVS_staWifiConfig_VAR, netConfigJson);

   if (interface == AP_WIFI_INTERFACE)
      return nvsSaveString(NVS_apWifiConfig_VAR, netConfigJson);

   return FALSE;
}

char_t* getNetConfigJson(NetInterfaceType interface)
{
   char_t *data;
   bool_t result = FALSE;

   if (interface == STA_WIFI_INTERFACE)
      result = nvsReadString(NVS_staWifiConfig_VAR, &data);
   else if (interface == AP_WIFI_INTERFACE)
      result = nvsReadString(NVS_apWifiConfig_VAR, &data);

   if (!result) return NULL;

   return data;
}

// ********************************************************************************************

static char_t* strCopy(char_t *str, size_t memLen)
{
   char_t *strCopied = (char_t*) malloc(memLen);
   if (strCopied == NULL) {
      ESP_LOGE(LOG_TAG, "memory allocation failed!");
      return NULL;
   }

   strcpy(strCopied, str);
   return strCopied;
}
