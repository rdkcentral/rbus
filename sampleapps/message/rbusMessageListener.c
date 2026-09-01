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
#include <rbus.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int running = 1;

static void rbusMessageHandler(rbusHandle_t handle, rbusMessage_t* msg, void * userData)
{
    (void)handle;
    (void)userData;
    printf("rbusMessageHandler topic=%s data=%.*s\n", msg->topic, msg->length, (char const *)msg->data);

    if(strstr((const char*)msg->data, "Goodbye") != 0)
    {
        printf("quiting app\n");     
        running = 0;
    }
}

int main()
{
    rbusError_t err;
    rbusHandle_t rbus;

    err = rbus_open(&rbus, "rbus_recv");
    if (err)
    {
        fprintf(stderr, "rbus_open:%s\n", rbusError_ToString(err));
        return 1;
    }

    rbusMessage_AddListener(rbus, "A.B.C", &rbusMessageHandler, NULL, 0);

    while (running)
    {
        sleep(1);
    }

    rbus_close(rbus);

    return 0;
}
