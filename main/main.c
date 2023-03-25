#include "esp_system.h"
#include "os_port.h"
#include "esp_log.h"
#include "debug.h"

#include "source/envTypes.h"
#include "source/storage/storage.h"
#include "source/serial/uartHelper.h"
#include "source/network/network.h"
#include "source/mqtt/mqttHelper.h"

// ********************************************************************************************
// Global Variables

#include "source/appEnv.h"
Environment appEnv;

// ********************************************************************************************

void app_main(void)
{
   // configure debug UART
   debugInit(115200);
   TRACE_INFO("Compiled: %s %s\r\n", __DATE__, __TIME__);

   // initialize non-volitile storage
   nvsInitialize();

   // initialize serial communication and serial task
   serialInit();

   // initialize and retrieve appEnv from nvs
   retrieveEnvironment(&appEnv);

   // initialize network interfaces and http server
   initializeNetworks();

   // initialize mqtt client task
   mqttInitialize();
}
