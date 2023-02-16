#include "includes.h"
#include "restore.h"

void initialRestore()
{
   retrieveUsers();

   char_t* data;
   if (nvsReadString(NVS_k210config_VAR, &data))
   {
      if (parseK210Configs(&myEnv.k210config, data))
      {
         ESP_LOGI("StartUp", "retrieved k210config successfully!");
         #if (AUTOMATED_READING == ENABLED)
         k210StartupConfig();
         #endif
      }
      else ESP_LOGE("StartUp", "failed to parse k210config!");
      free(data);
   }

   if(!nvsReadString(NVS_aiReading_VAR, &myEnv.aiReading))
      myEnv.aiReading = NULL;
   else if (strlen(myEnv.aiReading) != myEnv.k210config.digitCount)
   {
      ESP_LOGI("StartUp",
         "retrieved aiReading doesn't match k210config");
      free(myEnv.aiReading);
      myEnv.aiReading = NULL;
   }
   else ESP_LOGI("StartUp", "{%s = %s} retrieved successfully!",
      NVS_aiReading_VAR, myEnv.aiReading);
}

// ********************************************************************************************

void retrieveUsers()
{
   size_t usernameLen = sizeof(NVS_username_VAR);
   char_t *username = (char_t*) malloc(usernameLen);
   strcpy(username, NVS_username_VAR);

   size_t passwordLen = sizeof(NVS_password_VAR);
   char_t *password = (char_t*) malloc(passwordLen);
   strcpy(password, NVS_password_VAR);

   for (size_t i = 0; i < USER_COUNT; i++)
   {
      bool_t found;

      username[usernameLen-2] = '0' + i;
      found = nvsReadString(username, &myEnv.users[i].username);
      if (!found)
      {
         size_t defaultLen = sizeof(DEFAULT_USERNAME);
         char_t *defaultStr = (char_t*) malloc(defaultLen);
         strcpy(defaultStr, DEFAULT_USERNAME);
         defaultStr[defaultLen-2] = '0' + i;
         myEnv.users[i].username = defaultStr;
         nvsSaveString(username, defaultStr);
      }

      password[passwordLen-2] = '0' + i;
      found = nvsReadString(password, &myEnv.users[i].password);
      if (!found)
      {
         size_t defaultLen = sizeof(DEFAULT_PASSWORD);
         char_t *defaultStr = (char_t*) malloc(defaultLen);
         strcpy(defaultStr, DEFAULT_PASSWORD);
         myEnv.users[i].password = defaultStr;
         nvsSaveString(password, defaultStr);
      }
   }

   free(username);
   free(password);
}

// ********************************************************************************************
