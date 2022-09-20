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

#define TEST_VALUE_CHANGE   1
#define TEST_SUBSCRIBE      1
#define TEST_SUBSCRIBE_EX   1

int runtime = 20;

static void eventReceiveHandler1(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t value = rbusObject_GetValue(event->data, "value");

    printf("Consumer receiver 1st general event %s\n", event->name);

    if(value)
        printf("  Value: %s\n", rbusValue_GetString(value, NULL));

    printf("  My user data: %s\n", (char*)subscription->userData);
}

static void eventReceiveHandler2(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t valueLabel = rbusObject_GetValue(event->data, "label");
    rbusValue_t valueNumber = rbusObject_GetValue(event->data, "number");

    printf("Consumer receiver 2nd general event %s\n", event->name);

    if(valueLabel)
        printf("  Label: %s\n", rbusValue_GetString(valueLabel, NULL));

    if(valueNumber)
        printf("  Number: %d\n", rbusValue_GetInt32(valueNumber));

    printf("  My user data: %s\n", (char*)subscription->userData);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    char* data[2] = { "My Data 1", "My Data2" };

    // create two subscriptions
    rbusEventSubscription_t subscriptions[2];
    subscriptions[0].eventName = "Device.Provider1.Event1!";
    subscriptions[0].filter = NULL;
    subscriptions[0].duration = 0;
    subscriptions[0].handler  = eventReceiveHandler1;
    subscriptions[0].userData = data[0];

    subscriptions[1].eventName = "Device.Provider1.Event2!";
    subscriptions[1].filter = NULL;
    subscriptions[1].duration = 0;
    subscriptions[1].handler  = eventReceiveHandler2;
    subscriptions[1].userData = data[1];

    printf("constumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    rc = rbusEvent_SubscribeEx(handle, subscriptions, 2, 0);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit2;
    }

    sleep(runtime);

    rbusEvent_UnsubscribeEx(handle, subscriptions, 2);

exit2:
    rbus_close(handle);

exit1:
    printf("consumer: exit\n");
    return rc;
}
