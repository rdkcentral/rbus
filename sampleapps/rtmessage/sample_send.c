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
#include "rtVector.h"

#include <unistd.h>

int main()
{
  rtError err;
  int count = 0;

  rtLog_SetLevel(RT_LOG_INFO);

  rtConnection con;
  rtConnection_Create(&con, "APP1", "tcp://127.0.0.1:10001");
//  rtConnection_Create(&con, "APP1", "unix:///tmp/rtrouted");

  while (1)
  {
    rtMessage m;
    rtMessage_Create(&m);
    rtMessage_SetString(m, "description", "message sent to A.B.C");
    rtMessage_SetInt32(m, "field1", count++);
    rtMessage_SetString(m, "field2", "hello world");
    rtMessage item;
    rtMessage_Create(&item);
    rtMessage_SetString(item, "field3", "I am extra message");
    rtMessage_SetMessage(m, "new", item);

    err = rtConnection_SendMessage(con, m, "A.B.C");
    rtLog_Info("send[%s]: %s", "A.B.C", rtStrError(err));

    rtMessage_Release(m);
    sleep(1);

    rtMessage_Create(&m);
    rtMessage_SetString(m, "description", "message sent to A.B.C.FOO.BAR");
    rtMessage_SetInt32(m, "field1", 1234);

    err = rtConnection_SendMessage(con, m, "A.B.C.FOO.BAR");
    rtLog_Info("send[%s]: %s", "A.B.C.FOO.BAR", rtStrError(err));

    rtMessage_Release(m);
    sleep(1);
  }

  rtConnection_Destroy(con);
  return 0;
}

