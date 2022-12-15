#ifndef __SESSION_H__
#define __SESSION_H__

// warning!
// sessionId length must be less than or equal to 32
#define SESSION_ID_LENGTH 16

#define MAX_SESSION_COUNT 2

void initSessionHandler();
bool_t hasLoggedIn(HttpConnection *connection);
void logIn(HttpConnection *connection);

#endif