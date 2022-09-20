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
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rbus_setLogLevel(RBUS_LOG_DEBUG);

    rc = rbus_regDataElements(handle, dataElementsCount, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }

    pause(); 

    rbus_unregDataElements(handle, dataElementsCount, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
