#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvsHelper.h"

#include "nvs_flash.h"

#define LOG_TAG "NVS"
#define NVS_PAGE_NAME "storage"

// ********************************************************************************************
// forward declaration of functions

nvs_handle_t nvsStart();
bool_t nvsSaveString(char_t *varName, char_t *varValue);
bool_t nvsReadString(char_t *varName, char_t **varValue);
char_t* allocateStringMem(nvs_handle_t nvsHandle, char_t *varName);
bool_t nvsFinish(nvs_handle_t nvsHandle);

// ********************************************************************************************

/**
 * loads the NVS page and returns it's handle.
 * returns Null in case of failure.
 */
nvs_handle_t nvsStart()
{
   nvs_handle_t nvsHandle;

   // open the NVS page
   esp_err_t err = nvs_open(NVS_PAGE_NAME, NVS_READWRITE, &nvsHandle);
   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Error (%s) opening NVS handle!\n",
         esp_err_to_name(err));
      
      return 0;
   }

   return nvsHandle;
}

// ********************************************************************************************

/**
 * saves the string as a key-value pair varName:varValue in NVS.
 * varValue can be accessed using the varName key.
 */
bool_t nvsSaveString(char_t *varName, char_t *varValue)
{
   nvs_handle_t nvsHandle = nvsStart();
   if (!nvsHandle)
      return false;

   // the actual storage will not be updated until nvsFinish is called.
   esp_err_t err = nvs_set_str(nvsHandle, varName, varValue);
   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to set the string value!");
      return false;
   }

   return nvsFinish(nvsHandle);
}

// ********************************************************************************************

/**
 * reads the value of varName key in NVS.
 * 
 * string memory is allocated in the function.
 * the pointer to the string is saved at (*varValue)
 * free (*varValue) after you're done
 * 
 * string (*varValue) is null-terminated
 */
bool_t nvsReadString(char_t *varName, char_t **varValue)
{
   nvs_handle_t nvsHandle = nvsStart();
   if (!nvsHandle)
      return false;

   char_t *strMem = allocateStringMem(nvsHandle, varName);
   if (strMem == NULL)
      return false;

   size_t required_size;
   esp_err_t err = nvs_get_str(
      nvsHandle, varName, strMem, &required_size);

   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get the value of %s!", varName);
      free(strMem);
      return false;
   }
   strMem[required_size] = '\0';

   if (!nvsFinish(nvsHandle)){
      free(strMem);
      return false;
   }

   (*varValue) = strMem;
   return true;
}

// ********************************************************************************************

/**
 * allocates enough memory to store value of the varName
 * returns Null in case of failure
 */
char_t* allocateStringMem(nvs_handle_t nvsHandle, char_t *varName)
{
   // get the size of value string
   size_t required_size;
   esp_err_t err = nvs_get_str(nvsHandle, varName, NULL, &required_size);
   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to get the value of %s!", varName);
      return NULL;
   }

   // allocate needed memory to store the value string
   char_t *strMem = malloc(required_size + 1);
   if (strMem == NULL) {
      ESP_LOGE(LOG_TAG, "memory allocation failed!");
      return NULL;
   }

   return strMem;
}

// ********************************************************************************************

/**
 * writes the pending changes to non-volatile storage
 * and closes the storage handle and frees allocated resources.
 */
bool_t nvsFinish(nvs_handle_t nvsHandle)
{
   esp_err_t err = nvs_commit(nvsHandle);
   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Failed to commit!");
      return false;
   }

   nvs_close(nvsHandle);
   return true;
}

// ********************************************************************************************
