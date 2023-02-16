#include "includes.h"
#include "myNVS.h"

#define NVS_PAGE_NAME "storage"

nvs_handle_t nvsStart()
{
   nvs_handle_t myNVS;

   // open the NVS page
   esp_err_t err = nvs_open(NVS_PAGE_NAME, NVS_READWRITE, &myNVS);
   if (err != ESP_OK) {
      ESP_LOGE("NVS", "Error (%s) opening NVS handle!\n",
         esp_err_to_name(err));
      
      return 0;
   }

   return myNVS;
}

bool_t nvsFinish(nvs_handle_t myNVS)
{
   esp_err_t err = nvs_commit(myNVS);
   if (err != ESP_OK) {
      ESP_LOGE("NVS", "Failed to commit!");
      return false;
   }

   // Close
   nvs_close(myNVS);
   return true;
}

bool_t nvsSaveString(char_t* varName, char_t* varValue)
{
   nvs_handle_t myNVS = nvsStart();
   if ( !myNVS ) {
      return false;
   }

   esp_err_t err = nvs_set_str(myNVS, varName, varValue);
   if (err != ESP_OK) {
      ESP_LOGE("NVS", "Failed to set the string value!");
      return false;
   }

   return nvsFinish(myNVS);
}

/**
 * string memory is allocated in the function.
 * the pointer to the string is saved at (*varValue)
 * free (*varValue) after you're done
 * 
 * string (*varValue) is null-terminated
 */
bool_t nvsReadString(char_t* varName, char_t** varValue)
{
   nvs_handle_t myNVS = nvsStart();
   if ( !myNVS ) {
      return false;
   }

   size_t required_size;
   esp_err_t err = nvs_get_str(myNVS, varName, NULL, &required_size);
   if (err != ESP_OK) {
      ESP_LOGE("NVS", "Failed to get the value of %s!", varName);
      return false;
   }

   (*varValue) = malloc(required_size + 1);
   if (*varValue == NULL) {
      ESP_LOGE("NVS", "memory allocation failed!");
      return false;
   }

   err = nvs_get_str(myNVS, varName, (*varValue), &required_size);
   if (err != ESP_OK) {
      ESP_LOGE("NVS", "Failed to get the value of %s!", varName);
      free((*varValue));
      return false;
   }
   (*varValue)[required_size] = '\0';

   if (!nvsFinish(myNVS)){
      free((*varValue));
      return false;
   }

   return true;
}
