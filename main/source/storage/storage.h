#ifndef __RESTORE_H__
#define __RESTORE_H__

#include "../envTypes.h"

void retrieveEnvironment(Environment *appEnv);
bool_t saveImgConfigJson(char_t *imgConfigJson);
bool_t saveMqttConfigJson(char_t *mqttConfigJson);

#endif