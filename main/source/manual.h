#ifndef __MANUAL_H__
#define __MANUAL_H__

// ! supports maximum 10 users !
#define USER_COUNT 2

typedef struct _Position Position;
typedef struct _K210config K210config;
typedef struct _User User;
typedef struct _Environment Environment;

struct _Position
{
   uint_t x;
   uint_t y;
   uint_t width;
   uint_t height;
};

struct _K210config
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
};

struct _Environment
{
   User users[USER_COUNT];
   K210config k210config;
   char_t *aiReading;
};

#endif