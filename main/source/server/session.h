#ifndef __SESSION_H__
#define __SESSION_H__

#include "source/envTypes.h"
#include "core/net.h"
#include "http/http_server.h"

void initSessionHandler();
error_t loginHandler(HttpConnection *connection);
User* findLoggedInUser(HttpConnection *connection);

#endif