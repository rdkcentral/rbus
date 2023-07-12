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
