#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "core/net.h"
#include "http/http_server.h"

error_t httpServerManualRouter(
    HttpConnection *connection, const char_t *uri);

#endif