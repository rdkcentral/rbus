#ifndef RT_STRING_H
#define RT_STRING_H

#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Safe string copy: copies up to (size-1) characters and null-terminates
uint32_t rtString_Copy(char* dstBuffer, const char* srcBuffer, uint32_t dstBufferSize);

#ifdef __cplusplus
}
#endif
#endif // RT_STRING_H
