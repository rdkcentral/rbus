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

int loopFor = 24;

static void generalEvent1Handler(
    rbusHandle_t handle,
    rbusEventRawData_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    printf("\nevent_receive_handler1 called\r\n");
    printf("Event received %s\r\n", event->name);
    printf("Event data: %s\r\n", (char*)event->rawData);
    printf("Event data len: %d\r\n", event->rawDataLen);
    printf("\r\n");
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    rbusHandle_t directHandle = NULL;
    char* data[2] = { "My Data 1", "My Data2" };
    rbusEventSubscription_t subscriptions[1] = {
        {"Device.Provider1.Event1!", NULL, 0, 0, generalEvent1Handler, data[0], NULL, NULL, false},
    };

    printf("consumer: start\n");

    rc = rbus_open(&handle, "RawDataEventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit;
    }

    if(argc == 2 && strcmp(argv[1], "opendirect")==0)
    {
        rc = rbus_openDirect(handle, &directHandle, "Device.Provider1.Event1!");
        if (RBUS_ERROR_SUCCESS != rc)
        {
            printf ("Failed to open direct connection to %s\r\n", "Device.Provider1.Event1!");
        }
        else
        {
            printf("******* In rbus_openDirect Mode *******\n");
        }
    }
    printf("-----------------------------------------------------\n");
    printf("Testing rbusEvent_SubscribeRawData\n");
    printf("-----------------------------------------------------\n");
    rc = rbusEvent_SubscribeRawData(
        handle,
        "Device.Provider1.Event1!",
        (rbusEventHandler_t)generalEvent1Handler,
        data[0],
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit;
    }

    sleep(loopFor/4);
    printf("Unsubscribing from Event1\n");
    rbusEvent_UnsubscribeRawData(handle, "Device.Provider1.Event1!");

    printf("-----------------------------------------------------\n");
    printf("Testing rbusEvent_SubscribeExRawData\n");
    printf("-----------------------------------------------------\n");
    rc = rbusEvent_SubscribeExRawData(handle, subscriptions, 1, 0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_SubscribeExRawData 1 failed: %d\n", rc);
        goto exit;
    }

    sleep(loopFor/4);

    if(argc == 2 && strcmp(argv[1], "opendirect")==0)
    {
        rc = rbus_closeDirect(directHandle);
        if (RBUS_ERROR_SUCCESS != rc)
        {
            printf ("Failed to close direct connection to %s\r\n", "Device.Provider1.Event1!");
        }
    }
    rbusEvent_UnsubscribeExRawData(handle, subscriptions, 1);

exit:
    rbus_close(handle);
    printf("consumer: exit\n");
    return rc;
}
