#include "myMqtt.h"
#include "mqtt/mqtt_client.h"

// HttpClientContext httpClientContext;

//MQTT server name
#define APP_SERVER_NAME "test.mosquitto.org"

//MQTT server port
#define APP_SERVER_PORT 1883   //MQTT over TCP

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

   // Resolve MQTT server name
   // error = getHostByName(NULL, APP_SERVER_NAME, &ipAddr, 0);
   // if(error) {
   //    ESP_LOGE("mqttConnect", "failed to resolve server name");
   //    return error;
   // }

   ipStringToAddr("192.168.8.10", &ipAddr);

   //Set the MQTT version to be used
   mqttClientSetVersion(&mqttClientContext, MQTT_VERSION_3_1_1);

   //MQTT over TCP
   mqttClientSetTransportProtocol(&mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);

   //Register publish callback function
   mqttClientRegisterPublishCallback(&mqttClientContext, mqttTestPublishCallback);

   //Set communication timeout
   mqttClientSetTimeout(&mqttClientContext, 10000);
   //Set keep-alive value
   mqttClientSetKeepAlive(&mqttClientContext, 30);

   //Set Will message
   mqttClientSetWillMessage(&mqttClientContext, "board/status",
      "offline", 7, MQTT_QOS_LEVEL_0, FALSE);

   //Debug message
   ESP_LOGE("mqtt", "Connecting to MQTT server %s...\n", ipAddrToString(&ipAddr, NULL));

   //Start of exception handling block
   do
   {
      //Establish connection with the MQTT server
      error = mqttClientConnect(&mqttClientContext,
         &ipAddr, APP_SERVER_PORT, TRUE);
      //Any error to report?
      if(error)
         break;

      //Subscribe to the desired topics
      error = mqttClientSubscribe(&mqttClientContext,
         "kontur/message", MQTT_QOS_LEVEL_1, NULL);
      //Any error to report?
      if(error)
         break;

      //Send PUBLISH packet
      error = mqttClientPublish(&mqttClientContext, "board/status",
         "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);
      //Any error to report?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Check status code
   if(error)
   {
      //Close connection
      mqttClientClose(&mqttClientContext);
   }

   //Return status code
   return error;
}

void mqttTask(void *param)
{
   error_t error;
   bool_t connectionState;
   // char_t buffer[16];

   //Initialize variables
   connectionState = FALSE;

   //Initialize MQTT client context
   mqttClientInit(&mqttClientContext);

   //Endless loop
   while(1)
   {
      //Check connection state
      if(!connectionState)
      {
         //Make sure the link is up
         if(netGetLinkState(&netInterface[1]))
         {
            ESP_LOGI("mqttTask", "link is up!");
            //Try to connect to the MQTT server
            error = mqttConnect();

            //Successful connection?
            if(!error)
            {
               //The MQTT client is connected to the server
               connectionState = TRUE;
            }
            else
            {
               //Delay between subsequent connection attempts
               ESP_LOGE("mqttTask", "couldn't connect!");
               osDelayTask(2000);
            }
         }
         else
         {
            ESP_LOGE("mqttTask", "link is not up!");
            //The link is down
            osDelayTask(2000);
         }
      }
      else
      {
         //Initialize status code
         error = NO_ERROR;

         //Send PUBLISH packet
         error = mqttClientPublish(&mqttClientContext, "kontur/message",
            "hello", 5, MQTT_QOS_LEVEL_1, TRUE, NULL);

         //Check status code
         if(!error)
         {
            //Process events
            error = mqttClientTask(&mqttClientContext, 100);
         }

         //Connection to MQTT server lost?
         if(error)
         {
            //Close connection
            mqttClientClose(&mqttClientContext);
            //Update connection state
            connectionState = FALSE;
            //Recovery delay
         }

         osDelayTask(10000);
      }
   }
}