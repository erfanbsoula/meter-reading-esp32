#ifndef __MQTT_CONFIG__
#define __MQTT_CONFIG__

#include "os_port.h"
#include "../tasks/mqttHelper.h"

bool_t parseMqttConfig(MqttConfig *mqttConfig, char_t *data);

#endif