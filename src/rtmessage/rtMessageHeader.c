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
#include <inttypes.h>
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
  static uint16_t const kSizeWithoutStringsInBytes = 72; /* 28 bytes for basic data +
                                                             4 bytes for Marker +
                                                            40 bytes for timestamp */
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
  rtEncoder_EncodeUInt64(&ptr, hdr->T1);
  rtEncoder_EncodeUInt64(&ptr, hdr->T2);
  rtEncoder_EncodeUInt64(&ptr, hdr->T3);
  rtEncoder_EncodeUInt64(&ptr, hdr->T4);
  rtEncoder_EncodeUInt64(&ptr, hdr->T5);
#endif
  rtEncoder_EncodeUInt16(&ptr, RTMSG_HEADER_MARKER);
  return RT_OK;
}

rtError
rtMessageHeader_Decode(rtMessageHeader* hdr, uint8_t const* buff)
{
  RT_CHECK_INVALID_ARG(hdr);
  RT_CHECK_INVALID_ARG(buff);
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
  rtEncoder_DecodeStr(&ptr, hdr->topic, hdr->topic_length);

  rtEncoder_DecodeUInt32(&ptr, &hdr->reply_topic_length);
  if(hdr->reply_topic_length >= sizeof(hdr->reply_topic)) {
    rtLog_Warn("RTROUTED_INVALID_LENGTH: rtMessageHeader_Decode() - reply_topic_length %d", hdr->reply_topic_length);
    return RT_ERROR;
  }
  rtEncoder_DecodeStr(&ptr, hdr->reply_topic, hdr->reply_topic_length);
#ifdef MSG_ROUNDTRIP_TIME
  /* The timestamp block is optional on older header formats. Use header_length
     to detect whether the 5x uint64 timestamp fields are present to avoid
     reading past the received header when talking with older peers. */
  {
    size_t bytes_read_so_far = (size_t)(ptr - buff);
    size_t remaining_in_header = 0;
    if (hdr->header_length > bytes_read_so_far)
      remaining_in_header = (size_t)hdr->header_length - bytes_read_so_far;

    if (remaining_in_header >= (5 * sizeof(uint64_t)))
    {
      rtEncoder_DecodeUInt64(&ptr, &hdr->T1);
      rtEncoder_DecodeUInt64(&ptr, &hdr->T2);
      rtEncoder_DecodeUInt64(&ptr, &hdr->T3);
      rtEncoder_DecodeUInt64(&ptr, &hdr->T4);
      rtEncoder_DecodeUInt64(&ptr, &hdr->T5);
    }
    else if (remaining_in_header >= (5 * sizeof(uint32_t)))
    {
      uint32_t lt1 = 0, lt2 = 0, lt3 = 0, lt4 = 0, lt5 = 0;
      rtEncoder_DecodeUInt32(&ptr, &lt1);
      rtEncoder_DecodeUInt32(&ptr, &lt2);
      rtEncoder_DecodeUInt32(&ptr, &lt3);
      rtEncoder_DecodeUInt32(&ptr, &lt4);
      rtEncoder_DecodeUInt32(&ptr, &lt5);
      hdr->T1 = (uint64_t)lt1 * (uint64_t)1000000000ULL;
      hdr->T2 = (uint64_t)lt2 * (uint64_t)1000000000ULL;
      hdr->T3 = (uint64_t)lt3 * (uint64_t)1000000000ULL;
      hdr->T4 = (uint64_t)lt4 * (uint64_t)1000000000ULL;
      hdr->T5 = (uint64_t)lt5 * (uint64_t)1000000000ULL;
    }
    else
    {
      /* Timestamps not present in this header version; zero them for safety. */
      hdr->T1 = hdr->T2 = hdr->T3 = hdr->T4 = hdr->T5 = 0;
    }
  }
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
