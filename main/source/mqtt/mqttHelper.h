#ifndef __MQTT_HELPER_H__
#define __MQTT_HELPER_H__

#include "os_port.h"

typedef struct _MqttConfig MqttConfig;

struct _MqttConfig
{
   bool_t isConfigured;
   bool_t mqttEnable;
   systime_t timeout;
   systime_t taskInterval;
   char_t *serverIP;
   uint16_t serverPort;
   char_t *statusTopic;
   char_t *messageTopic;
};

void mqttInitialize();
bool_t mqttMessageQueuePush(char_t *message);
char_t* mqttStrCopy(char_t *str);

#endif