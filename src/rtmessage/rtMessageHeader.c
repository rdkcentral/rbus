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
#include "rtMessageHeader.h"
#include "rtEncoder.h"
#include "rtLog.h"

#include <string.h>

rtError
rtMessageHeader_Init(rtMessageHeader* hdr)
{
  hdr->version = RTMSG_HEADER_VERSION;
  hdr->header_length = 0;
  hdr->sequence_number = 0;
  hdr->flags = 0;
  hdr->control_data = 0;
  hdr->payload_length = 0;
  hdr->topic_length = 0;
  memset(hdr->topic, 0, RTMSG_HEADER_MAX_TOPIC_LENGTH);
  hdr->reply_topic_length = 0;
  memset(hdr->reply_topic, 0, RTMSG_HEADER_MAX_TOPIC_LENGTH);
#ifdef MSG_ROUNDTRIP_TIME
  hdr->T1 = 0;
  hdr->T2 = 0;
  hdr->T3 = 0;
  hdr->T4 = 0;
  hdr->T5 = 0;
#endif
  return RT_OK;
}

rtError
rtMessageHeader_Encode(rtMessageHeader* hdr, uint8_t* buff)
{
  uint8_t* ptr = buff;
#ifdef MSG_ROUNDTRIP_TIME
  static uint16_t const kSizeWithoutStringsInBytes = 52; /* 28 bytes for basic data +
                                                             4 bytes for Marker +
                                                            20 bytes for timestamp */
#else
  static uint16_t const kSizeWithoutStringsInBytes = 32; /* 28 bytes for basic data +
                                                             4 bytes for Marker */
#endif
  uint16_t length = kSizeWithoutStringsInBytes
    + strlen(hdr->topic)
    + strlen(hdr->reply_topic);
  hdr->header_length = length;
  rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
  rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_VERSION);
  rtEncoder_EncodeUInt16(&ptr, hdr->header_length);
  rtEncoder_EncodeUInt32(&ptr, hdr->sequence_number); // con->sequence_number++);
  rtEncoder_EncodeUInt32(&ptr, hdr->flags);
  rtEncoder_EncodeUInt32(&ptr, hdr->control_data);
  rtEncoder_EncodeUInt32(&ptr, hdr->payload_length);
  rtEncoder_EncodeString(&ptr, hdr->topic, NULL);
  rtEncoder_EncodeString(&ptr, hdr->reply_topic, NULL);
#ifdef MSG_ROUNDTRIP_TIME
  rtEncoder_EncodeUInt32(&ptr, hdr->T1);
  rtEncoder_EncodeUInt32(&ptr, hdr->T2);
  rtEncoder_EncodeUInt32(&ptr, hdr->T3);
  rtEncoder_EncodeUInt32(&ptr, hdr->T4);
  rtEncoder_EncodeUInt32(&ptr, hdr->T5);
#endif
  rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
  return RT_OK;
}

rtError
rtMessageHeader_Decode(rtMessageHeader* hdr, uint8_t const* buff)
{
  uint8_t const* ptr = buff;
  uint16_t marker = 0;
  rtEncoder_DecodeUInt16(&ptr, &marker);
  if(RTMSG_HEADER_MARKER != marker)
    return RT_ERROR;
  rtEncoder_DecodeUInt16(&ptr, &hdr->version);
  rtEncoder_DecodeUInt16(&ptr, &hdr->header_length);
  rtEncoder_DecodeUInt32(&ptr, &hdr->sequence_number);
  rtEncoder_DecodeUInt32(&ptr, &hdr->flags);
  rtEncoder_DecodeUInt32(&ptr, &hdr->control_data);
  rtEncoder_DecodeUInt32(&ptr, &hdr->payload_length);

  rtEncoder_DecodeUInt32(&ptr, &hdr->topic_length);
  if(hdr->topic_length >= sizeof(hdr->topic)) {
    rtLog_Warn("RTROUTED_INVALID_LENGTH: rtMessageHeader_Decode() - topic_length %d", hdr->topic_length);
    return RT_ERROR;
  }
  if(NULL == ptr) {
    rtLog_Warn("RTROUTED_INVALID_PTR: rtMessageHeader_Decode() - PTR is NULL");
    return RT_ERROR;
  }
  rtEncoder_DecodeStr(&ptr, hdr->topic, hdr->topic_length);

  rtEncoder_DecodeUInt32(&ptr, &hdr->reply_topic_length);
  if(hdr->reply_topic_length >= sizeof(hdr->reply_topic)) {
    rtLog_Warn("RTROUTED_INVALID_LENGTH: rtMessageHeader_Decode() - reply_topic_length %d", hdr->reply_topic_length);
    return RT_ERROR;
  }
  if(NULL == ptr) {
    rtLog_Warn("RTROUTED_INVALID_PTR: rtMessageHeader_Decode() - PTR is NULL");
    return RT_ERROR;
  }
  rtEncoder_DecodeStr(&ptr, hdr->reply_topic, hdr->reply_topic_length);
#ifdef MSG_ROUNDTRIP_TIME
  rtEncoder_DecodeUInt32(&ptr, (uint32_t*)&hdr->T1);
  rtEncoder_DecodeUInt32(&ptr, (uint32_t*)&hdr->T2);
  rtEncoder_DecodeUInt32(&ptr, (uint32_t*)&hdr->T3);
  rtEncoder_DecodeUInt32(&ptr, (uint32_t*)&hdr->T4);
  rtEncoder_DecodeUInt32(&ptr, (uint32_t*)&hdr->T5);
#endif

  rtEncoder_DecodeUInt16(&ptr, &marker);
  if(RTMSG_HEADER_MARKER != marker)
    return RT_ERROR;
  return RT_OK;
}

rtError
rtMessageHeader_SetIsRequest(rtMessageHeader* hdr)
{
  hdr->flags |= rtMessageFlags_Request;
  return RT_OK;
}

int
rtMessageHeader_IsRequest(rtMessageHeader const* hdr)
{
  return ((hdr->flags & rtMessageFlags_Request) == rtMessageFlags_Request ? 1 : 0);
}
