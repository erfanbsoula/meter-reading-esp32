#include "includes.h"
#include "source.h"
#include "manual.h"
#include "httpHelper.h"
#include "uartHelper.h"
#include "nvsHelper.h"
#include "myMqtt.h"
#include "session.h"
#include "cJSON.h"

// ********************************************************************************************

// number of bytes to expect when requesing image
#define TOTAL_SIZE 76800

// automatic reading
#define AUTOMATED_READING DISABLED

// NVS variable names
#define NVS_imgConfig_VAR "imgConfig"
#define NVS_aiReading_VAR "aiReading"
#define NVS_username_VAR "username#"
#define NVS_password_VAR "password#"

#define DEFAULT_USERNAME "admin#"
#define DEFAULT_PASSWORD "12345678"

// ********************************************************************************************
// Global Variables

Environment myEnv;

/**
 * in each callback function that wants to use the UART serial communication with k210,
 * check this flag and if set, return immediately.
 * 
 * if not set, you must set the flag at the beginning of the process
 * and unset the flag after the process is done.
 * 
 * **! remember to unset the flag in case of process termination due to errors !**
 */
bool_t uartBusy = FALSE;

// will hold the AI-reading result recieved from k210
char_t *aiReading;
bool_t readingValid = FALSE;

// ********************************************************************************************
// forward declaration of functions

void initEnvironment();
void aiTask(void *pvParameters);
bool_t parseK210Configs(K210config *k210config, char_t *data);
bool_t sendConfigToK210();
bool_t getAiHelper(char_t* res);

// ********************************************************************************************

void initManual()
{
   // initialize serial communication and serial task
   serialInit();

   // initilize myEnv object
   initEnvironment();

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
void retrieveUsers();
void k210StartupConfig();

/**
 * initialize global Environment object 'myEnv' on startup
 */
void initEnvironment()
{
   // k210 is not configured on startup
   myEnv.imgConfig.isConfigured = FALSE;
   myEnv.k210config.positions = NULL;

   
}

// handle this function later
void k210StartupConfig()
{
   while (uartBusy) {
      ESP_LOGE("StartUp", "unexpected usage on uart!");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
   }
   uartBusy = TRUE;
   sendConfigToK210();
   // myEnv.k210config.isConfigured = sendConfigToK210();
   uartBusy = FALSE;
}

// ********************************************************************************************

// ai task always running to keep the AI reading updated
void aiTask(void *pvParameters)
{
   while(1) {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      if (!myEnv.k210config.isConfigured) {
         ESP_LOGI("aiTask", "waiting for configuration");
         continue;
      }

      if (!readingValid) {
         free(aiReading);
         aiReading = (char_t*) malloc(myEnv.k210config.digitCount);
      }

      readingValid = getAiHelper(aiReading);
      if (readingValid) {
         ESP_LOGI("aiTask", "got current AI-reading successfully!");
         bool_t result;
         result = nvsSaveString(NVS_aiReading_VAR, aiReading);
         if (!result) {
            ESP_LOGI("aiTask", "couldn't save AI-reading");
         }
      }
      else ESP_LOGI("aiTask", "couldn't get AI-reading");
   }
}

// ********************************************************************************************
