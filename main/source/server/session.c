#include "../includes.h"
#include "../appEnv.h"
#include "session.h"
#include "httpHelper.h"
#include "esp_random.h"

#define READ_STREAM_BUF_SIZE 64

static const char_t *LOG_TAG = "session";

// ********************************************************************************************
// forward declaration of functions

void initSessionHandler();
error_t loginHandler(HttpConnection *connection);
error_t login(HttpConnection *connection, User *user);
User* findLoggedInUser(HttpConnection *connection);
User* findUser(char_t *username, char_t *password);
void getRandomStr(char_t *output, int len);
char_t* parseFormField(char_t **dataPointer, char_t *field);

// ********************************************************************************************

void initSessionHandler()
{
   for (int i = 0; i < USER_COUNT; i++) {
      appEnv.users[i].sessionId[0] = '\0';
   }
}

// ********************************************************************************************

error_t loginHandler(HttpConnection *connection)
{
   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   if (!data) {
      ESP_LOGE(LOG_TAG, "loginHandler couldn't allocate memory!");
      return httpSendManual(
         connection, 500, "text/plain", "something went wrong!");
   }

   size_t length = 0;
   httpReadStream(
      connection, data, READ_STREAM_BUF_SIZE, &length, 0);
   data[length] = '\0';

   char_t* tmp = data;
   char_t* username = parseFormField(&tmp, "username");
   char_t* password = parseFormField(&tmp, "password");
   free(data);

   User *currentUser = findUser(username, password);
   free(username);
   free(password);

   if (currentUser) return login(connection, currentUser);
   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t login(HttpConnection *connection, User *user)
{
   getRandomStr(user->sessionId, SESSION_ID_LENGTH);
   user->sessionId[SESSION_ID_LENGTH] = 0;
   strcpy(connection->response.setCookie, user->sessionId);
   connection->response.setCookie[SESSION_ID_LENGTH] = 0;

   connection->response.location = "/index.html";
   error_t error = httpSendHeaderManual(connection, 302, NULL, 0);
   if (error) return error;
   return httpCloseStream(connection);
}

// ********************************************************************************************

User* findLoggedInUser(HttpConnection *connection)
{
   char_t* sessionId = connection->request.cookie;
   if (!sessionId[0]) return NULL;

   for (int i = 0; i < USER_COUNT; i++) {
      if (!strcmp(sessionId, appEnv.users[i].sessionId))
         return &(appEnv.users[i]);
   }

   return NULL;
}

// ********************************************************************************************

User* findUser(char_t *username, char_t *password)
{
   for (int i = 0; i < USER_COUNT; i++)
   {
      if (!strcmp(username, appEnv.users[i].username))
      {
         if (!strcmp(password, appEnv.users[i].password))
            return &(appEnv.users[i]);

         return NULL;
      }
   }
   return NULL;
}

// ********************************************************************************************

void getRandomStr(char_t *output, int len)
{
   static const char_t eligible_chars[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
   static const size_t pool_len = sizeof(eligible_chars)-1;

   for(int i = 0; i < len; i++){
      uint32_t rand_int = (uint32_t) esp_random();
      output[i] = eligible_chars[rand_int % pool_len];
   }
}

// ********************************************************************************************

char_t* parseFormField(char_t **dataPointer, char_t *field)
{
   char_t *data = *dataPointer;
   int flen = strlen(field);
   int dlen = strlen(data);

   if (dlen < flen+2)
      return NULL;

   if(strncmp(data, field, flen) || data[flen] != '=')
      return NULL;

   int vStart = flen + 1;
   int vEnd = vStart + 1;

   while (data[vEnd] && data[vEnd] != '&')
      vEnd += 1;

   char_t* val = (char_t*) malloc(vEnd - vStart + 1);
   strncpy(val, (char_t*)(data + vStart), vEnd-vStart);
   val[vEnd-vStart] = '\0';

   *dataPointer = data + vEnd;
   if (**dataPointer == '&')
      *dataPointer += 1;

   return val;
}

// ********************************************************************************************
