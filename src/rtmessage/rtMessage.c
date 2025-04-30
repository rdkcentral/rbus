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
#define _GNU_SOURCE 1
#include "rtError.h"
#include "rtMessage.h"
#include "rtBase64.h"
#include "rtAtomic.h"
#include "rtMemory.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

struct _rtMessage
{
  cJSON* json;
  atomic_int count;
};

/**
 * Allocate storage and initializes it as new message
 * @param pointer to the new message
 * @return rtError
 **/
rtError
rtMessage_Create(rtMessage* message)
{
  *message = (rtMessage) rt_try_malloc(sizeof(struct _rtMessage));
  if(!*message)
    return rtErrorFromErrno(ENOMEM);
  if (message)
  {
    (*message)->count = 0;
    (*message)->json = cJSON_CreateObject();
    rt_atomic_fetch_add(&(*message)->count, 1);
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Copies only the data within the fields of the message
 * @param message to be copied
 * @param pointer to new copy of the message
 * @return rtError
 */
rtError
rtMessage_Clone(rtMessage const message, rtMessage* copy)
{
  *copy = (rtMessage) rt_try_malloc(sizeof(struct _rtMessage));
  if (!*copy)
    return rtErrorFromErrno(ENOMEM);
  (*copy)->count = 0;
  (*copy)->json = cJSON_Duplicate(message->json, 1);
  rt_atomic_fetch_add(&(*copy)->count, 1);
  return RT_OK;
}

/* Allocates storage and initializes it as new message
 * @param pointer to the new message
 * @param fill the new message with this data
 * @return rtError
 **/
rtError
rtMessage_FromBytes(rtMessage* message, uint8_t const* bytes, int n)
{
  (void) n;

  #if 0
  printf("------------------------------------------\n")
  for (i = 0; i < 256; ++i)
  {
    if (i > 0)
      printf(" ");
    if (i % 16 == 0)
      printf("\n");
    printf("0x%02x", bytes[i]);
  }
  printf("\n\n");
  #endif

  *message = (rtMessage) rt_try_malloc(sizeof(struct _rtMessage));
  if(!*message)
    return rtErrorFromErrno(ENOMEM);
  (*message)->json = cJSON_Parse((char *) bytes);
  if (!(*message)->json)
  {
    free(*message);
    *message = NULL;
    return RT_FAIL;
  }

  (*message)->count = 0;
  rt_atomic_fetch_add(&(*message)->count, 1);
  return RT_OK;
}

/**
 * Destroy a message; free the storage that it occupies.
 * @param pointer to message to be destroyed
 * @return rtError
 **/
rtError
rtMessage_Destroy(rtMessage message)
{
  if ((message) && ((message)->count == 0))
  {
    if (message->json)
      cJSON_Delete(message->json);
    free(message);
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Extract the data from a message as a byte sequence.
 * @param extract the data bytes from this message.
 * @param pointer to the byte sequence location 
 * @param pointer to number of bytes in the message
 * @return rtErro
 **/
rtError
rtMessage_ToByteArray(rtMessage message, uint8_t** buff, uint32_t *n)
{
  return rtMessage_ToString(message, (char **) buff, n);
}

rtError
rtMessage_ToByteArrayWithSize(rtMessage message, uint8_t** buff, uint32_t suggested_size, uint32_t* n)
{
#ifdef RDKC_BUILD
  (void)suggested_size;
  /*cJSON_PrintBuffered is returning NULL on rdkc so we have to use the less efficient method*/
  return rtMessage_ToString(message, (char **) buff, n);
#else
  *buff = (uint8_t *)cJSON_PrintBuffered(message->json, suggested_size, 0);
  *n = strlen((char *)*buff);
  return RT_OK;
#endif
}

rtError
rtMessage_FreeByteArray(uint8_t* buff)
{
  free(buff);
  return RT_OK;
}

/**
 * Format message as string
 * @param message to be converted to string
 * @param pointer to a string where message is to be stored
 * @param  pointer to number of bytes in the message
 * @return rtStatus
 **/
rtError
rtMessage_ToString(rtMessage const m, char** s, uint32_t* n)
{
  if (!m || !m->json)
  {
    if (*s)
      *s = NULL;
    if (n)
      *n = 0;
    return RT_FAIL;
  }

  *s = cJSON_PrintUnformatted(m->json);
  *n = strlen(*s);
  return RT_OK;
}

/**
 * Add string field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return void
 **/
void
rtMessage_SetString(rtMessage message, char const* name, char const* value)
{
   cJSON_AddItemToObject(message->json, name, cJSON_CreateString(value));
}

/**
 * Add integer field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param integer value of the field to be added
 * @return void
 **/
rtError
rtMessage_SetInt32(rtMessage message, char const* name, int32_t value)
{
  cJSON_AddNumberToObject(message->json, name, value);
  return RT_OK;
}

rtError
rtMessage_SetUInt32(rtMessage message, char const* name, uint32_t value)
{
  cJSON_AddNumberToObject(message->json, name, value);
  return RT_OK;
}

rtError
rtMessage_SetBool(rtMessage m, char const* name, bool b)
{
  cJSON_AddBoolToObject(m->json, name, b);
  return RT_OK;
}

rtError
rtMessage_GetBool(rtMessage const m, char const* name, bool* b)
{
  cJSON* p = cJSON_GetObjectItem(m->json, name);
  if (!p)
    return RT_FAIL;

  *b = (p->type == cJSON_True);
  return RT_OK;
}

/**
 * Add double field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param double value of the field to be added
 * @return rtError
 **/
rtError
rtMessage_SetDouble(rtMessage message, char const* name, double value)
{
  cJSON_AddItemToObject(message->json, name, cJSON_CreateNumber(value));
  return RT_OK;
}

/**
 * Add sub message field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param new message item to be added
 * @return rtError
 **/
rtError
rtMessage_SetMessage(rtMessage message, char const* name, rtMessage item)
{
  if (!message || !item)
    return RT_ERROR_INVALID_ARG;
  if (item->json)
  {
    cJSON* obj = cJSON_Duplicate(item->json, 1);
    cJSON_AddItemToObject(message->json, name, obj);
  }
  return RT_OK;
}

rtError
rtMessage_AddItemToArray(rtMessage message, rtMessage item)
{
  if (!message || !item)
    return RT_ERROR_INVALID_ARG;
  if (item->json)
  {
    cJSON* obj = cJSON_Duplicate(item->json, 1);
    cJSON_AddItemToArray(message->json, obj);
  }
  return RT_OK;
}


/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetString(rtMessage const  message, const char* name, char const** value)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valuestring;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get binary data from message
 * @param message to get the data from 
 * @param name of the field to get
 * @param ptr pointer to binary data (caller should free memory)
 * @param size of data buffer
 * @return rtError
 **/
rtError
rtMessage_GetBinaryData(rtMessage message, char const* name, void ** ptr, uint32_t *size)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    const unsigned char * value;
    value = (unsigned char *)p->valuestring;
    if(RT_OK == rtBase64_decode(value, strlen((const char *)value), ptr, size))
        return RT_OK;
    else
        return RT_FAIL;
  }
  return RT_FAIL;
}
/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @param size of value obtained
 * @return rtError
 **/
rtError
rtMessage_GetStringValue(rtMessage const message, char const* name, char* fieldvalue, int n)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    char *value = p->valuestring;
    int const len = (int) strlen(value);
    if (len <= n)
    {
      snprintf(fieldvalue, n, "%s", value);
      return RT_OK;
    }
    return RT_FAIL;
  }
  return RT_FAIL;
}

