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
#include "rtEncoder.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

rtError
rtEncoder_EncodeString(uint8_t** itr, char const* s, uint32_t* n)
{
  uint32_t len = 0;
  if (n)
    len = *n;
  else
    len = strlen(s);
  rtEncoder_EncodeUInt32(itr, len);
  memcpy(*itr, s, len);
  *itr += len;
  return RT_OK;
}

rtError
rtEncoder_DecodeStr(uint8_t const** itr, char* s, uint32_t len)
{
  memcpy(s, *itr, len);
  *itr += len;
  return RT_OK;
}

rtError
rtEncoder_DecodeString(uint8_t const** itr, char* s, uint32_t* n)
{
  uint32_t len = 0;
  rtEncoder_DecodeUInt32(itr, &len);
  memcpy(s, *itr, len);
  *n = len;
  *itr += len;
  return RT_OK;
}

rtError
rtEncoder_EncodeUInt16(uint8_t** itr, uint16_t n)
{
  uint16_t net = htons(n);
  memcpy(*itr, &net, 2);
  *itr += 2;
  return RT_OK;
}

rtError
rtEncoder_DecodeUInt16(uint8_t const** itr, uint16_t* n)
{
  uint16_t host = 0;
  memcpy(&host, *itr, 2);
  host = ntohs(host);
  *n = host;
  *itr += 2;
  return RT_OK;
}

rtError
rtEncoder_EncodeUInt32(uint8_t** itr, uint32_t n)
{
  uint32_t net = htonl(n);
  memcpy(*itr, &net, 4);
  *itr += 4;
  return RT_OK;
}

rtError
rtEncoder_DecodeUInt32(uint8_t const** itr, uint32_t* n)
{
  uint32_t host = 0;
  memcpy(&host, *itr, 4);
  host = ntohl(host);
  *n = host;
  *itr += 4;
  return RT_OK;
}

rtError rtEncoder_EncodeUInt64(uint8_t** ptr, uint64_t value)
{
    if (!ptr || !*ptr)
        return RT_ERROR_INVALID_ARG;
    (*ptr)[0] = (uint8_t)((value >> 56) & 0xFF);
    (*ptr)[1] = (uint8_t)((value >> 48) & 0xFF);
    (*ptr)[2] = (uint8_t)((value >> 40) & 0xFF);
    (*ptr)[3] = (uint8_t)((value >> 32) & 0xFF);
    (*ptr)[4] = (uint8_t)((value >> 24) & 0xFF);
    (*ptr)[5] = (uint8_t)((value >> 16) & 0xFF);
    (*ptr)[6] = (uint8_t)((value >> 8) & 0xFF);
    (*ptr)[7] = (uint8_t)(value & 0xFF);
    *ptr += 8;
    return RT_OK;
}

rtError rtEncoder_DecodeUInt64(uint8_t const** ptr, uint64_t* value)
{
    if (!ptr || !*ptr || !value)
        return RT_ERROR_INVALID_ARG;
    *value = ((uint64_t)(*ptr)[0] << 56) |
             ((uint64_t)(*ptr)[1] << 48) |
             ((uint64_t)(*ptr)[2] << 40) |
             ((uint64_t)(*ptr)[3] << 32) |
             ((uint64_t)(*ptr)[4] << 24) |
             ((uint64_t)(*ptr)[5] << 16) |
             ((uint64_t)(*ptr)[6] << 8)  |
             ((uint64_t)(*ptr)[7]);
    *ptr += 8;
    return RT_OK;
}
