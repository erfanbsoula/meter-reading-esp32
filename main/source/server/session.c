#include "../includes.h"
#include "session.h"
#include "esp_random.h"
#include "httpHelper.h"

#define READ_STREAM_BUF_SIZE 64

// stores a pointer to the users array for easier access
static User *users = NULL;

// ********************************************************************************************
// functions pre declaration
error_t login(User *user, HttpConnection *connection);
void getRandomStr(char_t *output, int len);
char_t* parseFormField(char_t **dataPointer, char_t *field);
User* findUser(char_t *username, char_t *password);

// ********************************************************************************************

void initSessionHandler(User *users_)
{
   users = users_;
   for (int i = 0; i < USER_COUNT; i++) {
      users[i].sessionId[0] = '\0';
   }
}

// ********************************************************************************************

error_t loginHandler(HttpConnection *connection)
{
   char_t* data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   if (!data) {
      ESP_LOGE("API", "login handler couldn't allocate memory!");
      return httpSendManual(
         connection, 500, "text/plain", "something went wrong!");
   }

   size_t length = 0;
   httpReadStream(
      connection, data, READ_STREAM_BUF_SIZE, &length, 0);
   data[length] = 0;

   char_t* tmp = data;
   char_t* username = parseFormField(&tmp, "username");
   char_t* password = parseFormField(&tmp, "password");
   free(data);

   User *currentUser = findUser(username, password);
   free(username);
   free(password);

   if (currentUser) return logIn(connection);
   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t login(User *user, HttpConnection *connection)
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

User* hasLoggedIn(HttpConnection *connection)
{
   char_t* sessionId = connection->request.cookie;
   if (!sessionId[0]) return NULL;

   for (int i = 0; i < USER_COUNT; i++) {
      if (!strcmp(sessionId, users[i].sessionId))
         return &users[i];
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

User* findUser(char_t *username, char_t *password)
{
   for (int i = 0; i < USER_COUNT; i++)
   {
      if (!strcmp(username, users[i].username))
      {
         if (!strcmp(password, users[i].password))
            return &users[i];

         return NULL;
      }
   }
   return NULL;
}

// ********************************************************************************************