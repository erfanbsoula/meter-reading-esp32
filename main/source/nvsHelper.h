#ifndef __myNVS_H__
#define __myNVS_H__

bool_t nvsSaveString(char_t* varName, char_t* varValue);

/**
 * string memory will be allocated in the function.
 * the pointer to the string will be saved at (*varValue)
 * free (*varValue) after you're done
 * 
 * string (*varValue) is null-terminated
 */
bool_t nvsReadString(char_t* varName, char_t** varValue);

#endif