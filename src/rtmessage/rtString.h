#ifndef RT_STRING_H
#define RT_STRING_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Copies up to (size - 1) characters and null-terminates
uint32_t rtString_Copy(char* dstBuffer,  const char* srcBuffer, uint32_t copySize);

#ifdef __cplusplus
}
#endif
#endif // RT_STRING_H
