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
#ifndef __RT_MESSAGING_H__
#define __RT_MESSAGING_H__

#include "rtError.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _rtMessage;
typedef struct _rtMessage* rtMessage;

/**
 * Allocate storage and initializes it as new message
 * @param pointer to the new message
 * @return rtError
 **/
rtError
rtMessage_Create(rtMessage* message);

/**
 * Copies only the data within the fields of the message
 * @param message to be copied
 * @param pointer to new copy of the message
 * @return rtError
 */
rtError
rtMessage_Clone(rtMessage const message, rtMessage* copy);

/* Allocates storage and initializes it as new message
 * @param pointer to the new message
 * @param fill the new message with this data
 * @return rtError
 **/
rtError
rtMessage_FromBytes(rtMessage* message, uint8_t const* buff, int n);

/**
 * Extract the data from a message as a byte sequence.
 * @param extract the data bytes from this message.
 * @param pointer to the byte sequence location
 * @param pointer to number of bytes in the message
 * @return rtError
 **/
rtError
rtMessage_ToByteArray(rtMessage message, uint8_t** buff, uint32_t* n);

/**
 * Extract the data from a message as a byte sequence. This takes the additional
 * argument of suggested_size, which will be used to allocate the memory on the heap.
 * If this is found to be insufficient, cJSON will realloc internally, but if the application
 * gets this right, costly realloc() calls can be avoided.
 * @param extract the data bytes from this message.
 * @param pointer to the byte sequence location
 * @param suggested size of buffer to allocate 
 * @param pointer to number of bytes in the message
 * @return rtError
 **/
rtError
rtMessage_ToByteArrayWithSize(rtMessage message, uint8_t** buff, uint32_t suggested_size, uint32_t* n);

/**
 * Free the buffer allocated by rtMessage_ToByteArray or rtMessage_ToByteArrayWithSize
 * @param pointer to the byte sequence to free
 * @return rtError
 **/
rtError
rtMessage_FreeByteArray(uint8_t* buff);

/**
 * Add string field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return void
 **/
void
rtMessage_SetString(rtMessage message, char const* name, char const* value);

/**
 * Add string field to array in message
 * @param message to be modified
 * @param name of the field to be added
 * @param value of the field to be added
 * @return rtError
 **/
rtError
rtMessage_AddString(rtMessage message, char const* name, char const* value);

/**
 * Add binary data to message
 * @param message to be modified
 * @param name of the field to be added
 * @param ptr pointer to the data buffer (ptr may be freed after this call)
 * @param size of the buffer
 * @return rtError
 **/
rtError
rtMessage_AddBinaryData(rtMessage message, char const* name, void const * ptr, const uint32_t size);
/**
 * Add message field to array in message
 * @param message to be modified
 * @param name of the field to be added
 * @param message to be added
 * @return rtError
 **/
rtError
rtMessage_AddMessage(rtMessage m, char const* name, rtMessage const item);

/**
 * Get length of array from message
 * @param message to get array length from
 * @param name of the array
 * @param fill length of array
 * @return rtError
 **/
rtError
rtMessage_GetArrayLength(rtMessage const m, char const* name, int32_t* length);

/**
 * Get string item from array in message
 * @param message to get string item from
 * @param name of the string item
 * @param index of array
 * @param value obtained
 * @return rtError
 **/
rtError
rtMessage_GetStringItem(rtMessage const m, char const* name, int32_t idx, char const** value);

/**
 * Get message item from array in parent message
 * @param message to get message item from
 * @param name of message item
 * @param index of array
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessageItem(rtMessage const m, char const* name, int32_t idx, rtMessage* msg);

/**
 * Add integer field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param integer value of the field to be added
 * @return rtError
 **/
rtError
rtMessage_SetInt32(rtMessage message, char const* name, int32_t value);

/**
 * Add double field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param double value of the field to be added
 * @return rtError 
 **/
rtError
rtMessage_SetDouble(rtMessage message, char const* name, double value);

/**
 * Add sub message field to the message
 * @param message to be modified
 * @param name of the field to be added
 * @param new message item to be added
 * @return rtError
 **/
rtError
rtMessage_SetMessage(rtMessage message, char const* name, rtMessage item);

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetString(rtMessage const m, char const* name, char const** value);
/**
 * Get binary data from message
 * @param message to be read from
 * @param name of the field to get
 * @param ptr pointer to data buffer (caller must free)
 * @param size of the data buffer
 * @return rtError
 **/
rtError
rtMessage_GetBinaryData(rtMessage message, char const* name, void ** ptr, uint32_t *size);

/**
 * Get field value of type string using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to string value obtained.
 * @param size of value obtained
 * @return rtError
 **/
rtError
rtMessage_GetStringValue(rtMessage const m, char const* name, char* value, int n);

/**
 * Get field value of type integer using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to integer value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetInt32(rtMessage const m, char const* name, int32_t* value);

/**
 * Get field value of type double using field name.
 * @param message to get field
 * @param name of the field
 * @param pointer to double value obtained.
 * @return rtError
 **/
rtError
rtMessage_GetDouble(rtMessage const m, char const* name, double* value);

/**
 * Get field value of type message using name
 * @param message to get field
 * @param name of the field
 * @param message obtained
 * @return rtError
 **/
rtError
rtMessage_GetMessage(rtMessage const m, char const* name, rtMessage* item);

/**
 * Format a message as string
 * @param message to be formatted
 * @param pointer to a string where message is to be stored
 * @param string length
 * @return rtError
 **/
rtError
rtMessage_ToString(rtMessage m, char** s, uint32_t* n);

/**
 * Get topic of message to be sent
 * @param message to get topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_GetSendTopic(rtMessage const m, char* topic);

/**
 * Set topic of message to be sent
 * @param message to set topic
 * @param name of the topic
 * @return rtError
 **/
rtError
rtMessage_SetSendTopic(rtMessage m, char const* topic);

rtError
rtMessage_SetBool(rtMessage const m, char const* name, bool b);

rtError
rtMessage_GetBool(rtMessage const m, char const* name, bool* b);

/**
 * Increase reference count of message by 1
 * @param message
 * @return rtError
 **/
rtError
rtMessage_Retain(rtMessage m);

/**
 * Decrease reference count of message by 1 and destroy message if count is 0
 * @param message
 * @return rtError
 **/
rtError
rtMessage_Release(rtMessage m);

#ifdef __cplusplus
}
#endif
#endif
