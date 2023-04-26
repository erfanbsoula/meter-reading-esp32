#ifndef __ENVTYPES_H__
#define __ENVTYPES_H__

#include "source/network/network.h"
#include "source/mqtt/mqttHelper.h"
#include "os_port.h"

#define MAX_DIGIT_COUNT 8

// ! supports maximum 10 users !
#define USER_COUNT 2

// sessionId length must be less than or equal to 32
#define SESSION_ID_LENGTH 16

typedef struct _ErrorLog ErrorLog;
typedef struct _Position Position;
typedef struct _ImgConfig ImgConfig;
typedef struct _User User;
typedef struct _Environment Environment;

struct _ErrorLog
{
   bool_t k210_not_responding;
};

struct _Position
{
   uint16_t x;
   uint16_t y;
   uint16_t width;
   uint16_t height;
};

struct _ImgConfig
{
   uint8_t isConfigured;
   uint8_t digitCount;
   uint8_t invert;
   Position positions[MAX_DIGIT_COUNT];
};

struct _User
{
   char_t username[16];
   char_t password[20];
   char_t sessionId[SESSION_ID_LENGTH+1];
};

struct _Environment
{
   LanConfig lanConfig;
   StaWifiConfig staWifiConfig;
   ApWifiConfig apWifiConfig;
   User users[USER_COUNT];
   ImgConfig imgConfig;
   char_t meterCounter[MAX_DIGIT_COUNT+1];
   MqttConfig mqttConfig;
   ErrorLog errorLog;
};

#endif