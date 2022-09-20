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
#ifndef __RT_ENCODER_H__
#define __RT_ENCODER_H__

#include "rtError.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

rtError rtEncoder_EncodeInt32(uint8_t** itr, int32_t n);
rtError rtEncoder_DecodeInt32(uint8_t const** itr, int32_t* n);
rtError rtEncoder_EncodeUInt32(uint8_t** itr, uint32_t n);
rtError rtEncoder_DecodeUInt32(uint8_t const** itr, uint32_t* n);
rtError rtEncoder_EncodeUInt16(uint8_t** itr, uint16_t n);
rtError rtEncoder_DecodeUInt16(uint8_t const** itr, uint16_t* n);
rtError rtEncoder_DecodeStr(uint8_t const** itr, char* s, uint32_t len);
rtError rtEncoder_EncodeString(uint8_t** itr, char const* s, uint32_t* n);
rtError rtEncoder_DecodeString(uint8_t const** itr, char* s, uint32_t* n);

#ifdef __cplusplus
}
#endif
#endif
