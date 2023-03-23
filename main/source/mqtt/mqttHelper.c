#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "mqttHelper.h"

#include "mqtt/mqtt_client.h"
#include "os_port_freertos.h"

#include "source/storage/storage.h"
#include "mqttConfig.h"

static const char *LOG_TAG = "mqtt";

#define  MQTT_MAIN_TASK_INTERVAL 200
MqttClientContext mqttClientContext;

static IpAddr serverIpAddr;
MqttConfig mqttConfig;

#define MESSAGE_QUEUE_LEN 5
char_t *messageQueue[MESSAGE_QUEUE_LEN];

// static const char_t *DEFAULT_SERVER_IP = "192.168.8.10";
// static const uint16_t DEFAULT_SERVER_PORT = 1883;
// static const systime_t DEFAULT_LOOP_DELAY = 10000;
// static const char_t *DEFAULT_STATUS_TOPIC = "board/status";
// static const char_t *DEFAULT_MESSAGE_TOPIC = "board/result";

// ********************************************************************************************
// forward declaration of functions

void mqttInitialize();
void mqttMainTask(void *param);
error_t mqttConnect();

void mqttPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId);

error_t mqttProcessMessageQueue();
void mqttMessageQueueInit();
bool_t mqttMessageQueuePush(char_t *message);
void mqttMessageQueuePop();
char_t* mqttStrCopy(char_t *str);

// ********************************************************************************************

void mqttInitialize()
{
   retrieveMqttConfig(&mqttConfig);

   if (!mqttConfig.isConfigured)
   {
      ESP_LOGI(LOG_TAG, "no configuration found!");
      return;
   }
   else if (!mqttConfig.mqttEnable)
   {
      ESP_LOGI(LOG_TAG, "mqtt disabled!");
      return;
   }

   mqttClientInit(&mqttClientContext);
   mqttMessageQueueInit();

   // initialize mqtt task
   BaseType_t ret = xTaskCreatePinnedToCore(
      mqttMainTask, "mqttTask", 2048, NULL, 12, NULL, 1
   );
   if(ret != pdPASS)
      ESP_LOGE(LOG_TAG,"failed to create mqtt task!");
}

// ********************************************************************************************

void mqttMainTask(void *param)
{
   error_t error;
   bool_t connectionState = FALSE;

   while(1)
   {
      if(!connectionState)
      {
         // make sure the link is up
         if (netGetLinkState(&netInterface[1]))
         {
            ESP_LOGI(LOG_TAG, "link is up!");
            error = mqttConnect();
            if (!error) connectionState = TRUE;
            else ESP_LOGE(LOG_TAG, "couldn't connect!");
         }
         else ESP_LOGE(LOG_TAG, "link is not up!");
      }
      else
      {
         error = mqttProcessMessageQueue();
         if (error)
         {
            // connection to MQTT server lost?
            mqttClientClose(&mqttClientContext);
            connectionState = FALSE;
         }
         else mqttClientTask(
            &mqttClientContext, OS_SYSTICKS_TO_MS(2));
      }
      osDelayTask(MQTT_MAIN_TASK_INTERVAL);
   }
}

// ********************************************************************************************

error_t mqttConnect()
{
   error_t error;

   ipStringToAddr(mqttConfig.serverIP, &serverIpAddr);
   mqttClientSetTransportProtocol(
      &mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);

   mqttClientSetTimeout(&mqttClientContext, mqttConfig.timeout);
   mqttClientSetKeepAlive(
      &mqttClientContext, MQTT_MAIN_TASK_INTERVAL);

   mqttClientRegisterPublishCallback(
      &mqttClientContext, mqttPublishCallback);

   mqttClientSetWillMessage(
      &mqttClientContext, mqttConfig.statusTopic,
      "offline", 7, MQTT_QOS_LEVEL_0, FALSE);

   do // exception handling block
   {
      error = mqttClientConnect(&mqttClientContext,
         &serverIpAddr, mqttConfig.serverPort, TRUE);
      if (error) break;

      // subscribe to the desired topics
      error = mqttClientSubscribe(&mqttClientContext,
         mqttConfig.messageTopic, MQTT_QOS_LEVEL_1, NULL);
      if (error) break;

      error = mqttClientPublish(
         &mqttClientContext, mqttConfig.statusTopic,
         "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);
   } 
   while (0);

   if (error) mqttClientClose(&mqttClientContext);
   return error;
}

// ********************************************************************************************

void mqttPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId)
{
   //Check topic name
   if(!strcmp(topic, mqttConfig.messageTopic))
   {
      char_t *str = (char_t*) malloc(length+1);
      if (!str) return;
      strncpy(str, (char_t *) message, length);
      str[length] = '\0';
      ESP_LOGI(LOG_TAG, "PUBLISH packet received '%s'", str);
   }
}

// ********************************************************************************************

error_t mqttProcessMessageQueue()
{
   error_t error;

   char_t *message = messageQueue[0];
   if (!message) return NO_ERROR;

   error = mqttClientPublish(
      &mqttClientContext, mqttConfig.messageTopic, message,
      strlen(message), MQTT_QOS_LEVEL_1, TRUE, NULL);

   if (!error) mqttMessageQueuePop();

   return error;
}

// ********************************************************************************************

void mqttMessageQueueInit()
{
   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN; i++)
      messageQueue[i] = NULL;
}

// ********************************************************************************************

bool_t mqttMessageQueuePush(char_t *message)
{
   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN; i++)
   {
      if (messageQueue[i] == NULL)
      {
         char_t *mCopy = mqttStrCopy(message);
         if (!mCopy) return FALSE;

         messageQueue[i] = mCopy;
         return TRUE;
      }
   }
   return FALSE;
}

// ********************************************************************************************

void mqttMessageQueuePop()
{
   free(messageQueue[0]);

   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN-1; i++)
      messageQueue[i] = messageQueue[i+1];
   
   messageQueue[MESSAGE_QUEUE_LEN-1] = NULL;
}

// ********************************************************************************************

char_t* mqttStrCopy(char_t *str)
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
