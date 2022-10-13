#ifndef __uartHelper_H__
#define __uartHelper_H__

// size of the buffer in bytes to store recieved data
#define BUFFER_SIZE 39000

void serialInit();

uint8_t* uartGetBuffer();
size_t uartGetBufLength();
void uartClearBuffer();
void uartSendBytes(const void* data, size_t size);
void uartSendString(const char_t* str);
bool_t waitForBuffer(size_t chunkSize, uint_t waitCount);

#endif