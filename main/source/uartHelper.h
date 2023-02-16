#ifndef __uartHelper_H__
#define __uartHelper_H__

// size of the buffer in bytes to store recieved data
#define BUFFER_SIZE 39000

void serialInit();

bool_t uartAcquire(uint_t waitTimeMS)
void uartRelease();

void uartClearBuffer();
uint8_t* uartReadBytesSync(size_t chunkSize, uint_t waitTimeMS);

void uartSendBytes(const void* data, size_t size);
void uartSendString(const char_t* str);

uint8_t* uartGetBuffer();
size_t uartGetBufLength();
bool_t waitForBuffer(size_t chunkSize, uint_t waitCount);

#endif