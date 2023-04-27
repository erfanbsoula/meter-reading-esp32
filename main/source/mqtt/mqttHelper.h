#ifndef __MQTT_HELPER_H__
#define __MQTT_HELPER_H__

#include "mqtt/mqtt_client.h"

#define MQTT_MAX_TOPIC_LENGTH 19

typedef struct _MqttConfig MqttConfig;

struct _MqttConfig
{
   uint8_t isConfigured;
   uint8_t mqttEnable;
   systime_t timeout;
   Ipv4Addr serverIP;
   uint16_t serverPort;
   char_t statusTopic[MQTT_MAX_TOPIC_LENGTH+1];
   char_t messageTopic[MQTT_MAX_TOPIC_LENGTH+1];
};

void mqttInitialize();
bool_t mqttMessageQueuePush(char_t *message);

// default server port for mqtt is usually 1883

#endif