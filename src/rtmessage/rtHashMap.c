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
#include "rtHashMap.h"
#include "rtVector.h"
#include "rtLog.h"
#include "rtMemory.h"
#include <string.h>

#define RTHASHMAP_INITSIZE 16
#define RTHASHMAP_LOAD_FACTOR 0.75f

typedef struct rtHashMapNode
{
    const void* key;
    const void* value;
    rtHashMap hashmap;
} rtHashMapNode;

struct _rtHashMap 
{
    rtVector buckets;
    size_t size;
    float load_factor;
    rtHashMap_Hash_Func key_hasher; 
    rtHashMap_Compare_Func key_comparer;
    rtHashMap_Copy_Func key_copier;
    rtHashMap_Destroy_Func key_destroyer;
    rtHashMap_Copy_Func val_copier;
    rtHashMap_Destroy_Func val_destroyer;
};

static rtVector rtHashMap_GetBucket(rtHashMap hashmap, const void* key)
{
    if(!rtVector_Size(hashmap->buckets))
        return NULL;
    uint32_t hash = hashmap->key_hasher(hashmap, key);
    if(rtVector_Size(hashmap->buckets) > (size_t)hash)
        return rtVector_At(hashmap->buckets, hash);
    else
        return NULL;
}

int rtHashMap_Compare_Node(const void* left, const void* right)
{
    const rtHashMapNode* node = left;
    if(node)
    {
        if(right)
        {
            return node->hashmap->key_comparer(node->key, right);
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

static rtHashMapNode* rtHashMap_GetNode(rtHashMap hashmap, const void* key)
{
    rtVector bucket = rtHashMap_GetBucket(hashmap, key);
    if(bucket)
        return rtVector_Find(bucket, key, rtHashMap_Compare_Node);
    else
        return NULL;
}

static const void* rtHashMap_Copy_Func_None(const void * data)
{
    return data;
}

static void rtHashMap_Destroy_Func_None(void *data)
{
    (void)data;
}

static void rtHashMap_Node_Free(void* item)
{
    rtHashMapNode* node = item;
    if(node)
    {
        node->hashmap->key_destroyer((void*)node->key);
        node->hashmap->val_destroyer((void*)node->value);
        free(node);
    }
}

static void rtHashMap_Bucket_Free(void* item)
{
    if(item)
    {
        rtVector_Destroy((rtVector)item, rtHashMap_Node_Free);
    }
}

static void rtHashMap_Bucket_Free_None(void* item)
{
    if(item)
    {
        rtVector_Destroy((rtVector)item, rtHashMap_Destroy_Func_None);
    }
}

void rtHashMap_Create(rtHashMap *phashmap)
{
    /* create a dictionary where 
    the key is a string and will be copied and destroyed by the hashmap
    and the value is not copied and not destroyed by the hasmap*/
    rtHashMap_CreateEx(phashmap, 0, NULL, NULL, NULL, NULL, NULL, NULL);
}

void rtHashMap_CreateEx(
  rtHashMap* phashmap, 
  float load_factor,
  rtHashMap_Hash_Func key_hasher, 
  rtHashMap_Compare_Func key_comparer, 
  rtHashMap_Copy_Func key_copier,
  rtHashMap_Destroy_Func key_destroyer, 
  rtHashMap_Copy_Func val_copier,
  rtHashMap_Destroy_Func val_destroyer)
{
    int i;
    if(!load_factor)
        load_factor = RTHASHMAP_LOAD_FACTOR;
    if(!key_hasher)
        key_hasher = rtHashMap_Hash_Func_String;
    if(!key_comparer)
        key_comparer = rtHashMap_Compare_Func_String;
    if(!key_copier)
        key_copier = rtHashMap_Copy_Func_String;
    if(!key_destroyer)
        key_destroyer = rtHashMap_Destroy_Func_Free;
    if(!val_copier)
        val_copier = rtHashMap_Copy_Func_None;
    if(!val_destroyer)
        val_destroyer = rtHashMap_Destroy_Func_None;
    *phashmap = rt_try_malloc(sizeof(struct _rtHashMap));
    if(!*phashmap)
        return;
    (*phashmap)->load_factor = load_factor;
    (*phashmap)->key_hasher = key_hasher;
    (*phashmap)->key_comparer = key_comparer;
    (*phashmap)->key_copier = key_copier;
    (*phashmap)->key_destroyer = key_destroyer;
    (*phashmap)->val_copier = val_copier;
    (*phashmap)->val_destroyer = val_destroyer;
    (*phashmap)->size = 0;
    if(rtVector_Create(&(*phashmap)->buckets) != RT_OK)
    {
        rt_free(*phashmap);
        *phashmap = NULL;
        return;
    }
    for(i = 0; i < RTHASHMAP_INITSIZE; ++i)
    {
        rtVector nodes;
        rtVector_Create(&nodes);
        rtVector_PushBack((*phashmap)->buckets, nodes);
    }
}

void rtHashMap_Destroy(rtHashMap hashmap)
{
    rtVector_Destroy(hashmap->buckets, rtHashMap_Bucket_Free);
    free(hashmap);
}

static void rtHashMap_Resize(rtHashMap hashmap, int grow)
{
    rtVector old_buckets = hashmap->buckets;
    rtVector new_buckets;
    size_t old_size = rtVector_Size(old_buckets);
    size_t next_size; 
    size_t i;
    if(grow)
        next_size = old_size * 2;
    else
        next_size = old_size / 2;
    rtVector_Create(&new_buckets);
    for(i = 0; i < next_size; ++i)
    {
        rtVector nodes;
        rtVector_Create(&nodes);
        rtVector_PushBack(new_buckets, nodes);
    }
    hashmap->buckets = new_buckets;
    for(i = 0; i < old_size; ++i)
    {
        rtVector old_bucket = rtVector_At(old_buckets, i);
        int old_bucket_size = rtVector_Size(old_bucket);
        int j;
        for(j = 0; j < old_bucket_size; ++j)
        {
            rtHashMapNode* node = rtVector_At(old_bucket, j);
            rtVector_PushBack(rtHashMap_GetBucket(hashmap, node->key), node);
        }
    }
    rtVector_Destroy(old_buckets, rtHashMap_Bucket_Free_None);
    rtLog_Debug("Hashmap resize %zu", rtVector_Size(hashmap->buckets));
}

void rtHashMap_Set(rtHashMap hashmap, const void* key, const void* value)
{
    rtVector bucket = rtHashMap_GetBucket(hashmap, key);
    rtHashMapNode* node = rtVector_Find(bucket, key, rtHashMap_Compare_Node);
    if(node)
    {
        rtLog_Debug("Overwritting exist value\n");
        hashmap->key_destroyer((void*)node->key);
        hashmap->val_destroyer((void*)node->value);
    }
    if(!node)
    {
        if((hashmap->size+1) / (float)rtVector_Size(hashmap->buckets) > hashmap->load_factor)
        {
            rtHashMap_Resize(hashmap, 1);
            bucket = rtHashMap_GetBucket(hashmap, key);
        }
        node = rt_try_malloc(sizeof(struct rtHashMapNode));
        node->hashmap = hashmap;
        rtVector_PushBack(bucket, node);
        hashmap->size++;
    }
    node->key = hashmap->key_copier(key);
    node->value = hashmap->val_copier(value);
}

void* rtHashMap_Get(rtHashMap hashmap, const void* key)
{
    rtHashMapNode* node = rtHashMap_GetNode(hashmap, key);
    if(node)
        return (void*)node->value;
    else
        return NULL;
}

size_t rtHashMap_GetSize(rtHashMap hashmap)
{
    return hashmap->size;
}

int rtHashMap_Contains(rtHashMap hashmap, const void* key)
{
    rtHashMapNode* node = rtHashMap_GetNode(hashmap, key);
    if(node)
        return 1;
    else
        return 0;
}

int rtHashMap_Remove(rtHashMap hashmap, const void* key)
{
    if(rtVector_RemoveItemByCompare(rtHashMap_GetBucket(hashmap, key), key, rtHashMap_Compare_Node, rtHashMap_Node_Free) == RT_OK)
    {
        hashmap->size--;
        size_t nbuckets = rtVector_Size(hashmap->buckets);
        if(nbuckets > RTHASHMAP_INITSIZE)
            if(hashmap->size / nbuckets/ 2.0f < hashmap->load_factor)
                rtHashMap_Resize(hashmap, 0);
        return 1;
    }
    else
        return 0;
}

uint32_t rtHashMap_Hash_Func_String(rtHashMap hashmap, const void* key)
{
    int hash = 0;
    const char* skey = key;
    while(*skey)
    {
        hash = hash * 31 + *skey;
        skey++;
    }
    return abs(hash) % rtVector_Size(hashmap->buckets);
}

int rtHashMap_Compare_Func_String(const void* left, const void* right)
{
  if(left)
  {
    if(right)
    {
      return strcmp((const char*)left, (const char*)right);
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

const void* rtHashMap_Copy_Func_String(const void* data)
{
    return strdup(data);
}

void rtHashMap_Destroy_Func_Free(void* data)
{
    if(data)
    {
        free(data);
    }
}

void rtHashMap_LogDebugStats(rtHashMap hashmap)
{
    int size_verified = 0;
    int buckets_with_many = 0;
    int total_collissions = 0;
    int largest_bucket_count = 0;
    int i;
    int num_buckets = (int)rtVector_Size(hashmap->buckets);
    for(i = 0; i < num_buckets; ++i)
    {
        int bucket_size = rtVector_Size(rtVector_At(hashmap->buckets, i));
        size_verified += bucket_size;
        if(bucket_size > 1)
        {
            buckets_with_many++;
            total_collissions += bucket_size;
        }
        if(bucket_size > largest_bucket_count)
            largest_bucket_count = bucket_size;
    }
    rtLog_Warn("hashmap size=%zu size_verified=%d num_buckets=%d buckets_with_many=%d total_collissions=%d largest_bucket_count=%d\n", 
        hashmap->size, size_verified, num_buckets, buckets_with_many, total_collissions,largest_bucket_count);
}
