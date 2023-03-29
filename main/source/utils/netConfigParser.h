#ifndef __NET_CONFIG_PARSER__
#define __NET_CONFIG_PARSER__

#include "source/network/network.h"

bool_t parseNetConfig(NetInterfaceConfig *netConfig, char_t *data,
   NetInterfaceType interface);

#endif