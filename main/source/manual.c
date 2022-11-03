#include "includes.h"
#include "manual.h"
#include "httpHelper.h"
#include "uartHelper.h"
#include "myNVS.h"

// ********************************************************************************************

// number of bytes to expect when requesing image
#define TOTAL_SIZE 76800

// NVS variable names
#define NVS_AI_RESULT_VAR_NAME "AiRes"
#define NVS_AI_CONFIG_VAR_NAME "AiConfig"

// ********************************************************************************************
// Global Variables

/**
 * in each callback function that wants to use the UART serial communication with k210,
 * check this flag and if set, return immediately.
 * 
 * if not set, you must set the flag at the beginning of the process
 * and unset the flag after the process is done.
 * 
 * **! remember to unset the flag in case of process termination due to errors !**
 */
bool_t uartBusy = FALSE;

// will hold the AI-config data recieved from the client
K210config k210config;
bool_t isConfigured = FALSE;

// will hold the AI-reading result recieved from k210
char_t* aiReading;
bool_t readingValid = FALSE;

// ********************************************************************************************
// forward declaration of functions

void aiRetrieveState();
void aiTask(void *pvParameters);
bool_t parseConfigs(char_t* data);
bool_t sendConfigToK210();
bool_t getAiHelper(char_t* res);

// ********************************************************************************************

void initManual()
{
   // we have no positions saved in th startup
   // so the pointer should be null
   k210config.positions = NULL;

   // initialize serial communication and serial task
   serialInit();

   // retrieve last configuration and reading if possible
   aiRetrieveState();   
}

// ********************************************************************************************

void aiRetrieveState()
{
   char_t* data;
   bool_t result = nvsReadString(NVS_AI_CONFIG_VAR_NAME, &data);
   if (!result) return;

   result = parseConfigs(data);
   if (result) {
      while (uartBusy) {
         ESP_LOGE("Init", "unexpected usage on uart!");
         vTaskDelay(200 / portTICK_PERIOD_MS);
      }
      uartBusy = TRUE;
      isConfigured = sendConfigToK210();
      uartBusy = FALSE;
   }
   free(data);

   if (!isConfigured) {
      ESP_LOGI("Init", "failed to configure k210!");
      return;
   }
   ESP_LOGI("Init", "configured k210 successfully!");

   readingValid = nvsReadString(NVS_AI_RESULT_VAR_NAME, &aiReading);
   if (!readingValid) return;

   if (strlen(aiReading) != k210config.digitCount) {
      ESP_LOGI("Init", "retrieved AI-reading doesn't match the config!");
      free(aiReading);
      readingValid = FALSE;
      return;
   }

   ESP_LOGI("NVS", "{%s = %s}", NVS_AI_RESULT_VAR_NAME, aiReading);
   ESP_LOGI("Init", "retrieved last reading successfully!");
}

// ai task always running to keep the AI reading updated
void aiTask(void *pvParameters)
{
   while(1) {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      if (!isConfigured) continue;

      if (!readingValid) {
         free(aiReading);
         aiReading = (char_t*) malloc(k210config.digitCount);
      }

      readingValid = getAiHelper(aiReading);
      if(readingValid) {
         ESP_LOGI("aiTask", "got current AI-reading successfully!");
      }
      else ESP_LOGI("aiTask", "couldn't get AI-reading!");
   }
}

// ********************************************************************************************

/**
 * helper function for parseConfigs error
 * makes sure to free the memory used by the CJSON parser
 */
bool_t parseFailCleanUp(cJSON* root)
{
   cJSON_Delete(root);
   return FALSE;
}

