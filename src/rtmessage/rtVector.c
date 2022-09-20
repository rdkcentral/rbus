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
#include "rtVector.h"
#include "rtMemory.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define RTVECT_BLOCKSIZE 16

struct _rtVector
{
  void** data;
  size_t capacity;
  size_t count;
};

rtError
rtVector_Create(rtVector* v)
{
  (*v) = (struct _rtVector *) rt_try_malloc(sizeof(struct _rtVector));
  if (!(*v))
    return rtErrorFromErrno(ENOMEM);
  (*v)->data = NULL;
  (*v)->capacity = 0;
  (*v)->count = 0;
  return RT_OK;
}

rtError
rtVector_Destroy(rtVector v, rtVector_Cleanup destroyer)
{
  size_t i;

  if (!v)
    return RT_ERROR_INVALID_ARG;
  if (v->data)
  {
    if (destroyer)
    {
      for (i = 0; i < v->count; ++i)
        destroyer(v->data[i]);
    }
    free(v->data);
  }
  free(v);
  return RT_OK;
}

rtError
rtVector_PushBack(rtVector v, void* item)
{
  if (!v)
    return RT_ERROR_INVALID_ARG;

  if (!v->data)
  {
    v->data = rt_try_calloc(RTVECT_BLOCKSIZE, sizeof(void *));
    v->capacity = RTVECT_BLOCKSIZE;
  }
  else if (v->count + 1 >= v->capacity)
  {
    v->data = rt_try_realloc(v->data, (v->capacity + RTVECT_BLOCKSIZE) * sizeof(void *));
    v->capacity += RTVECT_BLOCKSIZE;
  }

  if (!v->data)
    return rtErrorFromErrno(ENOMEM);

  v->data[v->count++] = item;
  return RT_OK;
}

rtError
rtVector_RemoveItem(rtVector v, void* item, rtVector_Cleanup destroyer)
{
  size_t i;
  for (i = 0; i < v->count; ++i)
  {
    if (v->data[i] == item)
    {
      if (destroyer)
        destroyer(v->data[i]);
      break;
    }
  }

  if(i == v->count)
    return RT_FAIL;

  while (i < v->count)
  {
    v->data[i] = v->data[i+1];
    i++;
  }

  v->count -= 1;

  while (i < v->capacity)
  {
    v->data[i] = NULL;
    i++;
  }

  return RT_OK;
}

rtError 
rtVector_RemoveItemByCompare(rtVector v, const void* comparison, rtVector_Compare compare, rtVector_Cleanup destroyer)
{
  size_t i;
  for (i = 0; i < v->count; ++i)
  {
    if (compare(v->data[i], comparison) == 0)
    {
      if (destroyer)
        destroyer(v->data[i]);
      break;
    }
  }

  if(i == v->count)
    return RT_FAIL;
    
  while (i < v->count)
  {
    v->data[i] = v->data[i+1];
    i++;
  }

  v->count -= 1;

  while (i < v->capacity)
  {
    v->data[i] = NULL;
    i++;
  }

  return RT_OK;
}

void*
rtVector_At(rtVector v, size_t index)
{
  if (!v) return NULL;
  if (index >= v->count) return NULL;
  return v->data[index];
}

size_t
rtVector_Size(rtVector v)
{
  if (!v) return 0;
  return v->count;
}

int 
rtVector_HasItem(rtVector v, const void* comparison, rtVector_Compare compare)
{
  if(rtVector_Find(v, comparison, compare))
    return 1;
  else
    return 0;
}

void* 
rtVector_Find(rtVector v, const void* comparison, rtVector_Compare compare)
{
  size_t i;
  for (i = 0; i < v->count; ++i)
  {
    if(compare)
    {
      if(compare(v->data[i], comparison) == 0)
        return v->data[i];
    }
    else if (v->data[i] == comparison)
    {
      return v->data[i];
    }
  }
  return NULL;
}

void 
rtVector_Cleanup_Free(void* item)
{
    if(item)
    {
        free(item);
    }
}

int 
rtVector_Compare_String(const void* left, const void* right)
{
  if(left)
  {
    if(right)
    {
      return strcmp(left, right);
    }
    else
    {
      return 1;
    }
  }
  else
  {
    return -1;
  }
}
