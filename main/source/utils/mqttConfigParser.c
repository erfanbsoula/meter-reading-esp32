#include "../includes.h"
#include "mqttConfigParser.h"
#include "cJSON.h"

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
   cJSON *serverPort, *loopInterval;
   cJSON *serverIP, *statusTopic, *resultTopic;

   serverPort = cJSON_GetObjectItem(root, "serverPort");
   loopInterval = cJSON_GetObjectItem(root, "interval");

   serverIP = cJSON_GetObjectItem(root, "serverIP");
   statusTopic = cJSON_GetObjectItem(root, "statusTopic");
   resultTopic = cJSON_GetObjectItem(root, "resultTopic");

   if(!cJSON_IsNumber(serverPort) ||
      !cJSON_IsNumber(loopInterval) ||
      !cJSON_IsString(serverIP) ||
      !cJSON_IsString(statusTopic) ||
      !cJSON_IsString(resultTopic))
   {
      return false;
   }

   mqttConfig->serverIP = mqttStrCopy(serverIP->valuestring);
   if(!mqttConfig->serverIP ||
      !isValidIpAddress(mqttConfig->serverIP))
   {
      free(mqttConfig->serverIP);
      return false;
   }

   mqttConfig->statusTopic = mqttStrCopy(statusTopic->valuestring);
   mqttConfig->messageTopic = mqttStrCopy(resultTopic->valuestring);
   if(!mqttConfig->statusTopic || !mqttConfig->messageTopic)
   {
      free(mqttConfig->serverIP);
      free(mqttConfig->statusTopic);
      free(mqttConfig->messageTopic);
      return false;
   }

   mqttConfig->serverPort = cJSON_GetNumberValue(serverPort);
   mqttConfig->taskLoopDelay = cJSON_GetNumberValue(loopInterval);

   return true;
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
         return false;
      }

      numParts++;
      part = strtok(NULL, ".");
   }
   free(ipAddress);
   return numParts == 4;
}

// ********************************************************************************************


