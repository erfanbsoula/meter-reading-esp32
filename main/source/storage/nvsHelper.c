#include <stdlib.h>
#include <stdbool.h>
#include "nvsHelper.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define LOG_TAG "NVS"
#define NVS_PAGE_NAME "storage"

static nvs_handle_t nvsHandle;

// ********************************************************************************************
// forward declaration of functions

void nvsInitialize();
bool_t nvsStart();
void nvsFinish();

bool_t nvsGetBlob(char_t *key, void *blob, size_t size);
bool_t nvsSetBlob(char_t *key, void *blob, size_t size);

// ********************************************************************************************

void nvsInitialize()
{
   // initialize NVS memory
   esp_err_t err = nvs_flash_init();
   if(err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND)
   {
      // NVS partition was truncated and needs to be erased
      ESP_ERROR_CHECK(nvs_flash_erase());
      // retry initializing NVS
      err = nvs_flash_init();
   }
   ESP_ERROR_CHECK(err);
   nvsHandle = NULL;
}

// ********************************************************************************************

/**
 * opens the NVS page and saves it's handle.
 * returns FALSE in case of failure.
 */
bool_t nvsStart()
{
   // open the NVS page
   esp_err_t err = nvs_open(NVS_PAGE_NAME, NVS_READWRITE, &nvsHandle);
   if (err != ESP_OK) {
      ESP_LOGE(LOG_TAG, "Error (%s) opening NVS handle!\n",
         esp_err_to_name(err));

      nvsHandle = NULL;
      return FALSE;
   }

   return TRUE;
}

// ********************************************************************************************

/**
 * reads the content of the key in NVS
 * and fills the blob.
 * 
 * needs the nvs already started and
 * doesn't close the nvs when done!
 */
bool_t nvsGetBlob(char_t *key, void *blob, size_t size)
{
   if (!nvsHandle) return FALSE;

   esp_err_t err = nvs_get_str(nvsHandle, key, blob, &size);
   if (err != ESP_OK)
   {
      ESP_LOGE(LOG_TAG, "failed to get %s!", key);
      return FALSE;
   }

   return TRUE;
}

// ********************************************************************************************

/**
 * closes the nvs handle and frees allocated resources.
 */
void nvsFinish()
{
   nvs_close(nvsHandle);
   nvsHandle = NULL;
}

// ********************************************************************************************

/**
 * saves the blob as a key-value pair in NVS.
 * blob can be accessed later using the key.
 * 
 * starts the nvs by itself and
 * closes the nvs handle when done!
 */
bool_t nvsSetBlob(char_t *key, void *blob, size_t size)
{
   if(nvsHandle || !nvsStart()) return FALSE;

   esp_err_t err = nvs_set_blob(nvsHandle, key, blob, size);
   if (err != ESP_OK)
   {
      ESP_LOGE(LOG_TAG, "failed to set %s!", key);
      return FALSE;
   }

   // write the pending changes to non-volatile storage
   err = nvs_commit(nvsHandle);
   if (err != ESP_OK)
   {
      ESP_LOGE(LOG_TAG, "failed to commit changes!");
      return FALSE;
   }

   nvsFinish(nvsHandle);
   return TRUE;
}

// ********************************************************************************************
