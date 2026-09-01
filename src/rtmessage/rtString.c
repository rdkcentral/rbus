#include "rtString.h"
#include "rtLog.h"

uint32_t rtString_Copy(char* dstBuffer, const char* srcBuffer, uint32_t copySize)
{
  uint32_t srcBufferSize = 0;
  if ((0 == copySize) || (!srcBuffer) || (!dstBuffer))
    return 0;

  srcBufferSize = snprintf(dstBuffer, copySize, "%s", srcBuffer);
  if (srcBufferSize > copySize)
  {
     rtLog_Debug("Truncated the given string (%s) as (%s) with size %d", srcBuffer, dstBuffer, copySize);
     return copySize;
  }
  else
    return srcBufferSize;
}
