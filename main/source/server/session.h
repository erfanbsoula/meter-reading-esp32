#ifndef __SESSION_H__
#define __SESSION_H__

#include "../envTypes.h"

void initSessionHandler();
error_t loginHandler(HttpConnection *connection);
User* findLoggedInUser(HttpConnection *connection);

#endif