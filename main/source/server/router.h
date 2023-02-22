#ifndef __ROUTER_H__
#define __ROUTER_H__

error_t httpServerManualRouter(
    HttpConnection *connection, const char_t *uri);

#endif