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

#define TEST_VALUE_CHNAGE 1
#define PRINT_EVENT(EVENT, SUBSCRIPTION) \
    printf("\n-----------------------------------------------------------------\n" \
        " Event received in handler: %s\n" \
        " Subscription:\n" \
        "   eventName=%s\n" \
        "   userData=%s\n" \
        " Event:\n" \
        "   type=%d\n" \
        "   name=%s\n" \
        "   data=\n", \
            __FUNCTION__, \
            (SUBSCRIPTION)->eventName, \
            (char*)(SUBSCRIPTION)->userData, \
            (EVENT)->type, \
            (EVENT)->name); \
    rbusObject_fwrite((EVENT)->data, 8, stdout); \
    printf("\n=============================================================\n");

rbusHandle_t g_handle;

static void eventReceiveHandler1(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    (void)handle;
    printf("\n => ReceiveHandleronsumer: %s\n", __FUNCTION__);
    printf(" Consumer receiver Value event for param %s\n", event->name);
    PRINT_EVENT(event, subscription);
}

static void eventReceiveHandler2(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    (void)handle;
    printf("\n => ReceiveHandleronsumer: %s\n", __FUNCTION__);
    printf("Consumer receiver Value event for param %s \n", event->name);
    PRINT_EVENT(event, subscription);
}

static void eventReceiveHandler3(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    (void)handle;
    printf("\n => ReceiveHandleronsumer: %s\n", __FUNCTION__);
    printf("Consumer receiver Value event for param %s\n", event->name);
    PRINT_EVENT(event, subscription);
    if (event->type == 6)
    {
        printf("\nConsumer received duration complete event\n");
        printf("*******************************************\n");
    }
}

#if TEST_VALUE_CHNAGE
static void valueChangeHandler(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    printf("\n => ReceiveHandleronsumer: %s\n", __FUNCTION__);
    printf("Consumer receiver ValueChange event for param %s\n", event->name);
    PRINT_EVENT(event, subscription);

    (void)handle;
}
#endif

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    char* data[3] = { "My Data 1", "My Data2", "My Data 3" };
    rbusEventSubscription_t subscription[] = {
        {"Device.Provider1.Param1", NULL, 10, 0, eventReceiveHandler1, data[0], NULL, NULL, false},
        {"Device.Provider1.Param2", NULL, 14, 0, eventReceiveHandler2, data[1], NULL, NULL, false},
        {"Device.Provider1.Param2", NULL, 10, 60, eventReceiveHandler3, data[2], NULL, NULL, false}
    };

    rc = rbus_open(&g_handle, "EventIntervalConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }
#if TEST_VALUE_CHNAGE
    rc = rbusEvent_Subscribe(
            g_handle,
            "Device.Provider1.Param1",
            valueChangeHandler,
            "My User Data",
            0);
    sleep(5);
#endif
    rc = rbusEvent_SubscribeEx(g_handle, subscription, 3, 0);
    sleep(180);
    rbusEvent_UnsubscribeEx(g_handle, subscription, 2);

#if TEST_VALUE_CHNAGE
    rbusEvent_Unsubscribe(
        g_handle,
        "Device.Provider1.Param1");
#endif
    printf("Rbus Closed!");
    rbus_close(g_handle);
    return rc;
}


