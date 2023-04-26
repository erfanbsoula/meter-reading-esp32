#ifndef __MQTT_HELPER_H__
#define __MQTT_HELPER_H__

#include "mqtt/mqtt_client.h"

typedef struct _MqttConfig MqttConfig;

struct _MqttConfig
{
   uint8_t isConfigured;
   uint8_t mqttEnable;
   systime_t timeout;
   Ipv4Addr serverIP;
   uint16_t serverPort;
   char_t statusTopic[20];
   char_t messageTopic[20];
};

void mqttInitialize();
bool_t mqttMessageQueuePush(char_t *message);
char_t* mqttStrCopy(char_t *str);

#endif