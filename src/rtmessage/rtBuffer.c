/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include "rtBuffer.h"
#include "rtEncoder.h"
#include "rtMemory.h"
#include <stdlib.h>
#include <string.h>

#define rtAtomic          volatile int32_t
#define rtAtomicInc(ptr)  (__sync_add_and_fetch(ptr, 1))
#define rtAtomicDec(ptr)  (__sync_sub_and_fetch(ptr, 1))


struct _rtBuffer
{
  uint8_t* data;
  uint32_t len;
  rtAtomic refcount;
};


rtError
rtBuffer_Create(rtBuffer* buff)
{
  *buff = (rtBuffer) rt_try_malloc(sizeof(struct _rtBuffer));
  if(!*buff)
    return return rtErrorFromErrno(ENOMEM);
  (*buff)->data = NULL;
  (*buff)->len = 0;
  (*buff)->refcount = 1;
  return RT_OK;
}

rtError
rtBuffer_CreateFromBytes(rtBuffer* buff, uint8_t* b, int n)
{
  rtError err = rtBuffer_Create(buff);
  if (err != RT_OK)
    return err;

  (*buff)->data = (uint8_t *) rt_try_malloc(sizeof(uint8_t) * n);
  if(!(*buff)->data)
    return rtErrorFromErrno(ENOMEM);
  memcpy((*buff)->data, b, n);
  return RT_OK;
}

rtError
rtBuffer_Destroy(rtBuffer buff)
{
  if (buff)
  {
    if (buff->data)
      free(buff->data);
    free(buff);
  }
  return RT_OK;
}

rtError
rtBuffer_Retain(rtBuffer buff)
{
  rtAtomicInc(&buff->refcount);
  return RT_OK;
}

rtError
rtBuffer_Release(rtBuffer buff)
{
  int32_t n = rtAtomicDec(&buff->refcount);
  if (n == 0)
    rtBuffer_Destroy(buff);
  return RT_OK;
}

rtError
rtBuffer_WriteInt32(rtBuffer buff, int32_t n)
{
  (void) buff;
  (void) n;
  return RT_OK;
}

rtError
rtBuffer_WriteString(rtBuffer buff, char const* s, int n)
{
  (void) buff;
  (void) s;
  (void) n;
  return RT_OK;
}

rtError
rtBuffer_ReadInt32(rtBuffer buff, int32_t* n)
{
  (void) buff;
  (void) n;
  return RT_OK;
}

rtError
rtBuffer_ReadString(rtBuffer buff, char** s, int* n)
{
  (void) buff;
  (void) s;
  (void) n;
  return RT_OK;
}
