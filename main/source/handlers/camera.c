#include "../includes.h"
#include "../appEnv.h"
#include "handlers.h"
#include "../serial/uartHelper.h"
#include "../server/httpHelper.h"

// number of bytes to expect when requesing image
static const size_t TOTAL_SIZE = 76800;

static const char_t *LOG_TAG = "camera";

// ********************************************************************************************
// forward declaration of functions

error_t cameraImgHandler(HttpConnection *connection);
error_t getAndSendCameraImg(HttpConnection *connection);
error_t sendChunk(HttpConnection *connection);
size_t findChunkSize(size_t size_count);

// ********************************************************************************************

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
error_t cameraImgHandler(HttpConnection *connection)
{
   if (strcmp(connection->request.method, "GET"))
      return ERROR_NOT_FOUND;

   ESP_LOGI(LOG_TAG, "camera image requested!");

   if (!uartAcquire(50))
      return apiSendRejectionManual(connection);

   error_t error = httpSendHeaderManual(
      connection, 200, "text/plain", TOTAL_SIZE*2);
   if(error) {
      uartRelease();
      return error;
   }

   error = getAndSendCameraImg(connection);
   if(error) {
      uartRelease();
      return error;
   }

   uartRelease();
   ESP_LOGI(LOG_TAG, "image task done!");
   return httpCloseStream(connection);
}

// ********************************************************************************************

/**
 * requests the camera image from k210 over UART
 * in chunks equal to the buffer size and sends each chunk
 * to the client immediately using the sendChunk function.
 * 
 * closes the http connection if k210 doesn't respond
 */
error_t getAndSendCameraImg(HttpConnection *connection)
{
   size_t size_count = 0, chunk_size = 0;
   char_t cmdStr[20];

   uartSendBytes("camTakeNew", 10);
   vTaskDelay(200 / portTICK_PERIOD_MS);

   while (size_count < TOTAL_SIZE)
   {
      uartClearBuffer();
      chunk_size = findChunkSize(size_count);

      if (size_count == 0) // if first request
         sprintf(cmdStr, "camStart:%d", chunk_size);
      else // if it's not the first request ...
         sprintf(cmdStr, "camNext:%d", chunk_size);

      uartSendString(cmdStr);
      uint8_t *buffer = uartReadBytesSync(chunk_size, 1000);
      if (!buffer) {
         ESP_LOGI(LOG_TAG, "K210 seems to be off! exiting the task ...");
         appEnv.errorLog.k210_not_responding = true;
         return NO_ERROR;
      }
      ESP_LOGI(LOG_TAG, "read chunk with size %d", uartGetBufLength());

      error_t error = sendChunk(connection);
      if (error) return error;
      size_count += chunk_size;
   }

   uartClearBuffer();
   return NO_ERROR;
}

// ********************************************************************************************

/**
 * encodes the current data in the UART buffer
 * and sends it as a base16 string (manual encoding)
 */
error_t sendChunk(HttpConnection *connection)
{
   error_t error;
   uint8_t *tmp_buf = (uint8_t*) malloc(4096);
   if (tmp_buf == NULL)
      ESP_LOGE(LOG_TAG, "couldn't allocate memory");
      //! handle this error

   uint8_t *buffer = uartGetBuffer();
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

// ********************************************************************************************

/**
 * calculates the chunk size to request from k210
 * such that no overflow occurs
 */
size_t findChunkSize(size_t size_count)
{
   if (size_count + BUFFER_SIZE <= TOTAL_SIZE)
      return BUFFER_SIZE;

   else return (TOTAL_SIZE - size_count);
}

// ********************************************************************************************
