#include "rtString.h"
#include "rtLog.h"

uint32_t rtString_Copy(char* dstBuffer, const char* srcBuffer, uint32_t dstBufferSize)
{
  uint32_t srcBufferSize = 0;
  if ((0 == dstBufferSize) || (!srcBuffer) || (!dstBuffer))
    return 0;

  srcBufferSize = snprintf(dstBuffer, dstBufferSize, "%s", srcBuffer);
  if (srcBufferSize > dstBufferSize)
  {
     rtLog_Info("Truncated the given string (%s) as (%s) with size %d", srcBuffer, dstBuffer, dstBufferSize);
     return dstBufferSize;
  }
  else
    return srcBufferSize;
}
