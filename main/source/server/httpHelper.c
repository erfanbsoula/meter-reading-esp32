#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "httpHelper.h"

#include "source/utils/cJSON.h"

// ********************************************************************************************

/**
 * send http response header to the client
 */
error_t httpSendHeaderManual(HttpConnection* connection,
   uint_t statusCode, char_t* contentType, size_t length)
{
   connection->response.version = connection->request.version;
   connection->response.statusCode = statusCode;
   connection->response.keepAlive = connection->request.keepAlive;
   connection->response.noCache = TRUE;
   connection->response.contentType = contentType;
   connection->response.chunkedEncoding = FALSE;
   connection->response.contentLength = length;

   // send the header to the client
   return httpWriteHeader(connection);
}

// ********************************************************************************************

/**
 * send message buffer to client
 * and safely close the connection
 * 
 * ! message is expected to be null-terminated !
 * doesn't free the message buffer after snding it
 */
error_t httpSendManual(HttpConnection *connection,
   int32_t statusCode, char_t *contentType, char_t *message)
{
   // send response header using the helper functions
   error_t error = httpSendHeaderManual(
      connection, statusCode, contentType, strlen(message)
   );
   if(error) return error;

   // send response body
   error = httpWriteStream(
      connection, message, connection->response.contentLength
   );
   if(error) return error;

   //Properly close output stream
   return httpCloseStream(connection);;
}

// ********************************************************************************************

/**
 * send message buffer to client and free the message buffer
 * then safely close the http connection
 * 
 * ! message is expected to be null-terminated !
 * message buffer won't be accessable after calling this function
 */
error_t httpSendAndFreeManual(HttpConnection *connection,
   int32_t statusCode, char_t *contentType, char_t *message)
{
   error_t error = httpSendManual(
      connection, statusCode, contentType, message
   );
   free(message);
   return error;
}

// ********************************************************************************************

/**
 * send message buffer to client
 * set content-type to json
 * then safely close the http connection
 * 
 * ! message is expected to be null-terminated !
 * doesn't free the message buffer after snding it
 */
error_t httpSendJsonManual(HttpConnection *connection,
   int32_t statusCode, char_t *message)
{
   return httpSendManual(
      connection, statusCode, "application/json", message
   );
}

// ********************************************************************************************

/**
 * send message buffer to client and free the message buffer
 * set content-type to json
 * then safely close the http connection
 * 
 * ! message is expected to be null-terminated !
 * message buffer won't be accessable after calling this function
 */
error_t httpSendJsonAndFreeManual(HttpConnection *connection,
   int32_t statusCode, char_t *message)
{
   error_t error = httpSendJsonManual(
      connection, statusCode, message
   );
   free(message);
   return error;
}

// ********************************************************************************************

/**
 * send rejection message to client in json format
 * with a "Bad Request" status code
 * then safely close the http connection
 */
error_t apiSendRejectionManual(HttpConnection *connection)
{
   cJSON *res = cJSON_CreateObject();
   char_t *jsonStr = NULL;

   cJSON_AddNumberToObject(res, "status", 0);
   cJSON_AddStringToObject(res, "message", "request rejected!");
   jsonStr = cJSON_Print(res);
   cJSON_Delete(res);
   ESP_LOGI("API", "request rejected!");
   return httpSendJsonAndFreeManual(
      connection, 400, jsonStr
   );
}

// ********************************************************************************************

/**
 * send message to client in json format
 * with a "HTTP OK" status code
 * then safely close the http connection
 * 
 * ! message is expected to be null-terminated !
 */
error_t apiSendSuccessManual(HttpConnection *connection, char_t *message)
{
   cJSON *res = cJSON_CreateObject();
   char_t *jsonStr = NULL;

   cJSON_AddNumberToObject(res, "status", 1);
   cJSON_AddStringToObject(res, "message", message);
   jsonStr = cJSON_Print(res);
   cJSON_Delete(res);
   ESP_LOGI("API", "request ok!");
   return httpSendJsonAndFreeManual(
      connection, 200, jsonStr
   );
}

// ********************************************************************************************
