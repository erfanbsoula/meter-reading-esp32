#include "includes.h"
#include "manual.h"
#include "httpHelper.h"
#include "myNVS.h"

// **! only change the (manual) fields !**
// ********************************************************************************************
// UART configuration

#define UART (UART_NUM_2)
#define SER_TXD  (GPIO_NUM_33)
#define SER_RXD  (GPIO_NUM_32)
// #define PATTERN_CHR_NUM (3)

#define UART_BUF_SIZE (1024)
#define RD_BUF_SIZE (UART_BUF_SIZE)
static QueueHandle_t uart0_queue;

// ********************************************************************************************
// serial communication configs

#define TOTAL_SIZE 76800    // number of bytes to expect when requesing image
#define BUFFER_SIZE 39000    // number of bytes to transfer at each try (manual)

// this size will be actually allocated and used as the serial buffer:
// **! make sure you have enough free memory. if not, change the BUFFER_SIZE !**
#define ACTUAL_BUF_SIZE (BUFFER_SIZE + 1000)

// this will point to the beginning of the allocated buffer:
uint8_t* buffer;

// this will show the number of bytes recieved by the UART event handler:
size_t in_buf_len = 0;

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

#define NVS_AI_VARIABLE_NAME "AiRes"

// ********************************************************************************************
// UART initialization & UART task

void serialInit()
{
	ESP_LOGI("Serial","Initializing serial communication!");

   uart_config_t uart_config = {
		.baud_rate = 921600 ,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE, // disable parity check
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
	};
   uart_driver_install(
      UART, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart0_queue, 0
   );
	uart_param_config(UART, &uart_config);
	uart_set_pin(
      UART, SER_TXD, SER_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE
   );
	uart_set_mode(UART, UART_MODE_UART);
   // you can also use pattern detection (we don't need it yet!)
   // uart_enable_pattern_det_baud_intr(UART, '+', PATTERN_CHR_NUM, 9, 0, 0);
   uart_pattern_queue_reset(UART, 20);
}

