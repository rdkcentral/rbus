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

static void on_response(const rtMessageHeader* header, const uint8_t* buff, uint32_t n, void* user_data)
{
  rtLog_Info("--- begin sync response --- ");
  rtLog_Info("topic         : %s", header->topic);
  rtLog_Info("async_response: '%.*s'", n, (char *) buff);
  rtLog_Info("async_context : '%s'", (char *) user_data);
  rtLog_Info("---  end async response --- ");
  free(user_data);
}

int main()
{
  int i;

  rtLog_SetLevel(RT_LOG_DEBUG);

  rtConnection con;
  rtConnection_Create(&con, "APP1", "tcp://127.0.0.1:10001");

  i = 1;
  while (true)
  {
    char buff[64];
    char* s = malloc(16);

    snprintf(s, 16, "ctx-%d", i);
    snprintf(buff, sizeof(buff), "Hello World:%d", i);

    rtError err = rtConnection_SendRequestAsync(con, (uint8_t *)buff, strlen(buff),
      "RDK.MODEL.PROVIDER1", on_response, s, -1, NULL);
    rtLog_Info("SendRequest:%s", rtStrError(err));

    usleep(1000 * 500);

    i++;
  }

  while (1) {
    sleep(1);
  }

  rtConnection_Destroy(con);

  return 0;
}

