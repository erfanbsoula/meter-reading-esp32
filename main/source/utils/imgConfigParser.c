#include "../includes.h"
#include "imgConfigParser.h"
#include "cJSON.h"

// ********************************************************************************************
// forward declaration of functions

bool_t parseImgConfig(ImgConfig *imgConfig, char_t *data);
bool_t parseHelper(ImgConfig *imgConfig, cJSON *root);
bool_t fillAttributes(ImgConfig *imgConfig, cJSON *root);
bool_t fillPositionArray(ImgConfig *imgConfig, cJSON *arrNode);
bool_t extractPosition(Position *position, cJSON *child);

// ********************************************************************************************

/**
 * parse the configuration data stored as json string
 * and fill 'imgConfig' with the result
 */
bool_t parseImgConfig(ImgConfig *imgConfig, char_t *data)
{
   cJSON* root = cJSON_Parse(data);
   if (root == NULL)
   {
      const char_t *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
         ESP_LOGE("CJSON", "error before: %s", error_ptr);

      cJSON_Delete(root);
      return FALSE;
   }

   bool_t res = parseHelper(imgConfig, root);   

   cJSON_Delete(root);
   return res;
}

// ********************************************************************************************

bool_t parseHelper(ImgConfig *imgConfig, cJSON *root)
{
   if (!fillAttributes(imgConfig, root))
      return FALSE;

   cJSON* arrNode  = cJSON_GetObjectItem(root, "rectanglePositions");
   if (!cJSON_IsArray(arrNode)) return FALSE;

   return fillPositionArray(imgConfig, arrNode);
}

// ********************************************************************************************

bool_t fillAttributes(ImgConfig *imgConfig, cJSON *root)
{
   cJSON* node = cJSON_GetObjectItem(root, "digitCount");
   if (!cJSON_IsNumber(node)) return FALSE;
   imgConfig->digitCount = cJSON_GetNumberValue(node);

   node = cJSON_GetObjectItem(root, "invert");
   if (!cJSON_IsBool(node)) return FALSE;
   imgConfig->invert = !cJSON_IsFalse(node);

   return TRUE;
}

// ********************************************************************************************

bool_t fillPositionArray(ImgConfig *imgConfig, cJSON *arrNode)
{
   // free the allocated memory by previous execution
   free(imgConfig->positions);

   // allocate memory to store new digit-positions
   size_t size = sizeof(Position) * imgConfig->digitCount;
   imgConfig->positions = (Position*) malloc(size);

   if (imgConfig->positions == NULL)
   {
      ESP_LOGE("CJSON", "unable to allocate memory!");
      return FALSE;
   }

   // loop through each digit's position property
   for (int_t i = 0; i < imgConfig->digitCount; i++)
   {
      // get the (i)th digit's position property
      cJSON* child = cJSON_GetArrayItem(arrNode, i);
      if (child == NULL) return FALSE;

      Position *position = &(imgConfig->positions[i]);
      if (!extractPosition(position, child)) return FALSE;
   }

   return TRUE;
}

// ********************************************************************************************

/**
 * parse the [x, y, width and height] values in the child node
 * and write the result in 'position'
 */
bool_t extractPosition(Position *position, cJSON *child)
{
   cJSON* attribute = cJSON_GetObjectItem(child, "x");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->x = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "y");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->y = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "width");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->width = cJSON_GetNumberValue(attribute);

   attribute = cJSON_GetObjectItem(child, "height");
   if (!cJSON_IsNumber(attribute)) return FALSE;
   position->height = cJSON_GetNumberValue(attribute);

   return TRUE;
}

// ********************************************************************************************
