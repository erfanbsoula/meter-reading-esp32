#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "storage.h"
#include "esp_log.h"

#define LOG_TAG "storage"

// NVS variable names
#define NVS_lanConfig_KEY "lanConfig"
#define NVS_staWifiConfig_KEY "staWifiConfig"
#define NVS_apWifiConfig_KEY "apWifiConfig"
#define NVS_imgConfig_KEY "imgConfig"
#define NVS_users_KEY "users"
#define NVS_meterCounter_KEY "meterCounter"
#define NVS_mqttConfig_VAR "mqttConfig"

#define DEFAULT_USERNAME "admin#"
#define DEFAULT_PASSWORD "test1234"

// ********************************************************************************************
// forward declaration of functions

bool_t retrieveEnvironment(Environment *appEnv);
void retrieveLanConfig(LanConfig *lanConfig);
void retrieveStaWifiConfig(StaWifiConfig *staWifiConfig);
void retrieveApWifiConfig(ApWifiConfig *apWifiConfig);
void retrieveImgConfig(ImgConfig *imgConfig);
void retrieveUsers(User *users);
void retrieveMeterCounter(char_t *meterCounter);
void retrieveMqttConfig(MqttConfig *mqttConfig);

bool_t saveLanConfig(LanConfig *lanConfig);
bool_t saveStaWifiConfig(StaWifiConfig *staWifiConfig);
bool_t saveApWifiConfig(ApWifiConfig *apWifiConfig);
bool_t saveImgConfig(ImgConfig *imgConfig);
bool_t saveUsers(User *users);
bool_t saveMeterCounter(char_t *meterCounter);
bool_t saveMqttConfig(MqttConfig *mqttConfig);

void setDefaultUsers(User *users);

// ********************************************************************************************

bool_t retrieveEnvironment(Environment *appEnv)
{
   bool_t result = nvsStart();
   if (!result) return FALSE;

   retrieveLanConfig(&appEnv->lanConfig);
   osDelayTask(50);
   retrieveStaWifiConfig(&appEnv->staWifiConfig);
   osDelayTask(50);
   retrieveApWifiConfig(&appEnv->apWifiConfig);
   osDelayTask(50);
   retrieveImgConfig(&appEnv->imgConfig);
   osDelayTask(50);
   retrieveUsers(appEnv->users);
   osDelayTask(50);
   retrieveMeterCounter(appEnv->meterCounter);
   osDelayTask(50);
   retrieveMqttConfig(&appEnv->mqttConfig);

   nvsFinish();
   return TRUE;
}

// ********************************************************************************************

void retrieveLanConfig(LanConfig *lanConfig)
{
   bool_t result = nvsGetBlob(
      NVS_lanConfig_KEY, lanConfig, sizeof(LanConfig));
   
   if (!result)
      lanSetDefaultConfig(lanConfig);
}

bool_t saveLanConfig(LanConfig *lanConfig)
{
   return nvsSetBlob(
      NVS_lanConfig_KEY, lanConfig, sizeof(LanConfig));
}

// ********************************************************************************************

void retrieveStaWifiConfig(StaWifiConfig *staWifiConfig)
{
   bool_t result = nvsGetBlob(
      NVS_staWifiConfig_KEY, staWifiConfig, sizeof(StaWifiConfig));
   
   if (!result)
      staWifiSetDefaultConfig(staWifiConfig);
}

bool_t saveStaWifiConfig(StaWifiConfig *staWifiConfig)
{
   return nvsSetBlob(
      NVS_staWifiConfig_KEY, staWifiConfig, sizeof(StaWifiConfig));
}

// ********************************************************************************************

void retrieveApWifiConfig(ApWifiConfig *apWifiConfig)
{
   bool_t result = nvsGetBlob(
      NVS_apWifiConfig_KEY, apWifiConfig, sizeof(ApWifiConfig));
   
   if (!result)
      apWifiSetDefaultConfig(apWifiConfig);
}

bool_t saveApWifiConfig(ApWifiConfig *apWifiConfig)
{
   return nvsSetBlob(
      NVS_apWifiConfig_KEY, apWifiConfig, sizeof(ApWifiConfig));
}

// ********************************************************************************************

void retrieveImgConfig(ImgConfig *imgConfig)
{
   bool_t result = nvsGetBlob(
      NVS_imgConfig_KEY, imgConfig, sizeof(ImgConfig));
   
   if (!result)
      imgConfig->isConfigured = FALSE;
}

bool_t saveImgConfig(ImgConfig *imgConfig)
{
   return nvsSetBlob(
      NVS_imgConfig_KEY, imgConfig, sizeof(ImgConfig));
}

// ********************************************************************************************

void retrieveUsers(User *users)
{
   bool_t result = nvsGetBlob(
      NVS_users_KEY, users, USER_COUNT * sizeof(User));
   
   if (!result)
      setDefaultUsers(users);
}

void setDefaultUsers(User *users)
{
   for (uint_t i = 0; i < USER_COUNT; i++)
   {
      User *user = &(users[i]);
      strcpy(user->username, DEFAULT_USERNAME);
      strcpy(user->password, DEFAULT_PASSWORD);

      user->username[sizeof(DEFAULT_USERNAME)-2] = '0' + i;
   }
}

bool_t saveUsers(User *users)
{
   return nvsSetBlob(
      NVS_users_KEY, users, USER_COUNT * sizeof(User));
}

// ********************************************************************************************

void retrieveMeterCounter(char_t *meterCounter)
{
   bool_t result = nvsGetBlob(
      NVS_meterCounter_KEY, meterCounter, MAX_DIGIT_COUNT+1);
   
   if (!result)
      meterCounter[0] = '\0';
}

bool_t saveMeterCounter(char_t *meterCounter)
{
   return nvsSetBlob(
      NVS_meterCounter_KEY, meterCounter, MAX_DIGIT_COUNT+1);
}

// ********************************************************************************************

void retrieveMqttConfig(MqttConfig *mqttConfig)
{
   bool_t result = nvsGetBlob(
      NVS_mqttConfig_VAR, mqttConfig, sizeof(MqttConfig));
   
   if (!result)
      mqttConfig->isConfigured = FALSE;
}

bool_t saveMqttConfig(MqttConfig *mqttConfig)
{
   return nvsSetBlob(
      NVS_mqttConfig_VAR, mqttConfig, sizeof(MqttConfig));
}

// ********************************************************************************************
