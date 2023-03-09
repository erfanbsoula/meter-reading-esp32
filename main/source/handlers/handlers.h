#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include "core/net.h"
#include "http/http_server.h"

error_t configHandler(HttpConnection *connection);
error_t cameraImgHandler(HttpConnection* connection);
error_t getAIHandler(HttpConnection *connection);

#endif