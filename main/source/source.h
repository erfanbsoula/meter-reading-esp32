#ifndef __SOURCE_H__
#define __SOURCE_H__

#include "core/net.h"
#include "http/http_server.h"

void initApp();

error_t httpServerManualRouter(
    HttpConnection* connection, const char_t *uri);

#endif