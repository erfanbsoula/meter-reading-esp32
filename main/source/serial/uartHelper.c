#include "esp_system.h"
#include "esp_log.h"
#include "uartHelper.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// this size will be actually allocated and used as the serial buffer:
// **! make sure you have enough free memory. if not, change the BUFFER_SIZE !**
#define ACTUAL_BUF_SIZE (BUFFER_SIZE + 1024)

#define LOG_TAG "UART"

// ********************************************************************************************
// UART configuration

#define MY_UART  (UART_NUM_2)
#define SER_TXD  (GPIO_NUM_33)
#define SER_RXD  (GPIO_NUM_32)
// #define PATTERN_CHR_NUM (3)

#define UART_BUF_SIZE (1024)
#define RD_BUF_SIZE (UART_BUF_SIZE)
static QueueHandle_t myQueue;

#define WAIT_FOR_EVENT_MS 10000
#define WAIT_FOR_DATA_MS 500

#define WAIT_FOR_EVENT_TICKS (WAIT_FOR_EVENT_MS / portTICK_PERIOD_MS)
#define WAIT_FOR_DATA_TICKS (WAIT_FOR_DATA_MS / portTICK_PERIOD_MS)

// ********************************************************************************************
// Global Variables

// this will point to the beginning of the allocated buffer:
uint8_t* buffer;

// this will show the number of bytes recieved by the UART event handler:
size_t in_buf_len = 0;

/**
 * every function that wants to use the UART serial communication,
 * must acquire this lock and release it after the process is done.
 * 
 * remember to release the lock in case of process termination due to errors!
 */
SemaphoreHandle_t mutexLock;

// ********************************************************************************************
// forward declaration of functions

void serialInit();
void serialEventTask(void *pvParameters);
bool_t uartAcquire(uint_t waitTimeMS);
void uartRelease();
void uartClearBuffer();
uint8_t* uartReadBytesSync(size_t chunkSize, uint_t waitTimeMS);
size_t uartGetBufLength();
uint8_t* uartGetBuffer();
void uartSendBytes(const void* data, size_t size);
void uartSendString(const char_t* str);
bool_t waitForBuffer(size_t chunkSize, uint_t waitTimeMS);

// ********************************************************************************************
// UART initialization

void serialInit()
{
	ESP_LOGI(LOG_TAG, "initializing serial communication!");

   uart_config_t uartConfig = {
		.baud_rate = 921600 ,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB,
	};

   uart_driver_install(
      MY_UART, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &myQueue, 0
   );

	uart_param_config(MY_UART, &uartConfig);
	uart_set_pin(
      MY_UART, SER_TXD, SER_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE
   );

	uart_set_mode(MY_UART, UART_MODE_UART);

   // you can also use pattern detection (we don't need it yet!)
   // uart_enable_pattern_det_baud_intr(MY_UART, '+', PATTERN_CHR_NUM, 9, 0, 0);

   uart_pattern_queue_reset(MY_UART, 20);

   // initialize Serial Event Task
   BaseType_t ret = xTaskCreatePinnedToCore(
      serialEventTask, "serialEventTask", 2048, NULL, 12, NULL, 1
   );
   if(ret != pdPASS) { // error check
      ESP_LOGE(LOG_TAG, "failed to create serial event task!");
   }

   mutexLock = xSemaphoreCreateMutex();
   xSemaphoreGive(mutexLock);
}

// ********************************************************************************************

