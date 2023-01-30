#include "../includes.h"
#include "read.h"

/**
 * handler function for serving the ai results
 * 
 * this function will request the ai result from k210
 * over UART and send the actual reading over api
 */
error_t getAIHandler(HttpConnection* connection)
{
   if (strcmp(connection->request.method, "GET"))
      return ERROR_NOT_FOUND;

   ESP_LOGI("API", "AI result requested!");
   if (uartBusy)
      return apiSendRejectionManual(connection);


   char_t tmp[11];
   bool_t res = getAiHelper(tmp);

   if (!res)
      return apiSendRejectionManual(connection);

   return apiSendSuccessManual(connection, tmp);
}

// ********************************************************************************************

/**
 * requests and recieves the AI reading over uart communication
 * handles'uartBusy' flag properly
 */
bool_t getAiHelper(char_t* res)
{
   if (uartBusy) return FALSE;

   uartBusy = TRUE;
   bool_t flag = getAiHelper_unsafe(res);

   uartBusy = FALSE;
   return flag;
}

// ********************************************************************************************

/**
 * requests and recieves the AI reading over uart communication
 * it does not handle 'uartBusy' flag!
 */
bool_t getAiHelper_unsafe(char_t* res)
{
   uint8_t* buffer = uartGetBuffer();

   vTaskDelay(100 / portTICK_PERIOD_MS);
   uartSendBytes("AIread:1", 8);
   vTaskDelay(400 / portTICK_PERIOD_MS);

   uartClearBuffer();
   uartSendBytes("AIsend:1", 8);
   if (!waitForBuffer(5 + myEnv.k210config.digitCount, 10)) {
      ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
      return FALSE;
   }

   buffer[5 + myEnv.k210config.digitCount] = 0;
   ESP_LOGI("UART", "recieved '%s'", (char_t*)buffer);
   if (checkAiResponseHelper()) {
      ESP_LOGE("UART", "k210 sent invalid response for ai request");
      return FALSE;
   }

   strncpy(res, (char_t*)(buffer+4), myEnv.k210config.digitCount);
   res[myEnv.k210config.digitCount] = '\0';
   return TRUE;
}

// ********************************************************************************************

/**
 * validates the k210 response for ai result request
 */
bool_t checkAiResponseHelper()
{
   uint8_t* buffer = uartGetBuffer();

   char_t tmp[5];
   strncpy(tmp, (char_t*)buffer, 4);
   tmp[4] = '\0';

   return buffer[4 + myEnv.k210config.digitCount] != ';' ||
      strcmp(tmp, "num:");
}

// ********************************************************************************************
