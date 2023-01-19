#include "includes.h"
#include "session.h"
#include "esp_random.h"

char_t sessions_ring[MAX_SESSION_COUNT][SESSION_ID_LENGTH+1];
int ringPointer = 0;

void getRandomStr(char_t *output, int len)
{
    static const char_t eligible_chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const size_t pool_len = sizeof(eligible_chars);

    for(int i = 0; i < len; i++){
        uint32_t rand_int = (uint32_t) esp_random();
        output[i] = eligible_chars[rand_int % pool_len];
    }
}

void initSessionHandler()
{
    for (int i = 0; i < MAX_SESSION_COUNT; i++) {
        sessions_ring[i][0] = 0;
    }
}

bool_t hasLoggedIn(HttpConnection *connection)
{
    for (int i = 0; i < MAX_SESSION_COUNT; i++) {
        if (!strcmp(connection->request.cookie, sessions_ring[i]))
            return sessions_ring[i][0];
    }

    return false;
}

void logIn(HttpConnection *connection)
{
    getRandomStr(sessions_ring[ringPointer], SESSION_ID_LENGTH);
    sessions_ring[ringPointer][SESSION_ID_LENGTH] = 0;
    strcpy(connection->response.setCookie, sessions_ring[ringPointer]);
    connection->response.setCookie[SESSION_ID_LENGTH] = 0;
    ringPointer = (ringPointer+1) % MAX_SESSION_COUNT;
}
