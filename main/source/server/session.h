#ifndef __SESSION_H__
#define __SESSION_H__

#include "../environment.h"

void initSessionHandler();
error_t loginHandler(HttpConnection *connection);
User* findLoggedInUser(HttpConnection *connection);

#endif