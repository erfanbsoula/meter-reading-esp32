#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "handlers.h"
#include "source/storage/storage.h"
#include "source/serial/uartHelper.h"
#include "source/server/httpHelper.h"
#include "source/network/netConfigParser.h"
#include "source/utils/imgConfigParser.h"
#include "source/mqtt/mqttConfigParser.h"
#include "esp_log.h"
#include "source/appEnv.h"

static const uint_t READ_STREAM_BUF_SIZE = 511;
static const char_t *LOG_TAG = "configHandler";

// ********************************************************************************************
// forward declaration of functions

error_t imgConfigHandler(HttpConnection *connection);
bool_t sendConfigToK210(ImgConfig *imgConfig);
error_t mqttConfigHandler(HttpConnection *connection);
error_t lanConfigHandler(HttpConnection *connection);
error_t staWifiConfigHandler(HttpConnection *connection);
error_t apWifiConfigHandler(HttpConnection *connection);

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
      parsingResult = parseImgConfig(&appEnv.imgConfig, data);
      free(data);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   if (parsingResult)
   {
      appEnv.meterCounter[0] = '\0';
      saveImgConfig(&appEnv.imgConfig);
      sendConfigToK210(&appEnv.imgConfig);
   }

   uartRelease();

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
   char_t* str = (char_t*) malloc(MAX_DIGIT_COUNT * 30 + 20);
   char_t* tmp = (char_t*) malloc(30);
   sprintf(str, "config:%"PRIu8":%"PRIu8,
      imgConfig->digitCount, imgConfig->invert ? 1 : 0);

   for (uint_t i = 0; i < imgConfig->digitCount; i++)
   {
      sprintf(tmp, ":%"PRIu16":%"PRIu16":%"PRIu16":%"PRIu16, 
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
   if (!strcmp(connection->request.method, "GET"))
   {
      char_t *data = mqttConfigToJson(&appEnv.mqttConfig);
      if (!data) return apiSendRejectionManual(connection);
      return httpSendJsonAndFreeManual(connection, 200, data);
   }

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
      if (parsingResult) saveMqttConfig(mqttConfigTmp);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   free(data);
   free(mqttConfigTmp);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t lanConfigHandler(HttpConnection *connection)
{
   if (!strcmp(connection->request.method, "GET"))
   {
      char_t *data = lanConfigToJson(&appEnv.lanConfig);
      if (!data) return apiSendRejectionManual(connection);
      return httpSendJsonAndFreeManual(connection, 200, data);
   }

   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   bool_t parsingResult = FALSE;

   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   LanConfig *lanConfigTmp = malloc(sizeof(LanConfig));

   if (data && lanConfigTmp)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseLanConfig(lanConfigTmp, data);
      if (parsingResult) saveLanConfig(lanConfigTmp);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   free(data);
   free(lanConfigTmp);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t staWifiConfigHandler(HttpConnection *connection)
{
   if (!strcmp(connection->request.method, "GET"))
   {
      char_t *data = staWifiConfigToJson(&appEnv.staWifiConfig);
      if (!data) return apiSendRejectionManual(connection);
      return httpSendJsonAndFreeManual(connection, 200, data);
   }

   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   bool_t parsingResult = FALSE;

   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   StaWifiConfig *staWifiConfigTmp = malloc(sizeof(StaWifiConfig));

   if (data && staWifiConfigTmp)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseStaWifiConfig(staWifiConfigTmp, data);
      if (parsingResult) saveStaWifiConfig(staWifiConfigTmp);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   free(data);
   free(staWifiConfigTmp);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

error_t apWifiConfigHandler(HttpConnection *connection)
{
   if (!strcmp(connection->request.method, "GET"))
   {
      char_t *data = apWifiConfigToJson(&appEnv.apWifiConfig);
      if (!data) return apiSendRejectionManual(connection);
      return httpSendJsonAndFreeManual(connection, 200, data);
   }

   if (strcmp(connection->request.method, "POST"))
      return ERROR_NOT_FOUND;

   bool_t parsingResult = FALSE;

   char_t *data = (char_t*) malloc(READ_STREAM_BUF_SIZE+1);
   ApWifiConfig *apWifiConfigTmp = malloc(sizeof(ApWifiConfig));

   if (data && apWifiConfigTmp)
   {
      size_t length = 0;
      httpReadStream(connection, data, READ_STREAM_BUF_SIZE, &length, 0);
      data[length] = '\0';
      parsingResult = parseApWifiConfig(apWifiConfigTmp, data);
      if (parsingResult) saveApWifiConfig(apWifiConfigTmp);
   }
   else ESP_LOGE(LOG_TAG, "couldn't allocate memory!");

   free(data);
   free(apWifiConfigTmp);

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************
