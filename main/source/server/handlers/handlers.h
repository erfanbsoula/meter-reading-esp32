#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include "core/net.h"
#include "http/http_server.h"
#include "source/network/network.h"

error_t imgConfigHandler(HttpConnection *connection);
error_t mqttConfigHandler(HttpConnection *connection);
error_t cameraImgHandler(HttpConnection* connection);
error_t getAIHandler(HttpConnection *connection);

error_t netConfigHandler(HttpConnection *connection,
   NetInterfaceType interface);

#endif