bool_t parseConfigs(char_t* data) {
   cJSON* root = cJSON_Parse(data);
   if (root == NULL)
   {
      const char_t *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
         ESP_LOGE("CJSON", "error before: %s", error_ptr);

      return parseFailCleanUp(root);
   }

   // *******************************************************
   cJSON* node = cJSON_GetObjectItem(root, "digitCount");
   if (!cJSON_IsNumber(node))
      return parseFailCleanUp(root);
   k210config.digitCount = cJSON_GetNumberValue(node);
   
   node = cJSON_GetObjectItem(root, "invert");
   if (!cJSON_IsBool(node))
      return parseFailCleanUp(root);
   k210config.invert = !cJSON_IsFalse(node);

   node = cJSON_GetObjectItem(root, "rectanglePositions");
   if (!cJSON_IsArray(node))
      return parseFailCleanUp(root);

   // free the allocated memory by previous execution
   // and allocate the memory needed to store new digit positions
   free(k210config.positions);
   k210config.positions = (Position*) malloc(sizeof(Position) * k210config.digitCount);
   if (k210config.positions == NULL)
   {
      ESP_LOGE("CJSON", "unable to allocate memory!");
      return parseFailCleanUp(root);
   }

   // loop throw each digit's position property
   // and parse the x, y, width and height values
   for (int_t i = 0; i < k210config.digitCount; i++)
   {
      // get the (i)th digit's position property
      cJSON* child = cJSON_GetArrayItem(node, i);
      if (child == NULL)
         return parseFailCleanUp(root);

      cJSON* attribute = cJSON_GetObjectItem(child, "x");
      if (!cJSON_IsNumber(attribute))
         return parseFailCleanUp(root);
      k210config.positions[i].x = cJSON_GetNumberValue(attribute);

      attribute = cJSON_GetObjectItem(child, "y");
      if (!cJSON_IsNumber(attribute))
         return parseFailCleanUp(root);
      k210config.positions[i].y = cJSON_GetNumberValue(attribute);

      attribute = cJSON_GetObjectItem(child, "width");
      if (!cJSON_IsNumber(attribute))
         return parseFailCleanUp(root);
      k210config.positions[i].width = cJSON_GetNumberValue(attribute);

      attribute = cJSON_GetObjectItem(child, "height");
      if (!cJSON_IsNumber(attribute))
         return parseFailCleanUp(root);
      k210config.positions[i].height = cJSON_GetNumberValue(attribute);
   }

   cJSON_Delete(root);
   return TRUE;
}

/**
 * this function will send the k210Config object to k210
 * over UART as a ':' seperated string 
 */
bool_t sendConfigToK210()
{
   char_t* str = (char_t*) malloc(300);
   char_t* tmp = (char_t*) malloc(20);
   sprintf(str, "Config:%d:%d", k210config.digitCount, k210config.invert ? 1 : 0);
   for (uint_t i = 0; i < k210config.digitCount; i++)
   {
      sprintf(tmp, ":%d:%d:%d:%d", 
         k210config.positions[i].x,
         k210config.positions[i].y,
         k210config.positions[i].width,
         k210config.positions[i].height
      );
      strcat(str, tmp);
   }
   free(tmp);

   uartSendString(str);
   ESP_LOGI("UART", "sent %s", str);
   free(str);
   return TRUE;
}

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
      parsingResult = parseConfigs(data);
   }
   else ESP_LOGE("API", "Config Parser couldn't allocate memory!");

   if (parsingResult)
   {
      sendConfigToK210();
      nvsSaveString(NVS_AI_CONFIG_VAR_NAME, data);
   }

   free(data);
   uartBusy = FALSE;

   if (parsingResult)
      return apiSendSuccessManual(connection, "Configs Recieved!");

   return apiSendRejectionManual(connection);
}

// ********************************************************************************************

/**
 * encodes the current data in the UART buffer
 * and sends it as a base16 string (manual encoding)
 */
error_t sendChunk(HttpConnection* connection)
{
   error_t error;
   uint8_t* tmp_buf = (uint8_t*) malloc(4096);
   if (tmp_buf == NULL)
      ESP_LOGE("API", "couldn't allocate memory");
      //! handle this error

   uint8_t* buffer = uartGetBuffer();
   size_t bufLen = uartGetBufLength();

   uint32_t counter = 0;
   while(counter < bufLen)
   {
      uint32_t i = 0;
      for (i = 0; i < 2048 && counter+i < bufLen; i++)
      {
         tmp_buf[2*i] = buffer[counter+i] / 16 + 48;
         tmp_buf[2*i + 1] = buffer[counter+i] % 16 + 48;
      }
      counter += i;
      error = httpWriteStream(connection, tmp_buf, 2*i);
      if(error) {
         free(tmp_buf);
         return error;
      }
   }

   free(tmp_buf);
   return NO_ERROR;
}