// this task will be pinned to the core1 and will always be running
// **! you don't need to change this task in order to intract with UART !**
void serialEventTask(void *pvParameters)
{
   ESP_LOGI(LOG_TAG, "starting serial task on Core: %d", xPortGetCoreID());
   ESP_LOGI(LOG_TAG, "Free Heap Size: %dkb", xPortGetFreeHeapSize()/1024);

   // allocating the serial buffer
   buffer = (uint8_t*) malloc(ACTUAL_BUF_SIZE);
   if (buffer == NULL)
      ESP_LOGE(LOG_TAG, "couldn't allocate Buffer Memory");
   else
      ESP_LOGI(LOG_TAG, "Buffer Memory allocated successfully!");

   uart_event_t event;
   size_t buffered_size;

   while(1) {
      if(xQueueReceive(myQueue, (void*) &event, WAIT_FOR_EVENT_TICKS))
      {
         switch(event.type)
         {
            case UART_DATA:
               uart_read_bytes(
                  MY_UART, buffer+in_buf_len, event.size, WAIT_FOR_DATA_TICKS);
               in_buf_len += event.size;
               break;
            // **************************************************
            case UART_FIFO_OVF:
               ESP_LOGI(LOG_TAG, "hw fifo overflow");
               uart_flush_input(MY_UART);
               xQueueReset(myQueue);
               break;
            // **************************************************
            case UART_BUFFER_FULL:
               ESP_LOGI(LOG_TAG, "ring buffer full");
               uart_flush_input(MY_UART);
               xQueueReset(myQueue);
               break;
            // **************************************************
            case UART_BREAK:
               ESP_LOGI(LOG_TAG, "uart rx break");
               break;
            // **************************************************
            case UART_PARITY_ERR: // this feature is disabled
               ESP_LOGI(LOG_TAG, "uart parity error");
               break;
            // **************************************************
            case UART_FRAME_ERR:
               ESP_LOGI(LOG_TAG, "uart frame error");
               break;
            // **************************************************
            case UART_PATTERN_DET: // this feature is not enabled
               uart_get_buffered_data_len(MY_UART, &buffered_size);
               int pos = uart_pattern_pop_pos(MY_UART);
               ESP_LOGI(LOG_TAG,
                  "[UART PATTERN DETECTED] pos: %d, buffered size: %d",
                  pos, buffered_size);
               break;
            // **************************************************
            default:
               ESP_LOGI(LOG_TAG, "uart event type: %d", event.type);
               break;
         }
      }
   }

   free(buffer);
   buffer = NULL;
   vTaskDelete(NULL);
}

// ********************************************************************************************
// resource lock

bool_t uartAcquire(uint_t waitTimeMS)
{
   TickType_t waitTicks = waitTimeMS / portTICK_PERIOD_MS;
   return xSemaphoreTake(mutexLock, waitTicks) == pdTRUE;
}

void uartRelease() { xSemaphoreGive(mutexLock); }

// ********************************************************************************************

void uartClearBuffer() { in_buf_len = 0; }

/**
 * waits until the size of the recieved data in the buffer
 * matches the chunkSize parameter and returns a pointer
 * to the begining of the buffer.
 * 
 * if filling the buffer takes longer than waitTimeMS,
 * function will return a Null value indicating failure.
 * 
 * don't free the buffer retuned by this function!
 */
uint8_t* uartReadBytesSync(size_t chunkSize, uint_t waitTimeMS)
{
   if (waitForBuffer(chunkSize, waitTimeMS))
      return buffer;

   return NULL;
}

size_t uartGetBufLength() { return in_buf_len; }
uint8_t* uartGetBuffer() { return buffer; }

// ********************************************************************************************

// wrapper for sending bytes
void uartSendBytes(const void* data, size_t size)
{
   uart_write_bytes(MY_UART, data, size);
}

// sends a null-terminated string
void uartSendString(const char_t* str)
{
   uart_write_bytes(MY_UART, str, strlen(str));
}

// ********************************************************************************************

/**
 * waits until the size of the recieved data in the buffer
 * matches the chunkSize parameter and returns a TRUE value
 * indicating success.
 * 
 * if filling the buffer takes longer than waitTimeMS,
 * function will return a FALSE value indicating failure.
 */
bool_t waitForBuffer(size_t chunkSize, uint_t waitTimeMS)
{
   uint_t counter = 0;
   uint_t waitCount = waitTimeMS / portTICK_PERIOD_MS;

   while (in_buf_len < chunkSize)
   {
      if (waitCount < counter)
         return FALSE;

      counter += 1;
      vTaskDelay(1);
   }
   return TRUE;
}
