#include "includes.h"
#include "source.h"
#include "manual.h"
#include "httpHelper.h"
#include "uartHelper.h"
#include "nvsHelper.h"
#include "myMqtt.h"
#include "session.h"
#include "cJSON.h"

// ********************************************************************************************

// number of bytes to expect when requesing image
#define TOTAL_SIZE 76800

// automatic reading
#define AUTOMATED_READING DISABLED

// NVS variable names
#define NVS_k210config_VAR "k210config"
#define NVS_aiReading_VAR "aiReading"
#define NVS_username_VAR "username#"
#define NVS_password_VAR "password#"

#define DEFAULT_USERNAME "admin#"
#define DEFAULT_PASSWORD "12345678"

// ********************************************************************************************
// Global Variables

Environment myEnv;

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

// will hold the AI-reading result recieved from k210
char_t *aiReading;
bool_t readingValid = FALSE;

// ********************************************************************************************
// forward declaration of functions

void initEnvironment();
void aiTask(void *pvParameters);
bool_t parseK210Configs(K210config *k210config, char_t *data);
bool_t sendConfigToK210();
bool_t getAiHelper(char_t* res);

// ********************************************************************************************

void initManual()
{
   // initialize serial communication and serial task
   serialInit();

   // initilize myEnv object
   initEnvironment();

   initSessionHandler();

   // initialize AI Event Task
   // BaseType_t ret = xTaskCreatePinnedToCore(
   //    aiTask, "aiTask", 2048, NULL, 12, NULL, 1
   // );
   // if(ret != pdPASS) { // error check
   //    ESP_LOGE("aiTask","failed to create task!");
   // }

   // initialize mqtt task
   // ret = xTaskCreatePinnedToCore(
   //    mqttTask, "mqttTask", 2048, NULL, 12, NULL, 1
   // );
   // if(ret != pdPASS) { // error check
   //    ESP_LOGE("mqttTask","failed to create task!");
   // }
}

// ********************************************************************************************
void retrieveUsers();
void k210StartupConfig();

/**
 * initialize global Environment object 'myEnv' on startup
 */
void initEnvironment()
{
   // k210 is not configured on startup
   myEnv.k210config.isConfigured = FALSE;
   myEnv.k210config.positions = NULL;

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

// handle this function later
void k210StartupConfig()
{
   while (uartBusy) {
      ESP_LOGE("StartUp", "unexpected usage on uart!");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
   }
   uartBusy = TRUE;
   sendConfigToK210();
   // myEnv.k210config.isConfigured = sendConfigToK210();
   uartBusy = FALSE;
}

// ********************************************************************************************

// ai task always running to keep the AI reading updated
void aiTask(void *pvParameters)
{
   while(1) {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      if (!myEnv.k210config.isConfigured) {
         ESP_LOGI("aiTask", "waiting for configuration");
         continue;
      }

      if (!readingValid) {
         free(aiReading);
         aiReading = (char_t*) malloc(myEnv.k210config.digitCount);
      }

      readingValid = getAiHelper(aiReading);
      if (readingValid) {
         ESP_LOGI("aiTask", "got current AI-reading successfully!");
         bool_t result;
         result = nvsSaveString(NVS_aiReading_VAR, aiReading);
         if (!result) {
            ESP_LOGI("aiTask", "couldn't save AI-reading");
         }
      }
      else ESP_LOGI("aiTask", "couldn't get AI-reading");
   }
}

// ********************************************************************************************
// config parser

bool_t extractPosition(cJSON *child, Position *position);
bool_t parseFailCleanUp(cJSON* root);

/**
 * parse the configuration data stored as json string
 * and fill 'k210config' with the result
 */
bool_t parseK210Configs(K210config *k210config, char_t *data)
{
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
   k210config->digitCount= cJSON_GetNumberValue(node);

   node = cJSON_GetObjectItem(root, "invert");
   if (!cJSON_IsBool(node))
      return parseFailCleanUp(root);
   k210config->invert = !cJSON_IsFalse(node);

   node = cJSON_GetObjectItem(root, "rectanglePositions");
   if (!cJSON_IsArray(node))
      return parseFailCleanUp(root);

   // *******************************************************
   // free the allocated memory by previous execution
   // and allocate the memory needed to store new digit positions
   free(k210config->positions);
   k210config->positions = (Position*) malloc (
      sizeof(Position) * k210config->digitCount
   );
   if (k210config->positions == NULL)
   {
      ESP_LOGE("CJSON", "unable to allocate memory!");
      return parseFailCleanUp(root);
   }

   // *******************************************************
   // loop throw each digit's position property
   for (int_t i = 0; i < k210config->digitCount; i++)
   {
      // get the (i)th digit's position property
      cJSON* child = cJSON_GetArrayItem(node, i);
      if (child == NULL)
         return parseFailCleanUp(root);

      Position *position = &(k210config->positions[i]);
      if (!extractPosition(child, position))
         return parseFailCleanUp(root);
   }

   cJSON_Delete(root);
   return TRUE;
}

/**
 * parse the [x, y, width and height] values in the child node
 * and write the result in 'position'
 */
bool_t extractPosition(cJSON *child, Position *position)
{
   cJSON* attribute = cJSON_GetObjectItem(child, "x");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->x = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "y");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->y = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "width");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->width = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "height");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->height = cJSON_GetNumberValue(attribute);

   return TRUE;
}

/**
 * helper function for parseK210Configs error
 * makes sure to free the memory used by the CJSON parser
 */
bool_t parseFailCleanUp(cJSON* root)
{
   cJSON_Delete(root);
   return FALSE;
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

   return buffer[4 + myEnv.k210config.digitCount] != ';' ||
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
// login handler

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

error_t loginHandler(HttpConnection *connection)
{
   char_t* data = (char_t*) malloc(100);
   if (!data) {
      ESP_LOGE("API", "login handler couldn't allocate memory!");
      return httpSendManual(connection, 500, "text/plain", "something went wrong!");
   }

   size_t length = 0;
   httpReadStream(connection, data, 99, &length, 0);
   data[length] = 0;

   char_t* tmp = data;
   char_t* username = parseFormField(&tmp, "username");
   char_t* password = parseFormField(&tmp, "password");

   if (!strcmp(username, "erfan") && !strcmp(password, "1234"))
      logIn(connection);

   free(username);
   free(password);

   connection->response.location = "/index.html";
   error_t error = httpSendHeaderManual(connection, 302, NULL, 0);
   if (error) return error;
   return httpCloseStream(connection);
}

// ********************************************************************************************

/**
 * manual router function for incoming http requests.
 * (uri is a null-terminated string)
 */
error_t httpServerManualRouter(HttpConnection *connection, const char_t *uri)
{
   // serve the login page
   if (!strcmp(uri, "/login.html"))
      return httpSendResponse(connection, uri);

   if (!strcmp(uri, "/login"))
      return loginHandler(connection);

   // serve the global styles
   if (!strcmp(uri, "/styles.css"))
      return httpSendResponse(connection, uri);

   // block request if not logged in
   if(!hasLoggedIn(connection)) {
      connection->response.location = "/login.html";
      error_t error = httpSendHeaderManual(connection, 302, NULL, 0);
      if (error) return error;
      return httpCloseStream(connection);
   }

   // use handler functions (API)
   if (!strcmp(uri, "/config"))
      return configHandler(connection);

   if (!strcmp(uri, "/camera"))
      return cameraImgHandler(connection);

   if (!strcmp(uri, "/ai"))
      return getAIHandler(connection);

   return ERROR_NOT_FOUND;
}
