#ifndef __myNVS_H__
#define __myNVS_H__

bool_t nvsSaveString(char_t* varName, char_t* varValue);

bool_t nvsReadString(char_t* varName, char_t** varValue);

#endif