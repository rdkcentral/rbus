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
#ifndef __RT_HASHMAP_H__
#define __RT_HASHMAP_H__

#include "rtError.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _rtHashMap;
typedef struct _rtHashMap* rtHashMap;

typedef uint32_t (*rtHashMap_Hash_Func)(rtHashMap, const void*);
typedef int (*rtHashMap_Compare_Func)(const void *, const void *);
typedef const void* (*rtHashMap_Copy_Func)(const void *);
typedef void (*rtHashMap_Destroy_Func)(void *);

void rtHashMap_Create(rtHashMap* phashmap);
void rtHashMap_CreateEx(
  rtHashMap* phashmap, 
  float load_factor,
  rtHashMap_Hash_Func key_hasher, 
  rtHashMap_Compare_Func key_comparer, 
  rtHashMap_Copy_Func key_copier,
  rtHashMap_Destroy_Func key_destroyer, 
  rtHashMap_Copy_Func val_copier,
  rtHashMap_Destroy_Func val_destroyer);
void rtHashMap_Destroy(rtHashMap hashmap);
void rtHashMap_Set(rtHashMap hashmap, const void* key, const void* value);
void* rtHashMap_Get(rtHashMap hashmap, const void* key);
size_t rtHashMap_GetSize(rtHashMap hashmap);
int rtHashMap_Contains(rtHashMap hashmap, const void* key);
int rtHashMap_Remove(rtHashMap hashmap, const void* key);

uint32_t rtHashMap_Hash_Func_String(rtHashMap hashmap, const void* key);
int rtHashMap_Compare_Func_String(const void* left, const void* right);
const void* rtHashMap_Copy_Func_String(const void* data);
void rtHashMap_Destroy_Func_Free(void* data);

void rtHashMap_LogDebugStats(rtHashMap hashmap);

#ifdef __cplusplus
}
#endif
#endif
