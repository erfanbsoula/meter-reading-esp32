#ifndef __myNVS_H__
#define __myNVS_H__

#include "os_port.h"

void nvsInitialize();

bool_t nvsStart();
void nvsFinish();

bool_t nvsGetBlob(char_t *key, void *blob, size_t size);
bool_t nvsSetBlob(char_t *key, void *blob, size_t size);

#endif