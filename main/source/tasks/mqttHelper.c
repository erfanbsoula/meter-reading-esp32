#include "mqttHelper.h"
#include "mqtt/mqtt_client.h"
#include "os_port.h"

#define MESSAGE_QUEUE_LEN 5

// static const char_t *DEFAULT_SERVER_IP = "192.168.8.10";
// static const uint16_t DEFAULT_SERVER_PORT = 1883;
// static const systime_t DEFAULT_LOOP_DELAY = 10000;
// static const char_t *DEFAULT_STATUS_TOPIC = "board/status";
// static const char_t *DEFAULT_MESSAGE_TOPIC = "board/result";

static const char *LOG_TAG = "mqtt";
static bool_t restart = FALSE;
char_t *messageQueue[MESSAGE_QUEUE_LEN];

static IpAddr serverIpAddr;
MqttConfig mqttConfig;
MqttClientContext mqttClientContext;

// ********************************************************************************************
// forward declaration of functions

void mqttInitConfiguration();
void mqttTask(void *param);
error_t manualPublishProcess();
error_t mqttConnect();
void mqttPrepareSettings();
error_t mqttConnectionRoutine();
char_t* mqttStrCopy(char_t *str);

void mqttPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId);

// ********************************************************************************************

void mqttInitConfiguration()
{
   mqttClientInit(&mqttClientContext);

   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN; i++)
      messageQueue[i] = NULL;

   // initialize mqtt task
   BaseType_t ret = xTaskCreatePinnedToCore(
      mqttTask, "mqttTask", 2048, NULL, 12, NULL, 1
   );
   if(ret != pdPASS)
      ESP_LOGE(LOG_TAG,"failed to create mqtt task!");
}

// ********************************************************************************************

void mqttTask(void *param)
{
   error_t error;
   bool_t connectionState = FALSE;

   // wait until configuration is loaded
   while(!mqttConfig.isConfigured)
   {
      ESP_LOGI(LOG_TAG, "waiting for configuration ...");
      osDelayTask(10000);
   }

   while(1)
   {
      if (restart) {
         mqttClientClose(&mqttClientContext);
         connectionState = FALSE;
         restart = FALSE;
      }

      if(!connectionState)
      {
         // make sure the link is up
         if (netGetLinkState(&netInterface[1]))
         {
            ESP_LOGI(LOG_TAG, "link is up!");
            error = mqttConnect();
            if (!error) {
               connectionState = TRUE;
               continue;
            }
            else ESP_LOGE(LOG_TAG, "couldn't connect!");
         }
         else ESP_LOGE(LOG_TAG, "link is not up!");
      }
      else
      {
         error = manualPublishProcess();
         if (error)
         {
            // connection to MQTT server lost?
            mqttClientClose(&mqttClientContext);
            connectionState = FALSE;
         }
         else mqttClientTask(&mqttClientContext, 100);
      }
      osDelayTask(mqttConfig.taskLoopDelay);
   }
}

// ********************************************************************************************

error_t manualPublishProcess()
{
   error_t error, result = NO_ERROR;

   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN; i++)
   {
      if (messageQueue[i])
      {
         error = mqttClientPublish(
            &mqttClientContext, mqttConfig.messageTopic,
            messageQueue[i], strlen(messageQueue[i]),
            MQTT_QOS_LEVEL_1, TRUE, NULL);
         
         if (!error) {
            free(messageQueue[i]);
            messageQueue[i] = NULL;
         }
         else result = ERROR_FAILURE;
      }
   }
   return result;
}

// ********************************************************************************************

error_t mqttConnect()
{
   mqttPrepareSettings();

   ESP_LOGI(LOG_TAG,
      "connecting to MQTT server %s...", mqttConfig.serverIP);

   error_t error = mqttConnectionRoutine();

   // check status
   if(error)
      mqttClientClose(&mqttClientContext);

   return error;
}

// ********************************************************************************************

void mqttPrepareSettings()
{
   ipStringToAddr(mqttConfig.serverIP, &serverIpAddr);

   mqttClientSetTransportProtocol(
      &mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);

   mqttClientRegisterPublishCallback(
      &mqttClientContext, mqttPublishCallback);
   
   mqttClientSetWillMessage(
      &mqttClientContext, mqttConfig.statusTopic,
      "offline", 7, MQTT_QOS_LEVEL_0, FALSE);

   mqttClientSetTimeout(&mqttClientContext, 10000);
   mqttClientSetKeepAlive(&mqttClientContext, 0);
}

// ********************************************************************************************

error_t mqttConnectionRoutine()
{
   error_t error = mqttClientConnect(&mqttClientContext,
      &serverIpAddr, mqttConfig.serverPort, TRUE);
   if (error) return error;

   // subscribe to the desired topics
   // error = mqttClientSubscribe(&mqttClientContext,
   //    mqttConfig.messageTopic, MQTT_QOS_LEVEL_1, NULL);
   // if (error) return error;

   error = mqttClientPublish(
      &mqttClientContext, mqttConfig.statusTopic,
      "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);

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
      char_t* str = (char_t*) malloc(length+1);
      strncpy(str, (char_t *) message, length);
      str[length] = '\0';
      ESP_LOGI(LOG_TAG, "PUBLISH packet received '%s'", str);
   }
}

// ********************************************************************************************

bool_t mqttMessageQueueAdd(char_t *message)
{
   for (int32_t i = 0; i < MESSAGE_QUEUE_LEN; i++)
   {
      if (messageQueue[i] == NULL)
      {
         char_t mCopy = mqttStrCopy(message);
         if (!mCopy) return false;

         messageQueue[i] = mCopy;
         return true;
      }
   }
   return false;
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
