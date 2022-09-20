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
# This file contains material from pxCore which is Copyright 2005-2018 John Robinson
# Licensed under the Apache-2.0 license.
*/
// rtError.h

#ifndef RT_ERROR_H
#define RT_ERROR_H

#include <stdint.h>
#include <errno.h>

#ifdef RT_DEBUG
#include <assert.h>
#define RT_ASSERT(X) assert((X))
#else
#define RT_ASSERT(X) if (!(X)) rtLogError("rt assert: '%s' failed", #X);
#endif

// TODO review base numbering scheme for different error classes... general vs rtremote vs... 
#define RT_ERROR_CLASS_SYSERROR 0x8000
#define RT_ERROR_CLASS_BUILTIN 0x00000000

#define RT_OK                           0
#define RT_ERROR                        1
#define RT_FAIL                         1
#define RT_ERROR_NOT_ENOUGH_ARGS        2
#define RT_ERROR_INVALID_ARG            3
#define RT_PROP_NOT_FOUND               4
#define RT_OBJECT_NOT_INITIALIZED       5
#define RT_PROPERTY_NOT_FOUND           6
#define RT_OBJECT_NO_LONGER_AVAILABLE   7

#define RT_RESOURCE_NOT_FOUND		8
#define RT_NO_CONNECTION		9
#define RT_ERROR_NOT_IMPLEMENTED 10
#define RT_ERROR_TYPE_MISMATCH 11

// errors specific to rtRemote
#define RT_ERROR_TIMEOUT 1000
#define RT_ERROR_DUPLICATE_ENTRY 1001
#define RT_ERROR_OBJECT_NOT_FOUND 1002
#define RT_ERROR_PROTOCOL_ERROR 1003
#define RT_ERROR_INVALID_OPERATION 1004
#define RT_ERROR_IN_PROGRESS 1005
#define RT_ERROR_QUEUE_EMPTY 1006
#define RT_ERROR_STREAM_CLOSED 1007

#define RT_CHECK(X) if(!(X)){return;}
#define RT_CHECK_R(X,Y) if(!(X)){return Y;}
#define RT_CHECK_INVALID_ARG(X) RT_CHECK_R(X,RT_ERROR_INVALID_ARG);
#define RT_CHECK_NO_MEM(X) RT_CHECK_R(X,rtErrorFromErrno(ENOMEM));

typedef uint32_t rtError;

#ifdef __cplusplus
extern "C" {
#endif

const char* rtStrError(rtError e);

rtError rtErrorGetLastError();
rtError rtErrorFromErrno(int err);
void    rtErrorSetLastError(rtError e);

#ifdef __cplusplus
}
#endif
#endif