/**
 * Get field value of type integer using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to integer value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetInt32(rtMessage const message,const char* name, int32_t* value)
{  
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valueint;
    return RT_OK;
  }
  return RT_FAIL;
}

rtError
rtMessage_GetUInt32(rtMessage const message,const char* name, uint32_t* value)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valueint;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get field value of type double using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to double value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetDouble(rtMessage const  message, char const* name,double* value)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (p)
  {
    *value = p->valuedouble;
    return RT_OK;
  }
  return RT_FAIL;
}

/**
 * Get field value of type message using name
 * @param message to get field
 * @param name of the field
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessage(rtMessage const message, char const* name, rtMessage* clone)
{
  cJSON* p = cJSON_GetObjectItem(message->json, name);
  if (!p)
    return RT_PROPERTY_NOT_FOUND;
  *clone = (rtMessage) rt_try_malloc(sizeof(struct _rtMessage));
  if(!*clone)
    return rtErrorFromErrno(ENOMEM);
  (*clone)->json = cJSON_Duplicate(p, cJSON_True);
  (*clone)->count = 0;
  rt_atomic_fetch_add(&(*clone)->count, 1);
  return RT_OK;
}

/**
 * Get topic of message to be sent
 * @param message to get topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_GetSendTopic(rtMessage const m, char* topic)
{
  rtError err = RT_OK;
  cJSON* obj = cJSON_GetObjectItem(m->json, "_topic");
  if (obj)
    strcpy(topic, obj->valuestring);
  else
    err = RT_FAIL;
  return err;
}

/**
 * Set topic of message to be sent
 * @param message to set topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_SetSendTopic(rtMessage const m, char const* topic)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, "_topic");
  if (obj)
    cJSON_ReplaceItemInObject(m->json, "_topic", cJSON_CreateString(topic));
  else
    cJSON_AddItemToObject(m->json, "_topic", cJSON_CreateString(topic));
  if (obj)
    cJSON_Delete(obj);
  return RT_OK;
}

/**
 * Add string field to array in message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return rtError
 **/
rtError
rtMessage_AddString(rtMessage m, char const* name, char const* value)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
  {
    obj = cJSON_CreateArray();
    cJSON_AddItemToObject(m->json, name, obj);
  }
  cJSON_AddItemToArray(obj, cJSON_CreateString(value));
  return RT_OK;
}

