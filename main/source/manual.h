#ifndef __MANUAL_H__
#define __MANUAL_H__

void initManual();

error_t httpServerManualRouter(
    HttpConnection* connection, const char_t *uri);

#endif