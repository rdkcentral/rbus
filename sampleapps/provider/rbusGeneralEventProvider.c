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

//TODO handle filter matching

int runtime = 30;
int subscribed1 = 0;
int subscribed2 = 0;

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;
    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp("Device.Provider1.Event1!", eventName))
    {
        subscribed1 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp("Device.Provider1.Event2!", eventName))
    {
        subscribed2 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    char buffer[128];
    int rc = RBUS_ERROR_SUCCESS;

    char componentName[] = "EventProvider";
    char* eventData[2] = { "Hello Earth", "Hello Mars" };

    rbusDataElement_t dataElements[2] = {
        {"Device.Provider1.Event1!", RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
        {"Device.Provider1.Event2!", RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}}
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(handle, 2, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbusEventProvider_Register failed: %d\n", rc);
        goto exit1;
    }

    while (runtime != 0)
    {
        printf("provider: exiting in %d seconds\n", runtime);
        sleep(1);
        runtime--;

        if(subscribed1)
        {
            printf("publishing Event1\n");

            snprintf(buffer, 128, "%s %d", eventData[0], runtime);

            rbusValue_t value;
            rbusObject_t data;

            rbusValue_Init(&value);
            rbusValue_SetString(value, buffer);

            rbusObject_Init(&data, NULL);
            rbusObject_SetValue(data, "value", value);

            rbusEvent_t event = {0};
            event.name = dataElements[0].name;
            event.data = data;
            event.type = RBUS_EVENT_GENERAL;

            rc = rbusEvent_Publish(handle, &event);

            rbusValue_Release(value);
            rbusObject_Release(data);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event1 failed: %d\n", rc);
        }
        if(subscribed2)
        {
            printf("publishing Event2\n");

            rbusValue_t valueLabel;
            rbusValue_t valueNumber;
            rbusObject_t data;

            snprintf(buffer, 128, "%s %d", eventData[1], runtime);

            rbusValue_Init(&valueLabel);
            rbusValue_Init(&valueNumber);

            rbusValue_SetString(valueLabel, buffer);
            rbusValue_SetInt32(valueNumber, runtime);

            rbusObject_Init(&data, NULL);
            rbusObject_SetValue(data, "label", valueLabel);
            rbusObject_SetValue(data, "number", valueNumber);

            rbusEvent_t event = {0};
            event.name = dataElements[1].name;
            event.data = data;
            event.type = RBUS_EVENT_GENERAL;

            rc = rbusEvent_Publish(handle, &event);

            rbusValue_Release(valueLabel);
            rbusValue_Release(valueNumber);
            rbusObject_Release(data);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event2 failed: %d\n", rc);
        }
    }

    rbus_unregDataElements(handle, 2, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
