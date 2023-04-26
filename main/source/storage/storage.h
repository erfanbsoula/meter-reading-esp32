#ifndef __RESTORE_H__
#define __RESTORE_H__

#include "os_port.h"
#include "nvsHelper.h"
#include "source/envTypes.h"

bool_t retrieveEnvironment(Environment *appEnv);

bool_t saveLanConfig(LanConfig *lanConfig);
bool_t saveStaWifiConfig(StaWifiConfig *staWifiConfig);
bool_t saveApWifiConfig(ApWifiConfig *apWifiConfig);
bool_t saveImgConfig(ImgConfig *imgConfig);
bool_t saveUsers(User *users);
bool_t saveMeterCounter(char_t *meterCounter);
bool_t saveMqttConfig(MqttConfig *mqttConfig);

#endif