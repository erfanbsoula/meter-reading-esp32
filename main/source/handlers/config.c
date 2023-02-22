#include "../includes.h"
#include "handlers.h"

/**
 * handler function for parsing the incoming post requests
 * and extracting the config data
 * then sending it to the k210 over UART communication api
 */
error_t configHandler(HttpConnection* connection)
{
   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   ESP_LOGI("API", "configs post request recieved!");

   if (uartBusy)
      return apiSendRejectionManual(connection);

   uartBusy = TRUE;
   bool_t parsingResult = FALSE;

   char_t* data = (char_t*) malloc(500);
   if (data)
   {
      size_t length = 0;
      httpReadStream(connection, data, 499, &length, 0);
      data[length] = 0;
      parsingResult = parseK210Configs(&myEnv.k210config, data);
   }
   else ESP_LOGE("API", "Config Parser couldn't allocate memory!");

   if (parsingResult)
   {
      sendConfigToK210();
      nvsSaveString(NVS_k210config_VAR, data);
      readingValid = FALSE;
      myEnv.k210config.isConfigured = TRUE;
   }

   free(data);
   uartBusy = FALSE;

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

/**
 * this function will send the k210Config object to k210
 * over UART as a ':' seperated string 
 */
bool_t sendConfigToK210()
{
   char_t* str = (char_t*) malloc(300);
   char_t* tmp = (char_t*) malloc(20);
   sprintf(str, "Config:%d:%d", myEnv.k210config.digitCount, myEnv.k210config.invert ? 1 : 0);
   for (uint_t i = 0; i < myEnv.k210config.digitCount; i++)
   {
      sprintf(tmp, ":%d:%d:%d:%d", 
         myEnv.k210config.positions[i].x,
         myEnv.k210config.positions[i].y,
         myEnv.k210config.positions[i].width,
         myEnv.k210config.positions[i].height
      );
      strcat(str, tmp);
   }
   free(tmp);

   uartSendString(str);
   ESP_LOGI("UART", "sent %s", str);
   free(str);
   return TRUE;
}

// ********************************************************************************************
