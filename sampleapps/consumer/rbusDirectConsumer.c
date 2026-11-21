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
#include <rtLog.h>

#define TotalParams   6
char componentName[] = "rbusDirectConsumer";

static void valueChangeHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    if (event)
    {
        rbusObject_fwrite(event->data, 0, stdout);

        printf("Consumer receiver ValueChange event for param %s\n", event->name);
    }
    else
        printf ("Invalid event received..\n");

    (void)handle;
    (void)subscription;
}

int main(int argc, char *argv[])
{
    int rc;
    rbusHandle_t    handle;

    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    rtLog_SetOption(RT_USE_RTLOGGER);
    {
        rbusValue_t value;
        printf("calling rbus get for [%s]\n", "Device.SampleProvider.SampleData.IntData");

        printf ("###############   GET 1 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        sleep(2);
        rbusHandle_t  directHNDL = NULL;
        rbusHandle_t  directHNDL2 = NULL;
        printf ("###############   OPEN DIRECT ################################################\n");
        rbus_openDirect(handle, &directHNDL, "Device.SampleProvider.SampleData.IntData");

        sleep(2);
        printf ("###############   OPEN DIRECT 2 ################################################\n");
        rbus_openDirect(handle, &directHNDL2, "Device.SampleProvider.AllTypes.StringData");

        sleep(5);
        printf ("###############   GET 2 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        sleep(4);
        printf ("###############   GET 3 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.AllTypes.StringData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        printf ("###############   Close Direct (after 15sec) #####################################################\n");
        sleep(15);
        rbus_closeDirect(directHNDL);
        sleep(3);
        rbus_closeDirect(directHNDL2);

        sleep(3);
        printf ("###############   GET 4 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        printf ("###############   GET 5 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.AllTypes.BoolData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        sleep(2);

        printf ("###############   OPEN DIRECT AGAIN  #####################################################\n");
        rbus_openDirect(handle, &directHNDL, "Device.SampleProvider.SampleData.IntData");
        printf ("###############   GET 6 #####################################################\n");
        rc = rbus_get(handle, "Device.SampleProvider.SampleData.IntData", &value);
        if (rc == RBUS_ERROR_SUCCESS)
        {
            rbusValue_fwrite(value, 0, stdout); printf("\n");
            rbusValue_Release(value);
        }

        sleep(15);
        rbus_closeDirect(directHNDL);
    (void) directHNDL;
    (void) directHNDL2;
    }

    //pause();

    sleep(10);



    printf ("###### Subscription ######\n");
    rbusHandle_t handle2;
    rc = rbus_open(&handle2, "EventConsumerTesting");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit2;
    }

    sleep(3);
    rbusHandle_t  directHandle = NULL;
    rc = rbus_openDirect(handle, &directHandle, "Device.SampleProvider.AllTypes.SingleData");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit3;
    }

    sleep(5);
    rc = rbusEvent_Subscribe(
        handle,
        "Device.SampleProvider.AllTypes.SingleData",
        valueChangeHandler,
        NULL,
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit4;
    }

    sleep(4);
    
    rc = rbusEvent_Subscribe(
        handle2,
        "Device.SampleProvider.AllTypes.SingleData",
        valueChangeHandler,
        NULL,
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit5;
    }

    sleep (25);
    rbusEvent_Unsubscribe(handle2, "Device.SampleProvider.AllTypes.StringData"); 
exit5:
    rbusEvent_Unsubscribe(handle, "Device.SampleProvider.AllTypes.StringData"); 
exit4:
    rbus_closeDirect(directHandle);
exit3:
    rbus_close(handle2);
exit2:
    rbus_close(handle);
exit1:
    printf("consumer: exit\n");
    return 0;
}


