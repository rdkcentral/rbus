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

#include <stdlib.h>
#include <unistd.h>

int main()
{
  rtError err;
  int i;

  rtLog_SetLevel(RT_LOG_DEBUG);

  rtConnection con;
  rtConnection_Create(&con, "APP1", "tcp://127.0.0.1:10001");


  for(i = 0 ; i < 5; i++) 
  {
    rtMessage res;
    rtMessage req;
    rtMessage_Create(&req);
    rtMessage_SetString(req, "field1", "hello world");

    err = rtConnection_SendRequest(con, req, "RDK.MODEL.PROVIDER1", &res, 2000);
    rtLog_Info("SendRequest:%s", rtStrError(err));

    if (err == RT_OK)
    {
      char* p = NULL;
      uint32_t len = 0;

      rtMessage_ToString(res, &p, &len);
      rtLog_Info("\tres:%.*s\n", len, p);

      free(p);
      rtMessage_Release(res);
    }

    rtMessage_Release(req);

    sleep(1);
  }

  rtConnection_Destroy(con);
  return 0;
}

