/*
  * If not stated otherwise in this file or this component's Licenses.txt file
  * the following copyright and licenses apply:
  *
  * Copyright 2016 RDK Management
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <rbus.h>
#include <src/core/rbuscore.h>
#include <src/session_manager/rbus_session_mgr.h>
#include "rtLog.h"


int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    rtLog_SetLevel(RT_LOG_INFO);

    rbusHandle_t handle = NULL;
    unsigned int sessionId = 0 , newSessionId = 0;
    char *componentName = NULL;

    componentName = strdup("sessiontest");

    rbus_open(&handle, componentName);
    rbus_createSession(handle, &sessionId);
    rbus_getCurrentSession(handle, &newSessionId);
    rbus_closeSession(handle, sessionId); 
    rbus_close(handle);

    return 0;
}
