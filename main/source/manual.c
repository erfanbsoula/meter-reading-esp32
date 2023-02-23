#include "includes.h"
#include "environment.h"
#include "source.h"

#include "serial/uartHelper.h"
#include "storage/storage.h"
#include "server/session.h"

// automatic reading
#define AUTOMATED_READING DISABLED

// ********************************************************************************************
// Global Variables

Environment appEnv;

// ********************************************************************************************
// forward declaration of functions

void initManual();

// ********************************************************************************************

void initManual()
{
   // initialize serial communication and serial task
   serialInit();

   // initialize and retrieve appEnv from nvs
   retrieveEnvironment(&appEnv);

   // initialize session handler
   initSessionHandler();


   esp_timer_get_time();

   // initialize AI Event Task
   // BaseType_t ret = xTaskCreatePinnedToCore(
   //    aiTask, "aiTask", 2048, NULL, 12, NULL, 1
   // );
   // if(ret != pdPASS) { // error check
   //    ESP_LOGE("aiTask","failed to create task!");
   // }

   // initialize mqtt task
   // ret = xTaskCreatePinnedToCore(
   //    mqttTask, "mqttTask", 2048, NULL, 12, NULL, 1
   // );
   // if(ret != pdPASS) { // error check
   //    ESP_LOGE("mqttTask","failed to create task!");
   // }
}

// ********************************************************************************************
// void k210StartupConfig();

// // handle this function later
// void k210StartupConfig()
// {
//    while (uartBusy) {
//       ESP_LOGE("StartUp", "unexpected usage on uart!");
//       vTaskDelay(2000 / portTICK_PERIOD_MS);
//    }
//    uartBusy = TRUE;
//    sendConfigToK210();
//    // myEnv.k210config.isConfigured = sendConfigToK210();
//    uartBusy = FALSE;
// }

// ********************************************************************************************

// ai task always running to keep the AI reading updated
// void aiTask(void *pvParameters)
// {
//    while(1) {
//       vTaskDelay(5000 / portTICK_PERIOD_MS);
//       if (!myEnv.k210config.isConfigured) {
//          ESP_LOGI("aiTask", "waiting for configuration");
//          continue;
//       }

//       if (!readingValid) {
//          free(aiReading);
//          aiReading = (char_t*) malloc(myEnv.k210config.digitCount);
//       }

//       readingValid = getAiHelper(aiReading);
//       if (readingValid) {
//          ESP_LOGI("aiTask", "got current AI-reading successfully!");
//          bool_t result;
//          result = nvsSaveString(NVS_aiReading_VAR, aiReading);
//          if (!result) {
//             ESP_LOGI("aiTask", "couldn't save AI-reading");
//          }
//       }
//       else ESP_LOGI("aiTask", "couldn't get AI-reading");
//    }
// }

// ********************************************************************************************
