#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

// ! supports maximum 10 users !
#define USER_COUNT 2

// warning!
// sessionId length must be less than or equal to 32
#define SESSION_ID_LENGTH 16

typedef struct _Position Position;
typedef struct _ImgConfig ImgConfig;
typedef struct _User User;
typedef struct _Environment Environment;

extern Environment appEnv;

struct _Position
{
   uint_t x;
   uint_t y;
   uint_t width;
   uint_t height;
};

struct _ImgConfig
{
   bool_t invert;
   uint_t digitCount;
   Position *positions;
   bool_t isConfigured;
};

struct _User
{
   char_t *username;
   char_t *password;
   char_t sessionId[SESSION_ID_LENGTH+1];
};

struct _Environment
{
   User users[USER_COUNT];
   ImgConfig imgConfig;
   char_t *meterCounter;
};

#endif