/**
 * Add binary data to message
 * @param message to be modified
 * @param name of the field to be added
 * @param ptr pointer to binary data to be added
 * @param size of binary data to be added
 * @return rtError
 **/
rtError
rtMessage_AddBinaryData(rtMessage message, char const* name, void const * ptr, const uint32_t size)
{
  unsigned char * encoded_string = NULL;
  uint32_t encoded_string_size = 0;
  if (size == 0 || ptr == NULL)
  {
	  rtMessage_SetString(message, name,"");
	  return RT_OK;
  }
  if(RT_OK == rtBase64_encode((const unsigned char *)ptr, size, &encoded_string, &encoded_string_size))
  {
    rtMessage_SetString(message, name, (char *)encoded_string);
    free(encoded_string);
    return RT_OK;
  }
  else
  {
    return RT_FAIL;
  }
}
/**
 * Add message field to array in message
 * @param message to be modified
 * @param name of the field to be added
 * @param message to be added
 * @return rtError
 **/
rtError
rtMessage_AddMessage(rtMessage m, char const* name, rtMessage const item)
{
    if (!m || !item){
    return RT_ERROR_INVALID_ARG;
    }

  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
  {
    obj = cJSON_CreateArray();
    cJSON_AddItemToObject(m->json, name, obj);
  }
  if (item->json)
  {
    cJSON* item_obj = cJSON_Duplicate(item->json, 1);
    cJSON_AddItemToArray(obj, item_obj);
  }
  return RT_OK;
}

/**
 * Get length of array from message
 * @param message to get array length from
 * @param name of the array
 * @param fill length of array
 * @return rtError
 **/
rtError
rtMessage_GetArrayLength(rtMessage const m, char const* name, int32_t* length)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    *length = 0;
  else
    *length = cJSON_GetArraySize(obj);
  return RT_OK;
}
rtError
rtMessage_GetArrayIntItem(rtMessage const m, char const* name, int32_t idx, int* value)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    return RT_PROPERTY_NOT_FOUND;
  if (idx >= cJSON_GetArraySize(obj))
    return RT_FAIL;

  cJSON* item = cJSON_GetArrayItem(obj, idx);
  if (item)
  {
    *value = item->valueint;
    return RT_OK;
  }
  return RT_FAIL;
}
/**
 * Get string item from array in message
 * @param message to get string item from
 * @param name of the string item
 * @param index of array
 * @param value obtained
 * @param length of string item
 * @return rtError
 **/
rtError
rtMessage_GetStringItem(rtMessage const m, char const* name, int32_t idx, char const** value)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    return RT_PROPERTY_NOT_FOUND;
  if (idx >= cJSON_GetArraySize(obj))
    return RT_FAIL;

  cJSON* item = cJSON_GetArrayItem(obj, idx);
  if (item)
  {
    *value = item->valuestring;
    return RT_OK;
  }
  return RT_FAIL;
}
rtError
rtMessage_GetItemName(rtMessage const m, char const* name, int32_t idx, char const** ItemName)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    return RT_PROPERTY_NOT_FOUND;
  if (idx >= cJSON_GetArraySize(obj))
    return RT_FAIL;

  cJSON* item = cJSON_GetArrayItem(obj, idx);
  if (item)
  {
    *ItemName = item->string;
    return RT_OK;
  }
  return RT_FAIL;
}

rtError rtMessage_GetBytes(rtMessage message, void ** ptr, uint32_t *size)
{
    return rtMessage_GetBinaryData(message, NULL, ptr, size);
}

rtError rtMessage_SetBytes(rtMessage message, void const * ptr, const uint32_t size)
{
    return rtMessage_AddBinaryData(message, NULL, ptr, size);
}


/**
 * Get message item from array in parent message
 * @param message to get message item from
 * @param name of message item
 * @param index of array
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessageItem(rtMessage const m, char const* name, int32_t idx, rtMessage* msg)
{
  cJSON* obj = cJSON_GetObjectItem(m->json, name);
  if (!obj)
    return RT_PROPERTY_NOT_FOUND;
  if (idx >= cJSON_GetArraySize(obj))
    return RT_FAIL;
  *msg = (rtMessage) rt_try_malloc(sizeof(struct _rtMessage));
  if(!*msg)
    return rtErrorFromErrno(ENOMEM);  
  (*msg)->json = cJSON_Duplicate(cJSON_GetArrayItem(obj, idx), 1);
  (*msg)->count = 0;
  rt_atomic_fetch_add(&(*msg)->count, 1);
  return RT_OK;
}

/**
 * Increase reference count of message by 1
 * @param message
 * @return rtError
 **/
rtError
rtMessage_Retain(rtMessage m)
{
  rt_atomic_fetch_add(&m->count, 1);
  return RT_OK;
}

/**
 * Decrease reference count of message by 1 and destroy message if count is 0
 * @param message
 * @return rtError
 **/
rtError
rtMessage_Release(rtMessage m)
{
  if (m->count != 0)
    rt_atomic_fetch_sub(&m->count, 1);
  if (m->count == 0)
    rtMessage_Destroy(m);
  return RT_OK;
}
