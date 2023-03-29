#ifndef __RESTORE_H__
#define __RESTORE_H__

#include "os_port.h"
#include "nvsHelper.h"
#include "source/envTypes.h"
#include "source/mqtt/mqttHelper.h"
#include "source/network/network.h"

void retrieveEnvironment(Environment *appEnv);
void retrieveMqttConfig(MqttConfig *mqttConfig);
bool_t saveImgConfigJson(char_t *imgConfigJson);
bool_t saveMqttConfigJson(char_t *mqttConfigJson);

bool_t saveNetConfigJson(char_t *netConfigJson,
   NetInterfaceType interface);

bool_t retrieveNetConfig(NetInterfaceConfig *netConfig,
   NetInterfaceType interface);

char_t* getNetConfigJson(NetInterfaceType interface);

#endif