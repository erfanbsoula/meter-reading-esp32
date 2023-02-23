#include "esp_log.h"
#include "esp_timer.h"

#include "source.h"
#include "envTypes.h"

#include "serial/uartHelper.h"
#include "storage/storage.h"
#include "server/session.h"

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

   // testing time
   ESP_LOGI("init", "time = %d", (int32_t)esp_timer_get_time());
}
