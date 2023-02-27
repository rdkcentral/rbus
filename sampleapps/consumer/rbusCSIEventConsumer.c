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
#include <string.h>
#include <rbus.h>
#include <rtTime.h>
#include <rtLog.h>
#include <unistd.h>

static void bigDataEventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void) subscription;
    rtTime_t t;
    char timebuf[200] = "";
    rtLog_Warn("Consumer receiver big event for param %s :: %s", rtTime_ToString (rtTime_Now(&t), timebuf),event->name);

    (void)handle;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;

    printf("constumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit4;
    }

    rc = rbusEvent_Subscribe(handle, "Device.SampleProvider.BigData", bigDataEventHandler, NULL, 0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit3;
    }

    pause();
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event2!");

exit3:
    rbus_close(handle);

exit4:
    printf("consumer: exit\n");
    return rc;
}


