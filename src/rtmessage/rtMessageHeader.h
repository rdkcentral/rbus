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
#ifndef __RT_MESSAGE_HEADER_H__
#define __RT_MESSAGE_HEADER_H__


#include "rtError.h"
#include <stdint.h>
#ifdef MSG_ROUNDTRIP_TIME
#include <time.h>
#endif

#define RTMSG_HEADER_MAX_TOPIC_LENGTH 128

// size of all fields in 
// #define RTMSG_HEADER_SIZE (24 + (2 * RTMSG_HEADER_MAX_TOPIC_LENGTH))

#ifdef __cplusplus
extern "C" {
#endif

#define RTMSG_HEADER_VERSION 2
#define RTMSG_HEADER_MARKER 0xAAAA
#define RTMESSAGEHEADER_PREAMBLE_LENGTH 6
#define RTMESSAGEHEADER_HDR_LENGTH_OFFSET 4
typedef enum
{
  rtMessageFlags_Request = 0x01,
  rtMessageFlags_Response = 0x02,
  rtMessageFlags_Undeliverable = 0x04,
  rtMessageFlags_Tainted = 0x08,
  rtMessageFlags_RawBinary = 0x10,
  rtMessageFlags_Encrypted = 0x20
} rtMessageFlags;

typedef struct
{
  uint16_t version;
  uint16_t header_length;
  uint32_t sequence_number;
  uint32_t flags;
  uint32_t control_data;
  uint32_t payload_length;
  uint32_t topic_length;
  char     topic[RTMSG_HEADER_MAX_TOPIC_LENGTH];
  uint32_t reply_topic_length;
  char     reply_topic[RTMSG_HEADER_MAX_TOPIC_LENGTH];
#ifdef MSG_ROUNDTRIP_TIME
  time_t   T1; /* Time at which consumer sends the request to daemon */
  time_t   T2; /* Time at which daemon receives the message from consumer */
  time_t   T3; /* Time at which daemon writes to provider socket */
  time_t   T4; /* Time at which provider sends back the response */
  time_t   T5; /* Time at which daemon received the response */
#endif
} rtMessageHeader;

rtError rtMessageHeader_Init(rtMessageHeader* hdr);
rtError rtMessageHeader_Encode(rtMessageHeader* hdr, uint8_t* buff);
rtError rtMessageHeader_Decode(rtMessageHeader* hdr, uint8_t const* buff);
rtError rtMessageHeader_SetIsRequest(rtMessageHeader* hdr);
int     rtMessageHeader_IsRequest(rtMessageHeader const* hdr);

#ifdef __cplusplus
}
#endif
#endif
