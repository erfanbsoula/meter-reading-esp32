#include "../includes.h"
#include "handlers.h"
#include "../envTypes.h"
#include "../appEnv.h"
#include "../serial/uartHelper.h"
#include "../server/httpHelper.h"

static const char_t *LOG_TAG = "readMeter";

// ********************************************************************************************
// forward declaration of functions

error_t getAIHandler(HttpConnection *connection);
bool_t getAiHelper(char_t *res);
bool_t checkAiResponseHelper();

// ********************************************************************************************

/**
 * handler function for serving the ai results
 * 
 * this function will request the ai result from k210
 * over UART and send the actual reading over api
 */
error_t getAIHandler(HttpConnection *connection)
{
   if (strcmp(connection->request.method, "GET"))
      return ERROR_NOT_FOUND;

   ESP_LOGI(LOG_TAG, "AI result requested!");
   if (!uartAcquire(50))
      return apiSendRejectionManual(connection);

   char_t tmp[11];
   bool_t res = getAiHelper(tmp);

   uartRelease();

   if (!res)
      return apiSendRejectionManual(connection);

   return apiSendSuccessManual(connection, tmp);
}

// ********************************************************************************************

/**
 * requests and recieves the AI reading over uart communication
 */
bool_t getAiHelper(char_t *res)
{
   uint8_t *buffer = uartGetBuffer();

   vTaskDelay(100 / portTICK_PERIOD_MS);
   uartSendBytes("AIread:1", 8);
   uint8_t *hanshake = uartReadBytesSync(4, 400);
   if (!hanshake || !strcmp((char*) hanshake, "done"))
   {
      ESP_LOGE(LOG_TAG, "handshaking failed!");
      return false;
   }

   uartClearBuffer();
   uartSendBytes("AIsend:1", 8);
   if (!waitForBuffer(5 + appEnv.imgConfig.digitCount, 300)) {
      ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
      return FALSE;
   }

   buffer[5 + appEnv.imgConfig.digitCount] = 0;
   ESP_LOGI("UART", "recieved '%s'", (char_t*)buffer);
   if (checkAiResponseHelper()) {
      ESP_LOGE("UART", "k210 sent invalid response for ai request");
      return FALSE;
   }

   strncpy(res, (char_t*)(buffer+4), appEnv.imgConfig.digitCount);
   res[appEnv.imgConfig.digitCount] = '\0';
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

   return buffer[4 + appEnv.imgConfig.digitCount] != ';' ||
      strcmp(tmp, "num:");
}

// ********************************************************************************************
