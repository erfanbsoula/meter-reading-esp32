#ifndef __uartHelper_H__
#define __uartHelper_H__

#include "os_port.h"

// size of the buffer in bytes to store recieved data
#define BUFFER_SIZE 39000

/**
 * configure uart and intialize event task
 * this function should be called only once at startup
 */
void serialInit();

/**
 * every function that needs to use uart must
 * accuire the recource and release it when done
 */
bool_t uartAcquire(uint_t waitTimeMS);
void uartRelease();

// clear old buffer and get ready to recieve new data
void uartClearBuffer();

// recieves (chunkSize) bytes of data
uint8_t* uartReadBytesSync(size_t chunkSize, uint_t waitTimeMS);
bool_t waitForBuffer(size_t chunkSize, uint_t waitTimeMS);

// send data
void uartSendBytes(const void* data, size_t size);
void uartSendString(const char_t* str);

// getter functions
uint8_t* uartGetBuffer();
size_t uartGetBufLength();

#endif