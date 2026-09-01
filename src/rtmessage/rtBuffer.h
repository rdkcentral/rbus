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
#ifndef __RT_BUFFER_H__
#define __RT_BUFFER_H__

#include "rtError.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _rtBuffer;
typedef struct _rtBuffer* rtBuffer;

rtError rtBuffer_Create(rtBuffer* buff);
rtError rtBuffer_CreateFromBytes(rtBuffer* buff, uint8_t* b, int n);
rtError rtBuffer_Destroy(rtBuffer buff);
rtError rtBuffer_Retain(rtBuffer buff);
rtError rtBuffer_Release(rtBuffer buff);
rtError rtBuffer_WriteInt32(rtBuffer buff, int32_t n);
rtError rtBuffer_WriteString(rtBuffer buff, char const* s, int n);
rtError rtBuffer_ReadInt32(rtBuffer buff, int32_t* n);
rtError rtBuffer_ReadString(rtBuffer buff, char** s, int* n);

#ifdef __cplusplus
}
#endif
#endif
