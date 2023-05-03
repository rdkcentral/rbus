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

#define TotalParams   2
#define RBUS_BIGDATA_SIZE    (56 * 1024)

rbusHandle_t        rbusHandle;
char                componentName[] = "Sample-CSI-Event-Provider";
int loop = 1;

rbusError_t SampleProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);

rbusDataElement_t dataElements[TotalParams] = {
    {"Device.WiFi.X_RDK_CSI.1.sampleData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_SampleDataGetHandler, NULL, NULL, NULL, eventSubHandler, NULL}},
    {"Device.SampleProvider.BigData", RBUS_ELEMENT_TYPE_PROPERTY, {SampleProvider_SampleDataGetHandler, NULL, NULL, NULL, eventSubHandler, NULL}}
};

char m_bigdata[RBUS_BIGDATA_SIZE] = "ABCD";


rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;

    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if (action != RBUS_EVENT_ACTION_SUBSCRIBE)
        loop = 0;

    *autoPublish = false;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t SampleProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    char const* name;

    rbusValue_Init(&value);
    name = rbusProperty_GetName(property);

    printf("Called get handler for [%s]\n", name);
    rbusValue_SetString(value, m_bigdata);

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    rbusError_t rc;
    unsigned long long i = 0;

    (void)argc;
    (void)argv;
    printf("provider: start\n");

    m_bigdata[0] = '1';
    m_bigdata[1] = '.';
    for (i = 2; i < RBUS_BIGDATA_SIZE; i++)
        if (i%3 == 0)
            m_bigdata[i] = 'X';
        else if (i%3 == 1)
            m_bigdata[i] = 'Y';
        else if (i%3 == 2)
            m_bigdata[i] = 'Z';

    m_bigdata[RBUS_BIGDATA_SIZE-1] = '\0';

    rc = rbus_open(&rbusHandle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(rbusHandle, TotalParams, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements Successful:\n");
        goto exit1;
    }

    rbusEvent_t event = {0};
    rbusObject_t data;
    rbusValue_t value;


    rbusValue_Init(&value);
    rbusValue_SetString(value, m_bigdata);

    rbusObject_Init(&data, NULL);
    rbusObject_SetValue(data, "someText", value);

    event.name = dataElements[0].name;
    event.data = data;
    event.type = RBUS_EVENT_GENERAL;

    printf("publishing Event1\n");
    while(loop)
    {
        usleep(100000);
        rc = rbusEvent_Publish(rbusHandle, &event);
        rc = rbusEvent_Publish(rbusHandle, &event);
        rc = rbusEvent_Publish(rbusHandle, &event);
    }

    rbusValue_Release(value);
    rbusObject_Release(data);

    rbus_unregDataElements(rbusHandle, TotalParams, dataElements);

exit1:

    rbus_close(rbusHandle);

exit2:
    printf("provider: exit\n");
    return 0;
}
