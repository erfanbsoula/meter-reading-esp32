#include "mqttHelper.h"
#include "mqtt/mqtt_client.h"

#define MQTT_SERVER_IP "192.168.8.10"
#define MQTT_SERVER_PORT 1883

static const char *STATUS_TOPIC = "board/status";
static const char *MESSAGE_TOPIC = "board/result";

MqttClientContext mqttClientContext;

void mqttTestPublishCallback(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId)
{
   //Check topic name
   if(!strcmp(topic, "kontur/message"))
   {
      char_t* str = (char_t*) malloc(length+1);
      strncpy(str, (char_t *) message, length);
      str[length] = '\0';
      ESP_LOGI("mqtt", "PUBLISH packet received '%s'", str);
   }
}

error_t mqttConnect(void)
{
   error_t error;

   IpAddr ipAddr;
   ipStringToAddr(MQTT_SERVER_IP, &ipAddr);

   mqttClientSetVersion(&mqttClientContext, MQTT_VERSION_3_1_1);
   mqttClientSetTransportProtocol(
      &mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);

   mqttClientRegisterPublishCallback(
      &mqttClientContext, mqttTestPublishCallback);

   mqttClientSetTimeout(&mqttClientContext, 5000);
   mqttClientSetKeepAlive(&mqttClientContext, 20);

   mqttClientSetWillMessage(&mqttClientContext, STATUS_TOPIC,
      "offline", 7, MQTT_QOS_LEVEL_0, FALSE);

   ESP_LOGI("mqtt", "Connecting to MQTT server %s...",
      MQTT_SERVER_IP);

   // exception handling block
   do
   {
      // establish connection with the MQTT server
      error = mqttClientConnect(&mqttClientContext,
         &ipAddr, MQTT_SERVER_PORT, TRUE);

      if(error) break;

      // subscribe to the desired topics
      error = mqttClientSubscribe(&mqttClientContext,
         "kontur/message", MQTT_QOS_LEVEL_1, NULL);

      if(error) break;

      // send PUBLISH packet
      error = mqttClientPublish(&mqttClientContext, STATUS_TOPIC,
         "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);

      if(error) break;

   } while(0);

   // check status
   if(error)
      mqttClientClose(&mqttClientContext);

   return error;
}

void mqttTask(void *param)
{
   error_t error;
   bool_t connectionState = FALSE;

   mqttClientInit(&mqttClientContext);

   while(1)
   {
      if(!connectionState)
      {
         // make sure the link is up
         if(netGetLinkState(&netInterface[1]))
         {
            ESP_LOGI("mqttTask", "link is up!");
            error = mqttConnect();
            if(!error) connectionState = TRUE;
            else
            {
               // delay between subsequent connection attempts
               ESP_LOGE("mqttTask", "couldn't connect!");
               osDelayTask(2000);
            }
         }
         else
         {
            ESP_LOGE("mqttTask", "link is not up!");
            osDelayTask(2000);
         }
      }
      else
      {
         error = NO_ERROR;
         // send PUBLISH packet
         error = mqttClientPublish(&mqttClientContext, "kontur/message",
            "hello", 5, MQTT_QOS_LEVEL_1, TRUE, NULL);

         if(!error)
            error = mqttClientTask(&mqttClientContext, 100);

         // connection to MQTT server lost?
         else
         {
            mqttClientClose(&mqttClientContext);
            connectionState = FALSE;
         }

         osDelayTask(10000);
      }
   }
}