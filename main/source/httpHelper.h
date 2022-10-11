#ifndef __HTTP_HELPER_H__
#define __HTTP_HELPER_H__

// HTTP helper functions
error_t httpSendHeaderManual(HttpConnection* connection,
   uint_t statusCode, char_t* contentType, size_t length);

error_t httpSendManual(HttpConnection* connection,
   int32_t statusCode, char_t* contentType, char_t* message);

error_t httpSendAndFreeManual(HttpConnection* connection,
   int32_t statusCode, char_t* contentType, char_t* message);

error_t httpSendJsonManual(HttpConnection* connection,
   int32_t statusCode, char_t* message);

error_t httpSendJsonAndFreeManual(HttpConnection* connection,
   int32_t statusCode, char_t* message);

error_t apiSendRejectionManual(HttpConnection* connection);

error_t apiSendSuccessManual(HttpConnection* connection,
   char_t* message);

#endif
