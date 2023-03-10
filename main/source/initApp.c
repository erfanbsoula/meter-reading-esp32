#include "esp_system.h"
#include "esp_log.h"

#include "source.h"
#include "envTypes.h"

#include "serial/uartHelper.h"
#include "storage/storage.h"
#include "server/session.h"
#include "mqtt/mqttHelper.h"

// ********************************************************************************************
// Global Variables

#include "appEnv.h"
Environment appEnv;

// ********************************************************************************************

void initApp()
{
   // initialize serial communication and serial task
   serialInit();

   // initialize and retrieve appEnv from nvs
   retrieveEnvironment(&appEnv);

   // initialize session handler
   initSessionHandler();

   // initialize mqtt client task
   mqttInitialize();
}
