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

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;
    char buff[16];
    rbusValue_t value;

    snprintf(buff, sizeof(buff), "value %s", name);

    printf("Called get handler for [%s] val=[%s]\n", name, buff);

    rbusValue_Init(&value);
    rbusValue_SetString(value, buff);

    rbusProperty_SetValue(property, value);

    if(strcmp(name, "rbus_obj_block") == 0)
        sleep(30);
    if(strcmp(name, "Device.Blocking.Test0") == 0)
        while(1);

    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

#define dataElementsCount sizeof(dataElements)/sizeof(dataElements[0])
int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;

    char componentName[] = "rbusEvProvider";

    rbusDataElement_t dataElements[] = {
        {"rbus_event", RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, eventSubHandler, NULL}},
        {"rbus_obj_block", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"rbus_obj_nonblock", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Blocking.Test0", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.NonBlocking.Test1", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.NonBlocking.Test2", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.NonBlocking.Test3", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.NonBlocking.Test4", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.NonBlocking.Test5", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    //rbus_setLogLevel(RBUS_LOG_DEBUG);
    rbusHandle_ConfigGetTimeout(handle, 2000);

    rc = rbus_regDataElements(handle, dataElementsCount, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }
    else
        printf("provider: rbus_regDataElements success\n");
    int count=0;
    while(1)
    {
        rbusProperty_t prop = NULL;
        int numOfVals = 0;
        const char* input[] =  {"Device.SampleProvider."};
        rc = rbus_getExt(handle, 1, input, &numOfVals, &prop);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf ("rbus_get failed for %s with error [%d]\n", "Device.SampleProvider.", rc);
            break;
        }
        count++;
        sleep(1);
        rbusProperty_Release(prop);
    }
    pause(); 

    rbus_unregDataElements(handle, dataElementsCount, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
