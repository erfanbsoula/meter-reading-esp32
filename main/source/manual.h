#ifndef __MANUAL_H__
#define __MANUAL_H__

void serialInit();
void serial_event_task(void *pvParameters);

void initManual();

error_t httpServerManualRouter(
    HttpConnection* connection, const char_t *uri);

#endif