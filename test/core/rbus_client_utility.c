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
#include "rbus_core.h"

#include "rtLog.h"

#define OBJ1_NAME "foo"
#define OBJ2_NAME "bar"
static char buffer[100];


static void dumpMessage(rbusMessage message)
{
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("dumpMessage: %.*s\n", buff_length, buff);
    free(buff);
}

int main(int argc, char *argv[])
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: sample_client <name of object to pull>\n");
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection("everyclient")) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }


    /*Pull the object from remote end.*/
    rbusMessage response;
    if((err = rbuscore_pullObj(argv[1], 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", argv[1]);
        dumpMessage(response);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", argv[1]);
    }

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
