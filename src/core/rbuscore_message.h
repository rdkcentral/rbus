/*
  * If not stated otherwise in this file or this component's Licenses.txt file
  * the following copyright and licenses apply:
  *
  * Copyright 2016 RDK Management
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

#ifndef __RBUS_CORE_MSG_H__
#define __RBUS_CORE_MSG_H__ 

#include <rtError.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _rbusMessage;
typedef struct _rbusMessage* rbusMessage;

void rbusMessage_Init(rbusMessage* message);
void rbusMessage_Retain(rbusMessage message);
void rbusMessage_Release(rbusMessage message);
void rbusMessage_FromBytes(rbusMessage* message, uint8_t const* buff, uint32_t n);
void rbusMessage_ToBytes(rbusMessage message, uint8_t** buff, uint32_t* n);
void rbusMessage_ToDebugString(rbusMessage message, char** s, uint32_t* n);

/*data types*/
rtError rbusMessage_SetString(rbusMessage message, char const* value);
rtError rbusMessage_GetString(rbusMessage const message, char const** value);

rtError rbusMessage_SetBytes(rbusMessage message, uint8_t const* value, uint32_t size);
rtError rbusMessage_GetBytes(rbusMessage message, uint8_t const** value, uint32_t *size);

rtError rbusMessage_SetInt32(rbusMessage message, int32_t value);
rtError rbusMessage_GetInt32(rbusMessage const message, int32_t* value);
rtError rbusMessage_GetUInt32(rbusMessage const message, uint32_t* value);

rtError rbusMessage_SetInt64(rbusMessage message, int64_t value);
rtError rbusMessage_GetInt64(rbusMessage const message, int64_t* value);

rtError rbusMessage_SetDouble(rbusMessage message, double value);
rtError rbusMessage_GetDouble(rbusMessage const message, double* value);

rtError rbusMessage_SetMessage(rbusMessage message, rbusMessage const item);
rtError rbusMessage_GetMessage(rbusMessage const message, rbusMessage* value);

#ifdef __cplusplus
}
#endif
#endif


