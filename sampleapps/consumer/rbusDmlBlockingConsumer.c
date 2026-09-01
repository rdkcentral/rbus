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
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>

static rbusHandle_t handle;
#define TotalParams 6

char const*  paramNames[TotalParams] = {
    "Device.Blocking.Test0",
    "Device.NonBlocking.Test1",
    "Device.NonBlocking.Test2",
    "Device.NonBlocking.Test3",
    "Device.NonBlocking.Test4",
    "Device.NonBlocking.Test5"
};

int main(int argc, char *argv[])
{
    int rc;

    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    rc = rbus_open(&handle, "rbusSubConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
       printf("consumer: rbus_open failed: %d\n", rc);
       goto exit1;
    }
    rbus_setLogLevel(RBUS_LOG_DEBUG);
    rbusHandle_ConfigGetTimeout(handle, 2000);

    rbusValue_t value = NULL;
    int count =0;
    while(1)
    {
        if(count >= TotalParams)
            count = 0;
        rc = rbus_get(handle, paramNames[count], &value);
        sleep(0.1);
        if(rc == RBUS_ERROR_DESTINATION_NOT_FOUND)
            break;
        count++;
    }

    rbus_close(handle);
exit1:
    printf("constumer: exit\n");
    return 0;
}


