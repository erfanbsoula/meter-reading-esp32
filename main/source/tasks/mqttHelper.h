#ifndef __MQTT_HELPER_H__
#define __MQTT_HELPER_H__

#include "../includes.h"

typedef struct _MqttConfig MqttConfig;
extern MqttConfig mqttConfig;

struct _MqttConfig
{
	bool_t isConfigured;
	systime_t taskLoopDelay;
   char_t *serverIP;
   uint16_t serverPort;
	char_t *statusTopic;
	char_t *messageTopic;
};

void mqttTask(void *param);

#endif