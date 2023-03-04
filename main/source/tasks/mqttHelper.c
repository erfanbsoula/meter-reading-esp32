#include "mqttHelper.h"
#include "mqtt/mqtt_client.h"

static const char *LOG_TAG = "mqtt";

static const char *MQTT_SERVER_IP = "192.168.8.10";
static const uint16_t MQTT_SERVER_PORT = 1883;

static IpAddr SERVER_IP_ADDR;

static const char *STATUS_TOPIC = "board/status";
static const char *MESSAGE_TOPIC = "board/result";

MqttClientContext mqttClientContext;

// ********************************************************************************************
// forward declaration of functions

void mqttTask(void *param);
error_t manualPublishProcess();
error_t mqttConnect();
void mqttPrepareSettings();
error_t mqttConnectionRoutine();

void mqttPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId);

// ********************************************************************************************

void mqttTask(void *param)
{
   error_t error;
   bool_t connectionState = FALSE;

   ipStringToAddr(MQTT_SERVER_IP, &SERVER_IP_ADDR);
   mqttClientInit(&mqttClientContext);

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
            else
            {
               // delay between subsequent connection attempts
               ESP_LOGE(LOG_TAG, "couldn't connect!");
               osDelayTask(2000);
            }
         }
         else
         {
            ESP_LOGE(LOG_TAG, "link is not up!");
            osDelayTask(2000);
         }
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

         osDelayTask(10000);
      }
   }
}

// ********************************************************************************************

error_t manualPublishProcess()
{
   error_t error;

   error = mqttClientPublish(&mqttClientContext, MESSAGE_TOPIC,
      "hello", 5, MQTT_QOS_LEVEL_1, TRUE, NULL);
   
   return error;
}

// ********************************************************************************************

void mqttPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId)
{
   //Check topic name
   if(!strcmp(topic, MESSAGE_TOPIC))
   {
      char_t* str = (char_t*) malloc(length+1);
      strncpy(str, (char_t *) message, length);
      str[length] = '\0';
      ESP_LOGI(LOG_TAG, "PUBLISH packet received '%s'", str);
   }
}

// ********************************************************************************************

error_t mqttConnect()
{
   mqttPrepareSettings();

   ESP_LOGI(LOG_TAG,
      "connecting to MQTT server %s...", MQTT_SERVER_IP);

   error_t error = mqttConnectionRoutine();

   // check status
   if(error)
      mqttClientClose(&mqttClientContext);

   return error;
}

// ********************************************************************************************

void mqttPrepareSettings()
{
   mqttClientSetTransportProtocol(
      &mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);

   mqttClientRegisterPublishCallback(
      &mqttClientContext, mqttPublishCallback);
   
   mqttClientSetWillMessage(&mqttClientContext, STATUS_TOPIC,
      "offline", 7, MQTT_QOS_LEVEL_0, FALSE);

   mqttClientSetTimeout(&mqttClientContext, 10000);
   mqttClientSetKeepAlive(&mqttClientContext, 0);
}

// ********************************************************************************************

error_t mqttConnectionRoutine()
{
   error_t error = mqttClientConnect(&mqttClientContext,
      &SERVER_IP_ADDR, MQTT_SERVER_PORT, TRUE);
   if (error) return error;

   // subscribe to the desired topics
   error = mqttClientSubscribe(&mqttClientContext,
      MESSAGE_TOPIC, MQTT_QOS_LEVEL_1, NULL);
   if (error) return error;

   error = mqttClientPublish(&mqttClientContext, STATUS_TOPIC,
      "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);

   return error;
}

// ********************************************************************************************
