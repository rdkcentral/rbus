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

int loopFor = 30;
int subscribed1 = 0;
int subscribed2 = 0;

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;

    *autoPublish = true;
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
    else if(!strcmp("Device.Provider1.Prop1", eventName))
    {
        subscribed2 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    if(strcmp(name, "Device.Provider1.Param1") == 0)
    {
        static int mydata = 0;
        char buff[16];
        rbusValue_t value;

        snprintf(buff, sizeof(buff), "v%d", mydata++/3);/*fake a value change every 3rd call to this function*/

        printf("Called get handler for [%s] val=[%s]\n", name, buff);

        rbusValue_Init(&value);
        rbusValue_SetString(value, buff);

        rbusProperty_SetValue(property, value);

        rbusValue_Release(value);
    }
    else if(strcmp(name, "Device.SampleProvider.SampleData2.StrData") == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, "String Data from Event Provider");

        rbusProperty_SetValue(property, value);

        rbusValue_Release(value);
    }
    else if(strcmp(name, "Device.Provider1.Prop1") == 0)
    {
        char buff[24];
        rbusValue_t value;
        snprintf(buff, sizeof(buff), "TestInitialValue");
        rbusValue_Init(&value);
        rbusValue_SetString(value, buff);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
    }
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;

    char componentName[] = "EventProvider";
    char* eventData[2] = { "Hello Earth", "Hello Mars" };

    rbusDataElement_t dataElements[5] = {
        {"Device.Provider1.Event1!", RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
        {"Device.Provider1.Event2!", RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
        {"Device.Provider1.Prop1", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, eventSubHandler, NULL}},
        {"Device.Provider1.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.SampleProvider.SampleData2.StrData", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}}
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(handle, 4, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }

    while (loopFor != 0)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
        loopFor--;

        if(subscribed1)
        {
            rbusEvent_t event = {0};
            rbusObject_t data;
            rbusValue_t value;

            printf("publishing Event1\n");

            rbusValue_Init(&value);
            rbusValue_SetString(value, eventData[0]);

            rbusObject_Init(&data, NULL);
            rbusObject_SetValue(data, "someText", value);

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
            rbusEvent_t event = {0};
            rbusObject_t data;
            rbusObject_t subobj;

            rbusValue_t value;

            printf("publishing Event2\n");

            rbusObject_Init(&data, NULL);
            rbusObject_Init(&subobj, "This is a sub object");

            rbusValue_Init(&value);
            rbusValue_SetString(value, eventData[1]);
            rbusObject_SetValue(data, "label", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetString(value, "Some Text");
            rbusObject_SetValue(subobj, "text", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetInt32(value, loopFor);
            rbusObject_SetValue(subobj, "counter", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetBoolean(value, ((loopFor % 3) == 0 ? true:false));
            rbusObject_SetValue(subobj, "flag", value);
            rbusValue_Release(value);

            rbusValue_Init(&value);
            rbusValue_SetObject(value, subobj);
            rbusObject_Release(subobj);
            rbusObject_SetValue(data, "subObject", value);
            rbusValue_Release(value);

            event.name = dataElements[1].name;
            event.data = data;
            event.type = RBUS_EVENT_GENERAL;

            rc = rbusEvent_Publish(handle, &event);

            rbusObject_Release(data);

            if(rc != RBUS_ERROR_SUCCESS)
                printf("provider: rbusEvent_Publish Event2 failed: %d\n", rc);
        }
    }

    rbus_unregDataElements(handle, 4, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
