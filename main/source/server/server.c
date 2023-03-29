#include <stdlib.h>
#include <string.h>
#include "core/net.h"
#include "source/network/network.h"
#include "http/http_server.h"
#include "server.h"
#include "httpHelper.h"
#include "handlers/session.h"
#include "handlers/handlers.h"
#include "esp_log.h"
#include "debug.h"

// application configuration
#define APP_HTTP_MAX_CONNECTIONS 4

// global variables
HttpServerSettings httpServerSettings;
HttpServerContext httpServerContext;
HttpConnection httpConnections[APP_HTTP_MAX_CONNECTIONS];

// ********************************************************************************************
// forward declaration of functions

error_t httpServerRouter(
	HttpConnection *connection, const char_t *uri);

error_t routerHelper(HttpConnection *connection,
	const char_t *uri, User *currentUser);

error_t httpServerUriNotFoundCallback(
   HttpConnection *connection, const char_t *uri);

// ********************************************************************************************

void initializeHttpServer()
{
   error_t error;
   initSessionHandler();

   httpServerGetDefaultSettings(&httpServerSettings);
   // bind HTTP server to a desired interface
   httpServerSettings.interface = NULL;

   // listen on port 80
   httpServerSettings.port = HTTP_PORT;
   httpServerSettings.maxConnections = APP_HTTP_MAX_CONNECTIONS;
   httpServerSettings.connections = httpConnections;
   strcpy(httpServerSettings.rootDirectory, "/");
   strcpy(httpServerSettings.defaultDocument, "index.html");

   httpServerSettings.requestCallback = httpServerRouter;
   httpServerSettings.uriNotFoundCallback = httpServerUriNotFoundCallback;

   error = httpServerInit(&httpServerContext, &httpServerSettings);
   if (error) {
      TRACE_ERROR("Failed to initialize HTTP server!\r\n");
   }

   error = httpServerStart(&httpServerContext);
   if(error) {
      TRACE_ERROR("Failed to start HTTP server!\r\n");
   }
}

// ********************************************************************************************

/**
 * manual router function for incoming http requests.
 * (uri is a null-terminated string)
 */
error_t httpServerRouter(HttpConnection *connection, const char_t *uri)
{
   if(!strcmp(uri, "/login.html") ||
      !strcmp(uri, "/styles.css"))
   {
      // serve public content without authentication
      return httpSendResponse(connection, uri);
   }

   if (!strcmp(uri, "/login"))
      return loginHandler(connection);

   User *currentUser = findLoggedInUser(connection);

   // block request if not logged in
   if (!currentUser)
   {
      connection->response.location = "/login.html";
      error_t error = httpSendHeaderManual(connection, 302, NULL, 0);
      if (error) return error;
      return httpCloseStream(connection);
   }

   return routerHelper(connection, uri, currentUser);
}

// ********************************************************************************************

error_t routerHelper(HttpConnection *connection,
	const char_t *uri, User *currentUser)
{
   // use handler functions (API)
   if (!strcmp(uri, "/config"))
      return imgConfigHandler(connection);

   if (!strcmp(uri, "/camera"))
      return cameraImgHandler(connection);

   if (!strcmp(uri, "/ai"))
      return getAIHandler(connection);
   
   if (!strcmp(uri, "/stawifi"))
      return netConfigHandler(connection, STA_WIFI_INTERFACE);

   if (!strcmp(uri, "/apwifi"))
      return netConfigHandler(connection, AP_WIFI_INTERFACE);

   return ERROR_NOT_FOUND;
}

// ********************************************************************************************

error_t httpServerUriNotFoundCallback(
   HttpConnection *connection, const char_t *uri)
{
   // not implemented
   return ERROR_NOT_FOUND;
}

// ********************************************************************************************