/**
 * calculates the chunk size to request from k210
 * such that no overflow occurs
 */
size_t findChunkSize(size_t size_count)
{
   if (size_count + BUFFER_SIZE <= TOTAL_SIZE)
      return BUFFER_SIZE;
   else
      return TOTAL_SIZE - size_count;
}

/**
 * requests the camera image from k210 over UART
 * in chunks equal to the buffer size and sends each chunk
 * to the client immediately using the sendChunk function.
 * 
 * closes the http connection if k210 doesn't respond
 */
error_t getAndSendCameraImg(HttpConnection* connection)
{
   size_t size_count = 0, chunk_size = 0;
   char_t cmdStr[20];

   vTaskDelay(200 / portTICK_PERIOD_MS);
   uartSendBytes("GetNew:1", 8);
   vTaskDelay(400 / portTICK_PERIOD_MS);

   while (size_count < TOTAL_SIZE)
   {
      uartClearBuffer();
      chunk_size = findChunkSize(size_count);

      if (size_count == 0) // if first request
         sprintf(cmdStr, "Start:%d", chunk_size);
      else // if it's not the first request ...
         sprintf(cmdStr, "Next:%d", chunk_size);

      uartSendString(cmdStr);
      if (!waitForBuffer(chunk_size, 10)) {
         ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
         return NO_ERROR;
      }
      ESP_LOGI("API", "read chunk with size %d", uartGetBufLength());

      error_t error = sendChunk(connection);
      if (error) return error;
      size_count += chunk_size;
   }

   uartClearBuffer();
   vTaskDelay(100 / portTICK_PERIOD_MS);
   return NO_ERROR;
}

/**
 * handler function for serving the camera image over a manual api.
 * 
 * this function will send the http header to the client
 * and then call the getAndSendCameraImg function
 * to do the rest of the job!
 * 
 * if there are any errors in the getAndSendCameraImg function,
 * client will face a CONTENT_LENGTH_MISMATCH error that should
 * be handled properly on the client-side
 */
error_t cameraImgHandler(HttpConnection* connection)
{
   if (strcmp(connection->request.method, "GET"))
      return ERROR_NOT_FOUND;

   ESP_LOGI("API", "camera image requested!");
   if (uartBusy)
      return apiSendRejectionManual(connection);

   uartBusy = TRUE;

   error_t error = httpSendHeaderManual(
      connection, 200, "text/plain", TOTAL_SIZE*2
   );
   if(error) {
      uartBusy = FALSE;
      return error;
   }

   error = getAndSendCameraImg(connection);
   if(error) {
      uartBusy = FALSE;
      return error;
   }

   uartBusy = FALSE;
   ESP_LOGI("API", "image task done! closing http connection");
   return httpCloseStream(connection);
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

   return buffer[4 + k210config.digitCount] != ';' ||
      strcmp(tmp, "num:");
}

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
   if (!waitForBuffer(5 + k210config.digitCount, 10)) {
      ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
      return FALSE;
   }

   buffer[5 + k210config.digitCount] = 0;
   ESP_LOGI("UART", "recieved '%s'", (char_t*)buffer);
   if (checkAiResponseHelper()) {
      ESP_LOGE("UART", "k210 sent invalid response for ai request");
      return FALSE;
   }

   strncpy(res, (char_t*)(buffer+4), k210config.digitCount);
   res[k210config.digitCount] = '\0';
   return TRUE;
}

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
 * manual router function for incoming http requests.
 * (uri is a null-terminated string)
 */
error_t httpServerManualRouter(HttpConnection *connection, const char_t *uri)
{
   if (!strcmp(uri, "/config"))
      return configHandler(connection);

   if (!strcmp(uri, "/camera"))
      return cameraImgHandler(connection);

   if (!strcmp(uri, "/ai"))
      return getAIHandler(connection);

   return ERROR_NOT_FOUND;
}
