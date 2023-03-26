#ifndef __NET_CONFIG_PARSER__
#define __NET_CONFIG_PARSER__

#include "source/network/network.h"

bool_t parseNetConfig(NetworkConfig *netConfig, char_t *data,
   NetworkType interface);

#endif