#include "../includes.h"
#include "_defines.h"
#include "storage.h"
#include "nvsHelper.h"
#include "retrieve.h"

// ********************************************************************************************
// Global Variables

Environment appEnv;
void initStorage() { retrieveEnvironment(&appEnv); }

// ********************************************************************************************

void storage_ClearSessionIds()
{
	for (int i = 0; i < USER_COUNT; i++)
      appEnv.users[i].sessionId[0] = '\0';
}

char_t* storage_GetUserSessionId(uint_t userIndx)
{
	return appEnv.users[userIndx].sessionId;
}

// ********************************************************************************************

bool_t storage_UserExists(char_t *username, char_t *password)
{
   for (int i = 0; i < USER_COUNT; i++)
	{
      if (!strcmp(username, appEnv.users[i].username))
      {
         if (!strcmp(password, appEnv.users[i].password))
            return true;

         return false;
      }
   }
   return false;
}

// ********************************************************************************************
