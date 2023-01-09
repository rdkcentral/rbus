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

int loopFor = 24;

#define PRINT_EVENT(EVENT, SUBSCRIPTION) \
    printf("\n############################################################################\n" \
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
    printf("\n############################################################################\n");

static void generalEvent1Handler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t someText;

    PRINT_EVENT(event, subscription);

    someText = rbusObject_GetValue(event->data, "someText");

    printf("Consumer receiver General event 1 %s\n", event->name);

    if(someText)
        printf("  someText: %s\n", rbusValue_GetString(someText, NULL));

    printf("  My user data: %s\n", (char*)subscription->userData);

    (void)handle;
}

static void generalEvent2Handler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t label, text, counter, flag, subObject;
    label = text = counter = flag = subObject = NULL;

    PRINT_EVENT(event, subscription);

    label = rbusObject_GetValue(event->data, "label");

    subObject = rbusObject_GetValue(event->data, "subObject");

    if(subObject)
    {
        rbusObject_t obj = rbusValue_GetObject(subObject);

        text = rbusObject_GetValue(obj, "text");
        counter = rbusObject_GetValue(obj, "counter");
        flag = rbusObject_GetValue(obj, "flag");
    }

    printf("Consumer receiver General event 2 %s\n", event->name);

    if(label)
        printf("  label: %s\n", rbusValue_GetString(label, NULL));

    if(text)
        printf("  text: %s\n", rbusValue_GetString(text, NULL));

    if(counter)
        printf("  counter: %d\n", rbusValue_GetInt32(counter));

    if(flag)
        printf("  flag: %s\n", rbusValue_GetBoolean(flag) ? "true" : "false");

    printf("  My user data: %s\n", (char*)subscription->userData);

    (void)handle;
}

static void valueChangeHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");

    printf("Consumer receiver ValueChange event for param %s\n", event->name);

    if(newValue)
        printf("  New Value: %s\n", rbusValue_GetString(newValue, NULL));

    if(oldValue)
        printf("  Old Value: %s\n", rbusValue_GetString(oldValue, NULL));

    printf("  My user data: %s\n", (char*)subscription->userData);

    PRINT_EVENT(event, subscription);

    (void)handle;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    char* data[2] = { "My Data 1", "My Data2" };
    rbusEventSubscription_t subscriptions[3] = {
        {"Device.Provider1.Event1!", NULL, 0, 0, generalEvent1Handler, data[0], NULL, NULL, false},
        {"Device.Provider1.Event2!", NULL, 0, 0, generalEvent2Handler, data[1], NULL, NULL, false},
        {"Device.Provider1.Prop1", NULL, 0, 0, generalEvent1Handler, data[1], NULL, NULL, true}
    };

    printf("constumer: start\n");

    rc = rbus_open(&handle, "EventConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit4;
    }

#if TEST_VALUE_CHANGE

    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Param1",
        valueChangeHandler,
        NULL,
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe Param1 failed: %d\n", rc);
        goto exit3;
    }

    sleep(loopFor/4);
    
#endif

#if TEST_SUBSCRIBE
    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Event1!",
        generalEvent1Handler,
        data[0],
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit3;
    }

    rc = rbusEvent_Subscribe(
        handle,
        "Device.Provider1.Event2!",
        generalEvent2Handler,
        data[1],
        0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 2 failed: %d\n", rc);
        goto exit2;
    }

    sleep(loopFor/4);
    printf("Unsubscribing from Event2\n");
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event2!");
    sleep(loopFor/4);
    printf("Unsubscribing from Event1\n");
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event1!");
#endif

#if TEST_SUBSCRIBE_EX
    rc = rbusEvent_SubscribeEx(handle, subscriptions, 3, 0);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_Subscribe 1 failed: %d\n", rc);
        goto exit3;
    }

    sleep(loopFor/4);

    rbusEvent_UnsubscribeEx(handle, subscriptions, 3);
#endif
    goto exit3;

exit2:
#if TEST_SUBSCRIBE
    rbusEvent_Unsubscribe(handle, "Device.Provider1.Event1");
#endif

exit3:
    rbus_close(handle);

exit4:
    printf("consumer: exit\n");
    return rc;
}