// this task is pinned to the core1 and will always be running
// **! you don't need to change this task in order to intract with UART !**
void serial_event_task(void *pvParameters)
{
   ESP_LOGI("Serial", "Start serial task on Core: %d", xPortGetCoreID());
   ESP_LOGI("serialTask", "Free Heap Size: %dkb", xPortGetFreeHeapSize()/1024);

   // allocating the serial buffer
   buffer = (uint8_t*) malloc(ACTUAL_BUF_SIZE);
   if (buffer == NULL)
      ESP_LOGE("serialTask", "Couldn't allocate memory");
   else
      ESP_LOGI("serialTask", "Buffer memory allocation successful!");

   uart_event_t event;
   size_t buffered_size;

   for(;;) {
      if(xQueueReceive(uart0_queue, (void *) &event, (portTickType)portMAX_DELAY))
      {
         switch(event.type)
         {
            case UART_DATA:
               uart_read_bytes(UART, buffer+in_buf_len, event.size, 20);
               in_buf_len += event.size;
               break;
            case UART_FIFO_OVF:
               ESP_LOGI("serialTask", "hw fifo overflow");
               uart_flush_input(UART);
               xQueueReset(uart0_queue);
               break;
            case UART_BUFFER_FULL:
               ESP_LOGI("serialTask", "ring buffer full");
               uart_flush_input(UART);
               xQueueReset(uart0_queue);
               break;
            case UART_BREAK:
               ESP_LOGI("serialTask", "uart rx break");
               break;
            case UART_PARITY_ERR: // this feature is disabled
               ESP_LOGI("serialTask", "uart parity error");
               break;
            case UART_FRAME_ERR:
               ESP_LOGI("serialTask", "uart frame error");
               break;
            case UART_PATTERN_DET: // this feature is not enabled
               uart_get_buffered_data_len(UART, &buffered_size);
               int pos = uart_pattern_pop_pos(UART);
               ESP_LOGI("serialTask", "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
               break;
            default:
               ESP_LOGI("serialTask", "uart event type: %d", event.type);
               break;
         }
      }
   }

   free(buffer);
   buffer = NULL;
   vTaskDelete(NULL);
}

// ********************************************************************************************

void initManual()
{
   // we have no positions saved in th startup
   // so the pointer should be null
   k210config.positions = NULL;

   // initialize serial communication and serial task
   serialInit();
   BaseType_t ret = xTaskCreatePinnedToCore(
      serial_event_task, "serial_event_task", 2048, NULL, 12, NULL, 1
   );
   if(ret != pdPASS) { // error check
      ESP_LOGE("Serial","Failed to create serial task!\r\n");
   }

   // retrieve the ai reading before shutdown
   char_t* myVar;
   bool_t res = nvsReadString(NVS_AI_VARIABLE_NAME, &myVar);
   if (res) {
      ESP_LOGI("NVS", "{%s = %s}", NVS_AI_VARIABLE_NAME, myVar);
      free(myVar);
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
         ESP_LOGE("CJSON", "Error before: %s", error_ptr);

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
      ESP_LOGE("CJSON", "Unable to allocate memory!");
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

   uart_write_bytes(UART, str, strlen(str));
   ESP_LOGI("UART", "sent: %s", str);
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
      free(data);
   }
   else ESP_LOGE("ConfigParser", "Couldn't allocate memory!");

   if (parsingResult)
   {
      sendConfigToK210();
      uartBusy = FALSE;
      return apiSendSuccessManual(connection, "Configs Recieved!");
   }

   uartBusy = FALSE;
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
      ESP_LOGE("API", "Couldn't allocate memory");
      //! handle this error

   uint32_t counter = 0;
   while(counter < in_buf_len)
   {
      uint32_t i = 0;
      for (i = 0; i < 2048 && counter+i < in_buf_len; i++)
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
 * waits until the size of the recieved data in the buffer
 * matches the chunkSize parameter.
 * 
 * waitCount is the number of 100ms periods that the function
 * will wait for the data.
 * 
 * if filling the buffer takes longer than (waitCount*100)ms,
 * function will return a false value indicating failure.
 * 
 * otherwise, function will return a true value indicating
 * that the data is ready in the buffer.
 */
bool_t waitForBuffer(size_t chunkSize, uint_t waitCount)
{
   uint_t counter = 0;
   while (in_buf_len < chunkSize) {
      if (waitCount < counter) {
         return FALSE;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
      counter += 1;
   }
   return TRUE;
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
   uart_write_bytes(UART, (const char *) "GetNew:1", 8);
   vTaskDelay(400 / portTICK_PERIOD_MS);

   while (size_count < TOTAL_SIZE)
   {
      in_buf_len = 0;
      chunk_size = findChunkSize(size_count);

      if (size_count == 0) // if first request
         sprintf(cmdStr, "Start:%d", chunk_size);
      else // if it's not the first request ...
         sprintf(cmdStr, "Next:%d", chunk_size);

      uart_write_bytes(UART, (const char *) cmdStr, strlen(cmdStr));
      if (!waitForBuffer(chunk_size, 10)) {
         ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
         return NO_ERROR;
      }
      ESP_LOGI("API", "read chunk with size %d", in_buf_len);

      error_t error = sendChunk(connection);
      if (error) return error;
      size_count += chunk_size;
   }

   in_buf_len = 0;
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
   char_t tmp[5];
   strncpy(tmp, (char_t*)buffer, 4);
   tmp[4] = '\0';

   return buffer[4 + k210config.digitCount] != ';' ||
      strcmp(tmp, "num:");
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

   uartBusy = TRUE;

   vTaskDelay(100 / portTICK_PERIOD_MS);
   uart_write_bytes(UART, (const char *) "AIread:1", 8);
   vTaskDelay(400 / portTICK_PERIOD_MS);

   in_buf_len = 0;
   uart_write_bytes(UART, (const char *) "AIsend:1", 8);
   if (!waitForBuffer(5 + k210config.digitCount, 10)) {
      ESP_LOGI("API", "K210 seems to be off! exiting the task ...");
      uartBusy = FALSE;
      return apiSendRejectionManual(connection);
   }

   buffer[5 + k210config.digitCount] = 0;
   ESP_LOGI("UART", "recieved '%s'", (char_t*)buffer);
   if (checkAiResponseHelper()) {
      ESP_LOGE("UART", "k210 sent invalid response for ai request");
      uartBusy = FALSE;
      return apiSendRejectionManual(connection);
   }

   char_t tmp[11];
   strncpy(tmp, (char_t*)(buffer+4), k210config.digitCount);
   tmp[k210config.digitCount] = '\0';

   uartBusy = FALSE;
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
