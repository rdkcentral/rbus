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

int runtime = 60;

char componentName[] = "RbusOpenRegRbusOpenConsumer";

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
    rbusValue_t byValue = rbusObject_GetValue(event->data, "by");
    rbusValue_t filter = rbusObject_GetValue(event->data, "filter");

    printf("Consumer receiver ValueChange event for param %s\n", event->name);

    if(newValue)
        printf("   New Value: %d\n", rbusValue_GetInt32(newValue));

    if(oldValue)
        printf("   Old Value: %d\n", rbusValue_GetInt32(oldValue));

    if(byValue)
        printf("   By component: %s\n", rbusValue_GetString(byValue, NULL));

    if(filter)
        printf("   Filter: %d\n", rbusValue_GetBoolean(filter));

    if(subscription->userData)
        printf("   User data: %s\n", (char*)subscription->userData);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    rbusFilter_t filter;
    rbusValue_t filterValue;
    rbusEventSubscription_t subscription = {"Device.Provider1.Param1", NULL, 0, 0, eventReceiveHandler, NULL, NULL, NULL, false};

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }

    printf("Subscribing to Device.Provider1.Param1\n");
    /* subscribe to all value change events on property "Device.Provider1.Param1" */
    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Param1",
        eventReceiveHandler,
        "My User Data",
        0);

    sleep(5);
    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }


    sleep(15);

    printf("Unsubscribing Device.Provider1.Param1\n");

    rbusEvent_Unsubscribe(
        handle,
        "Device.Provider1.Param1");

    /* subscribe using filter to value change events on property "Device.Provider1.Param1"
       setting filter to: value >= 5.
     */

    rbusValue_Init(&filterValue);
    rbusValue_SetInt32(filterValue, 5);

    rbusFilter_InitRelation(&filter, RBUS_FILTER_OPERATOR_GREATER_THAN, filterValue);

    subscription.filter = filter;

    printf("Subscribing to Device.Provider1.Param1 with filter > 5\n");

    rc = rbusEvent_SubscribeEx(handle, &subscription, 1, 0);

    rbusValue_Release(filterValue);
    rbusFilter_Release(filter);

    sleep(25);

    return rc;
}


