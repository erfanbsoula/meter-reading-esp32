#ifndef __MQTT_HELPER_H__
#define __MQTT_HELPER_H__

#include "os_port.h"

typedef struct _MqttConfig MqttConfig;
extern MqttConfig mqttConfig;

struct _MqttConfig
{
   bool_t isConfigured;
   bool_t mqttEnable;
   systime_t taskLoopDelay;
   char_t *serverIP;
   uint16_t serverPort;
   char_t *statusTopic;
   char_t *messageTopic;
};

void mqttTask(void *param);
char_t* mqttStrCopy(char_t *str);

#endif