#ifndef __NET_CONFIG_PARSER__
#define __NET_CONFIG_PARSER__

#include "source/network/network.h"

bool_t parseLanConfig(LanConfig *config, char_t *data);
char_t* lanConfigToJson(LanConfig *config);

bool_t parseStaWifiConfig(StaWifiConfig *config, char_t *data);
char_t* staWifiConfigToJson(StaWifiConfig *config);

bool_t parseApWifiConfig(ApWifiConfig *config, char_t *data);
char_t* apWifiConfigToJson(ApWifiConfig *config);

#endif