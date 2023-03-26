#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "handlers.h"
#include "source/network/network.h"
#include "source/utils/netConfigParser.h"
#include "source/serial/uartHelper.h"
#include "source/server/httpHelper.h"
#include "source/appEnv.h"
#include "source/storage/storage.h"
#include "source/utils/imgConfigParser.h"
#include "source/mqtt/mqttConfigParser.h"
#include "source/utils/cJSON.h"
#include "esp_log.h"

static const uint_t READ_STREAM_BUF_SIZE = 511;
static const char_t *LOG_TAG = "configHandler";

// ********************************************************************************************
// forward declaration of functions

error_t imgConfigHandler(HttpConnection *connection);
bool_t sendConfigToK210(ImgConfig *imgConfig);
error_t mqttConfigHandler(HttpConnection *connection);

// ********************************************************************************************

/**
 * handler function for parsing the incoming post requests
 * and extracting the config data
 * then sending it to the k210 over UART communication api
 */
error_t imgConfigHandler(HttpConnection *connection)
{
   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   if (!uartAcquire(50))
      return apiSendRejectionManual(connection);

   bool_t parsingResult = FALSE;
   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   if (data)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseImgConfig(&(appEnv.imgConfig), data);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   if (parsingResult)
   {
      free(appEnv.meterCounter);
      appEnv.meterCounter = NULL;
      appEnv.imgConfig.isConfigured = TRUE;
      cJSON_Minify(data);
      saveImgConfigJson(data);
      sendConfigToK210(&(appEnv.imgConfig));
   }

   uartRelease();
   free(data);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

/**
 * this function will send the k210Config object to k210
 * over UART as a ':' seperated string 
 */
bool_t sendConfigToK210(ImgConfig *imgConfig)
{
   char_t* str = (char_t*) malloc(300);
   char_t* tmp = (char_t*) malloc(20);
   sprintf(str, "config:%d:%d",
      imgConfig->digitCount, imgConfig->invert ? 1 : 0);

   for (uint_t i = 0; i < imgConfig->digitCount; i++)
   {
      sprintf(tmp, ":%d:%d:%d:%d", 
         imgConfig->positions[i].x,
         imgConfig->positions[i].y,
         imgConfig->positions[i].width,
         imgConfig->positions[i].height
      );
      strcat(str, tmp);
   }
   free(tmp);

   uartSendString(str);
   ESP_LOGI(LOG_TAG, "uart sent %s", str);
   free(str);

   uint8_t *hanshake = uartReadBytesSync(8, 500);
   if (!hanshake || !strcmp((char*) hanshake, "recieved"))
   {
      ESP_LOGE(LOG_TAG, "handshaking failed!");
      return false;
   }
   ESP_LOGI(LOG_TAG, "handshaking successful!");
   return true;
}

// ********************************************************************************************

error_t mqttConfigHandler(HttpConnection *connection)
{
   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   bool_t parsingResult = FALSE;

   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   MqttConfig *mqttConfigTmp = malloc(sizeof(MqttConfig));

   if (data && mqttConfigTmp)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseMqttConfig(mqttConfigTmp, data);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   if (parsingResult)
   {
      cJSON_Minify(data);
      saveMqttConfigJson(data);
   }

   free(data);
   free(mqttConfigTmp);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t netConfigHandler(HttpConnection *connection,
   NetworkType interface)
{
   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   bool_t parsingResult = FALSE;

   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   NetworkConfig *netConfig = malloc(sizeof(NetworkConfig));

   if (data && netConfig)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseNetConfig(netConfig, data, interface);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   if (parsingResult)
   {
      cJSON_Minify(data);
      saveNetConfigJson(data, interface);
   }

   free(data);
   free(netConfig);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************
