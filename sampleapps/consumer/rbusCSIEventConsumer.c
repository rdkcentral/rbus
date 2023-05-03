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
    rtLog_Info ("Consumer received %s event at %s", event->name, rtTime_ToString (rtTime_Now(&t), timebuf));

    (void)handle;
}

const char * pDMLName[] = {"Device.WiFi.X_RDK_CSI.1.data", "Device.WiFi.X_RDK_CSI.1.sampleData"};

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    int index = 0;

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    rbusHandle_t directHandle = NULL;

    rtLog_SetOption(RT_USE_RTLOGGER);
    rc = rbus_open(&handle, "WiFi-CSIEvent-Consumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit4;
    }

    if (argc == 1)
    {
        index = 0;
        printf ("Trying to subscribe to actual DML (%s) that WIFI publishes..\n", pDMLName[index]);
    }
    else
    {
        index = 1;
        printf ("Trying to subscribe to CSI Sample Provider's DML (%s) which posts 56K of data in every 35ms interval\n", pDMLName[index]);
    }

    rbusEventSubscription_t subscription = {pDMLName[index], NULL, 100, 0, bigDataEventHandler, NULL, NULL, NULL, 0};

    rc = rbusEvent_SubscribeEx(handle, &subscription, 1, 0);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe failed: %d\n", rc);
        goto exit3;
    }

    while(1)
    {
        if ((0 == access("/tmp/rbus_csi_test", F_OK)) && (directHandle == NULL))
        {
            rc = rbus_openDirect(handle, &directHandle, pDMLName[index]);
            if(rc != RBUS_ERROR_SUCCESS)
            {
                printf("consumer: openDirect failed: %d\n", rc);
                goto exit3;
            }
        }
        else if ((0 != access("/tmp/rbus_csi_test", F_OK)) && (directHandle != NULL))
        {
            rc = rbus_closeDirect(directHandle);
            if(rc != RBUS_ERROR_SUCCESS)
            {
                printf("consumer: closeDirect failed: %d\n", rc);
                goto exit3;
            }
            directHandle = NULL;
        }

        sleep(1);
    }

exit3:
    rbus_close(handle);

exit4:
    printf("consumer: exit\n");
    return rc;
}


