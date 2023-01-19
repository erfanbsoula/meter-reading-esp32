#ifndef __SOURCE_H__
#define __SOURCE_H__

void initManual();

error_t httpServerManualRouter(
    HttpConnection* connection, const char_t *uri);

#endif