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
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void onMessage(rtMessageHeader const* hdr, uint8_t const* p, uint32_t n, void* closure)
{
  rtMessage m;
  char* str;
  uint32_t num;

  (void) closure;

  rtLog_Info("onMessage topic: %s", hdr->topic);

  rtMessage_FromBytes(&m, p, n);

  rtMessage_ToString(m, &str, &num);
  rtLog_Info("onMessage message: %s", str);
  free(str);

  if(!strcmp(hdr->topic, "A.B.C"))
  {
    rtMessage item;
    rtMessage_GetMessage(m, "new", &item);
    rtMessage_ToString(item, &str, &num);
    rtLog_Info("onMessage subitem: %s", str);
    free(str);
  }

  rtMessage_Release(m);
}

int main()
{
  rtConnection con;

  rtLog_SetLevel(RT_LOG_DEBUG);
  rtConnection_Create(&con, "APP2", "unix:///tmp/rtrouted");
  rtConnection_AddListener(con, "A.*.C", onMessage, NULL);
  rtConnection_AddListener(con, "A.B.C.*", onMessage, NULL);

  pause();

  rtConnection_Destroy(con);
  return 0;
}
