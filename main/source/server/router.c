#include "../includes.h"
#include "router.h"
#include "httpHelper.h"
#include "session.h"
#include "../handlers/handlers.h"

// ********************************************************************************************
// forward declaration of functions

error_t httpServerManualRouter(
	HttpConnection *connection, const char_t *uri);

error_t routerHelper(HttpConnection *connection,
	const char_t *uri, User *currentUser);

// ********************************************************************************************

/**
 * manual router function for incoming http requests.
 * (uri is a null-terminated string)
 */
error_t httpServerManualRouter(HttpConnection *connection, const char_t *uri)
{
	if (!strcmp(uri, "/login.html") ||
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
		return configHandler(connection);

	if (!strcmp(uri, "/camera"))
		return cameraImgHandler(connection);

	if (!strcmp(uri, "/ai"))
		return getAIHandler(connection);

	return ERROR_NOT_FOUND;
}

// ********************************************************************************************
