#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mqttConfigParser.h"
#include "source/utils/cJSON.h"
#include "esp_log.h"

// ********************************************************************************************
// forward declaration of functions

bool_t parseMqttConfig(MqttConfig *mqttConfig, char_t *data);
bool_t mqttParseHelper(MqttConfig *mqttConfig, cJSON *root);
bool_t isValidIpAddress(char_t *ipAddress_);

char_t* mqttConfigToJson(MqttConfig *mqttConfig);
bool_t mqttConfigToJsonHelper(MqttConfig *mqttConfig, cJSON *root);

// ********************************************************************************************

bool_t parseMqttConfig(MqttConfig *mqttConfig, char_t *data)
{
   if (mqttConfig == NULL)
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

   bool_t res = mqttParseHelper(mqttConfig, root);   

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t mqttParseHelper(MqttConfig *mqttConfig, cJSON *root)
{
   cJSON *child;
   // ************************************************************

   child = cJSON_GetObjectItem(root, "mqttEnable");
   if (!cJSON_IsNumber(child)) return FALSE;
   mqttConfig->mqttEnable = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "serverIP");
   if(!cJSON_IsString(child) ||
      !isValidIpAddress(child->valuestring))
      return FALSE;
   ipv4StringToAddr(child->valuestring, &mqttConfig->serverIP);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "serverPort");
   if (!cJSON_IsNumber(child)) return FALSE;
   mqttConfig->serverPort = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "timeout");
   if (!cJSON_IsNumber(child)) return FALSE;
   mqttConfig->timeout = cJSON_GetNumberValue(child);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "statusTopic");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > 19)
      return FALSE;
   strcpy(mqttConfig->statusTopic, child->valuestring);

   // ************************************************************

   child = cJSON_GetObjectItem(root, "messageTopic");
   if(!cJSON_IsString(child) ||
      strlen(child->valuestring) > 19)
      return FALSE;
   strcpy(mqttConfig->messageTopic, child->valuestring);

   // ************************************************************

   mqttConfig->isConfigured = TRUE;
   return TRUE;
}

// ********************************************************************************************

bool_t isValidIpAddress(char_t *ipAddress_)
{
   char_t *ipAddress = mqttStrCopy(ipAddress_);
   if (!ipAddress) return FALSE;

   int32_t numParts = 0;
   char_t *part = strtok(ipAddress, ".");
   while (part != NULL)
   {
      char *endptr;
      long value = strtol(part, &endptr, 10);
      if (endptr == part || value < 0 || value > 255) {
         free(ipAddress);
         return FALSE;
      }

      numParts++;
      part = strtok(NULL, ".");
   }
   free(ipAddress);
   return numParts == 4;
}

// ********************************************************************************************

char_t* mqttConfigToJson(MqttConfig *mqttConfig)
{
   if (mqttConfig == NULL)
      return NULL;

   char_t *jsonStr = NULL;
   cJSON *root = cJSON_CreateObject();
   bool_t result = mqttConfigToJsonHelper(mqttConfig, root);
   if (result) jsonStr = cJSON_Print(root);
   cJSON_Delete(root);
   return jsonStr;
}

// ********************************************************************************************

bool_t mqttConfigToJsonHelper(MqttConfig *mqttConfig, cJSON *root)
{
   cJSON *child;

   child = cJSON_AddNumberToObject(root,
      "mqttEnable", mqttConfig->mqttEnable);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "serverIP", ipv4AddrToString(mqttConfig->serverIP, NULL));
   if (!child) return FALSE;

   child = cJSON_AddNumberToObject(root,
      "serverPort", mqttConfig->serverPort);
   if (!child) return FALSE;

   child = cJSON_AddNumberToObject(root,
      "timeout", mqttConfig->timeout);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "statusTopic", mqttConfig->statusTopic);
   if (!child) return FALSE;

   child = cJSON_AddStringToObject(root,
      "messageTopic", mqttConfig->messageTopic);
   if (!child) return FALSE;

   return TRUE;
}

// ********************************************************************************************
