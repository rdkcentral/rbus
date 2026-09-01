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
#include <rbus.h>


int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    rbus_setLogLevel(RBUS_LOG_INFO);

    rbusHandle_t handle = NULL;
    unsigned int sessionId = 0 , newSessionId = 0;

    rbus_open(&handle, "sessiontest");
    rbus_createSession(handle, &sessionId);
    printf ("Created Session ID %u\n", sessionId);

    rbus_getCurrentSession(handle, &newSessionId);
    printf ("Current Session ID %u\n", newSessionId);

    rbus_closeSession(handle, sessionId); 
    rbus_close(handle);

    return 0;
}
