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

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtError.h"
#define RBUS_BINARY_DATA_SIZE_LIMIT 1026
#define RBUS_BASE64_DATA_SIZE_LIMIT (RBUS_BINARY_DATA_SIZE_LIMIT * 4 / 3)

/* Note: out_size accounts for the string terminator as well.*/
rtError rtBase64_encode(const void * in, const unsigned int in_size, unsigned char ** out, unsigned int *out_size);

/* Note: in_size shouldn't account for the string terminator (if present).*/
rtError rtBase64_decode(const unsigned char * in, const unsigned int in_size,  void ** out, unsigned int *out_size);

#ifdef __cplusplus
}
#endif
#endif

