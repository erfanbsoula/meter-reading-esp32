#include "includes.h"
#include "uartHelper.h"

// ********************************************************************************************

// UART configuration
#define MY_UART  (UART_NUM_2)
#define SER_TXD  (GPIO_NUM_33)
#define SER_RXD  (GPIO_NUM_32)
// #define PATTERN_CHR_NUM (3)

#define UART_BUF_SIZE (1024)
#define RD_BUF_SIZE (UART_BUF_SIZE)
static QueueHandle_t myQueue;

// this size will be actually allocated and used as the serial buffer:
// **! make sure you have enough free memory. if not, change the BUFFER_SIZE !**
#define ACTUAL_BUF_SIZE (BUFFER_SIZE + 1024)

#define MY_UART_TAG "UART"

// this will point to the beginning of the allocated buffer:
uint8_t* buffer;

// this will show the number of bytes recieved by the UART event handler:
size_t in_buf_len = 0;

void serialEventTask(void *pvParameters);

// ********************************************************************************************
// UART initialization
void serialInit()
{
	ESP_LOGI(MY_UART_TAG, "initializing serial communication!");

   uart_config_t uartConfig = {
		.baud_rate = 921600 ,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE, // disable parity check
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
      ESP_LOGE(MY_UART_TAG,"failed to create serial event task!");
   }
}

// ********************************************************************************************
// this task will be pinned to the core1 and will always be running
// **! you don't need to change this task in order to intract with UART !**
void serialEventTask(void *pvParameters)
{
   ESP_LOGI(MY_UART_TAG, "starting serial task on Core: %d", xPortGetCoreID());
   ESP_LOGI(MY_UART_TAG, "Free Heap Size: %dkb", xPortGetFreeHeapSize()/1024);

   // allocating the serial buffer
   buffer = (uint8_t*) malloc(ACTUAL_BUF_SIZE);
   if (buffer == NULL)
      ESP_LOGE(MY_UART_TAG, "couldn't allocate Buffer Memory");
   else
      ESP_LOGI(MY_UART_TAG, "Buffer Memory allocated successfully!");

   uart_event_t event;
   size_t buffered_size;

   while(1) {
      if(xQueueReceive(myQueue, (void*) &event, (portTickType)portMAX_DELAY))
      {
         switch(event.type)
         {
            case UART_DATA:
               uart_read_bytes(MY_UART, buffer+in_buf_len, event.size, 20);
               in_buf_len += event.size;
               break;
            // **************************************************
            case UART_FIFO_OVF:
               ESP_LOGI(MY_UART_TAG, "hw fifo overflow");
               uart_flush_input(MY_UART);
               xQueueReset(myQueue);
               break;
            // **************************************************
            case UART_BUFFER_FULL:
               ESP_LOGI(MY_UART_TAG, "ring buffer full");
               uart_flush_input(MY_UART);
               xQueueReset(myQueue);
               break;
            // **************************************************
            case UART_BREAK:
               ESP_LOGI(MY_UART_TAG, "uart rx break");
               break;
            // **************************************************
            case UART_PARITY_ERR: // this feature is disabled
               ESP_LOGI(MY_UART_TAG, "uart parity error");
               break;
            // **************************************************
            case UART_FRAME_ERR:
               ESP_LOGI(MY_UART_TAG, "uart frame error");
               break;
            // **************************************************
            case UART_PATTERN_DET: // this feature is not enabled
               uart_get_buffered_data_len(MY_UART, &buffered_size);
               int pos = uart_pattern_pop_pos(MY_UART);
               ESP_LOGI(MY_UART_TAG,
                  "[UART PATTERN DETECTED] pos: %d, buffered size: %d",
                  pos, buffered_size);
               break;
            // **************************************************
            default:
               ESP_LOGI(MY_UART_TAG, "uart event type: %d", event.type);
               break;
         }
      }
   }

   free(buffer);
   buffer = NULL;
   vTaskDelete(NULL);
}

// ********************************************************************************************

uint8_t* uartGetBuffer() { return buffer; }
size_t uartGetBufLength() { return in_buf_len; }
void uartClearBuffer() { in_buf_len = 0; }

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
