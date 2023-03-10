#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "mqttConfigParser.h"

#include "source/utils/cJSON.h"

// ********************************************************************************************
// forward declaration of functions

bool_t parseMqttConfig(MqttConfig *mqttConfig, char_t *data);
bool_t maqttParseHelper(MqttConfig *mqttConfig, cJSON *root);
bool_t isValidIpAddress(char_t *ipAddress_);

// ********************************************************************************************

bool_t parseMqttConfig(MqttConfig *mqttConfig, char_t *data)
{
   cJSON* root = cJSON_Parse(data);
   if (root == NULL)
   {
      const char_t *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
         ESP_LOGE("CJSON", "error before: %s", error_ptr);

      cJSON_Delete(root);
      return FALSE;
   }

   bool_t res = maqttParseHelper(mqttConfig, root);   

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t maqttParseHelper(MqttConfig *mqttConfig, cJSON *root)
{
   cJSON *serverPort, *loopInterval, *mqttEnable;
   cJSON *serverIP, *statusTopic, *resultTopic;

   mqttEnable = cJSON_GetObjectItem(root, "mqttEnable");
   serverPort = cJSON_GetObjectItem(root, "serverPort");
   loopInterval = cJSON_GetObjectItem(root, "interval");

   serverIP = cJSON_GetObjectItem(root, "serverIP");
   statusTopic = cJSON_GetObjectItem(root, "statusTopic");
   resultTopic = cJSON_GetObjectItem(root, "resultTopic");

   if(!cJSON_IsNumber(mqttEnable) ||
      !cJSON_IsNumber(serverPort) ||
      !cJSON_IsNumber(loopInterval) ||
      !cJSON_IsString(serverIP) ||
      !cJSON_IsString(statusTopic) ||
      !cJSON_IsString(resultTopic))
   {
      return FALSE;
   }

   mqttConfig->serverIP = mqttStrCopy(serverIP->valuestring);
   if(!mqttConfig->serverIP ||
      !isValidIpAddress(mqttConfig->serverIP))
   {
      free(mqttConfig->serverIP);
      return FALSE;
   }

   // !! also check if these are valid topics later !!
   mqttConfig->statusTopic = mqttStrCopy(statusTopic->valuestring);
   mqttConfig->messageTopic = mqttStrCopy(resultTopic->valuestring);
   if(!mqttConfig->statusTopic || !mqttConfig->messageTopic)
   {
      free(mqttConfig->serverIP);
      free(mqttConfig->statusTopic);
      free(mqttConfig->messageTopic);
      return FALSE;
   }

   mqttConfig->mqttEnable = cJSON_GetNumberValue(mqttEnable);
   mqttConfig->serverPort = cJSON_GetNumberValue(serverPort);
   mqttConfig->taskLoopDelay = cJSON_GetNumberValue(loopInterval);
   mqttConfig->isConfigured = TRUE;

   return TRUE;
}

// ********************************************************************************************

bool_t isValidIpAddress(char_t *ipAddress_)
{
   char_t *ipAddress = mqttStrCopy(ipAddress_);
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


