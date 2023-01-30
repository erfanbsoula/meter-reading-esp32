#ifndef __SESSION_H__
#define __SESSION_H__

#include "../manual.h"

void initSessionHandler(User *users_);
error_t loginHandler(HttpConnection *connection);
User* hasLoggedIn(HttpConnection *connection);

#endif