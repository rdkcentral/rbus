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

#include <rbus.h>
#include <rtTime.h>
#include <rbus_config.h>
#include "../common/test_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "rtVector.h"
#include "rbus_handle.h"

static int gDuration = 10;

int gEventCounts[3] = {0}; /* shared with subscribeEx.c*/
int gEventTable[6] = {0}; /*used to track events from EventTable rows*/
static rtTime_t gAsyncStartTime;
static int gAsyncExpectedElapseMin;
static int gAsyncExpectedElapseMax;
static rbusError_t gAsyncExpectedError;
static rbusError_t gAsyncActualError;
static int gAsyncActualElapsed;
static int gAsyncSuccess;

bool testSubscribeHandleEvent( /*also shared with subscribeEx.c*/
    char* label,
    int eventIndex,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t valBuff;
    rbusValue_t valIndex;
    char expectedBuff[32];
    bool pass;

    PRINT_TEST_EVENT(label, event, subscription);

    snprintf(expectedBuff, 32, "event %d data %d", eventIndex, gEventCounts[eventIndex]);

    valBuff = rbusObject_GetValue(event->data, "buffer");
    if(!valBuff)
    {
        printf("%s FAIL: value 'buffer' NULL\n", label);
        return false;
    }

    valIndex = rbusObject_GetValue(event->data, "index");
    if(!valIndex)
    {
        printf("%s FAIL: value 'index' NULL\n", label);
        return false;
    }

    pass = (strcmp(rbusValue_GetString(valBuff, NULL), expectedBuff) == 0 && 
            rbusValue_GetInt32(valIndex) == gEventCounts[eventIndex]);

    gEventCounts[eventIndex]++;

    return pass;
}

static void handler1(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    testSubscribeHandleEvent("_test_Subscribe handle1", 0, event, subscription);
}

static void handler2(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    testSubscribeHandleEvent("_test_Subscribe handle2", 1, event, subscription);
}

static void handlerProviderNotFound1(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    PRINT_TEST_EVENT("handlerProviderNotFound1", event, subscription);
    gEventCounts[2]++;
}

static void eventsTableHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    (void)(subscription);
    PRINT_TEST_EVENT("eventsTableHandler", event, subscription);
    if(!strcmp(event->name, "Device.TestProvider.EventsTable.1.Event"))
    {
        gEventTable[0]++;
    }
    else if(!strcmp(event->name, "Device.TestProvider.EventsTable.2.Event"))
    {
        gEventTable[1]++;
    }
    else if(!strcmp(event->name, "Device.TestProvider.EventsTable.3.Event"))
    {
        gEventTable[2]++;
    }
    else if(!strcmp(event->name, "Device.TestProvider.EventsTable.1.Prop"))
    {
        gEventTable[3]++;
    }
    else if(!strcmp(event->name, "Device.TestProvider.EventsTable.2.Prop"))
    {
        gEventTable[4]++;
    }
    else if(!strcmp(event->name, "Device.TestProvider.EventsTable.3.Prop"))
    {
        gEventTable[5]++;
    }
}

static void handlerAsyncSub(
    rbusHandle_t handle, 
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
    gAsyncActualError = error;
    gAsyncActualElapsed = rtTime_Elapsed(&gAsyncStartTime, NULL);

    if(gAsyncActualError == gAsyncExpectedError &&
       gAsyncActualElapsed >= gAsyncExpectedElapseMin &&
       gAsyncActualElapsed <= gAsyncExpectedElapseMax)
    {
        gAsyncSuccess = 1;
    }
    else
    {
        gAsyncSuccess = 0;
    }

    printf("handlerAsyncSub event=%s\n", subscription->eventName);
    
    (void)(handle);
}

int getDurationSubscribe()
{
    return gDuration;
}

void printResult(bool ok, char const* func, char const* event, int timeout, int rc, int elapsed)
{
    printf("%s func=%s event=%s timeout=%d rc=%d elapsed=%d\n", ok ? "PASS":"FAIL", func, event, timeout, rc, elapsed);
}

void subscribe(
    rbusHandle_t handle, 
    char const* event, 
    rbusEventHandler_t handler, 
    int timeout, 
    int maxElapsed, 
    int rcExpected)
{
    rtTime_t tm;
    int elapsed;
    int success;
    int rc;
    static char userData[] = "My Data";

    rtTime_Now(&tm);
    rc = rbusEvent_Subscribe(handle, event, handler, userData, timeout);
    elapsed = rtTime_Elapsed(&tm, NULL);
    success = ((rc == rcExpected) && (elapsed < maxElapsed));
    TALLY(success);
    printf("%s rbusEvent_Subscribe %s timeout=%d expectedErr=%d expectedMaxElapsed=%d actualErr=%d actualElapsed=%d\n", 
        success ? "PASS" : "FAIL", 
        event,
        timeout,
        rcExpected,
        maxElapsed,
        rc,
        elapsed);
}

void unsubscribe(
    rbusHandle_t handle, 
    char const* event, 
    int rcExpected)
{
    int success;
    int rc;
    rc = rbusEvent_Unsubscribe(handle, event);
    success = (rc == rcExpected);
    TALLY(success);
    printf("%s rbusEvent_Unsubscribe %s rc=%d\n", 
        success ? "PASS" : "FAIL", 
        event,
        rc);
}

void subscribeAsync(
    rbusHandle_t handle, 
    char const* event, 
    rbusEventHandler_t handler, 
    int timeout, 
    int expectedErr, 
    int expectedAsyncErr,
    int expectedMinElapsed, 
    int expectedMaxElapsed)
{
    int actualErr;
    static char userData[] = "My Async Data";

    gAsyncExpectedElapseMin = expectedMinElapsed;
    gAsyncExpectedElapseMax = expectedMaxElapsed;
    gAsyncExpectedError = expectedAsyncErr;
    gAsyncSuccess = -1;
    rtTime_Now(&gAsyncStartTime);
    actualErr = rbusEvent_SubscribeAsync(handle, event, handler, handlerAsyncSub, userData, timeout);
    if(actualErr != expectedErr)
        gAsyncSuccess = 0;
    while(gAsyncSuccess == -1)
        usleep(100);
    printf("%s rbusEvent_SubscribeAsync %s timeout=%d\n"
           "\texpectedErr=%d expectedAsyncErr=%d expectedMinElapsed=%d expectedMaxElapsed=%d\n"
           "\tactualErr=%d actualAsyncError=%d actualElapsed=%d\n", 
        gAsyncSuccess ? "PASS" : "FAIL", 
        event,
        timeout,
        expectedErr,
        gAsyncExpectedError,
        gAsyncExpectedElapseMin,
        gAsyncExpectedElapseMax,
        actualErr,
        gAsyncActualError,
        gAsyncActualElapsed);
    TALLY(gAsyncSuccess);
}

void testSubscribe(rbusHandle_t handle, int* countPass, int* countFail)
{
    int rc = RBUS_ERROR_SUCCESS;
    int success;
    int maxTimeout = 30;
    int i;
    unsigned int quit_counter = 10;

    /*changing from default so tests don't take 10 minutes for async sub to complete*/
    rbusConfig_Get()->subscribeTimeout = maxTimeout * 1000;

    subscribe(handle, "Device.TestProvider.Event1!", handler1, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.Event2!", handler2, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.ErrorSubHandlerEvent!", handler2, 0, 500, RBUS_ERROR_ACCESS_NOT_ALLOWED);

    /* RDKB-38648: Changed the code to check for minimum number of events count to address the issue of test case number varying for each run */ 
    sleep(30);

    for(i = 0; i < 2; ++i)
    {
        TALLY(gEventCounts[i] >= 20);
        printf("%s Device.TestProvider.Event%d expectedMinEventCount=%d actualEventCount=%d\n", 
            gEventCounts[i] >= 20 ? "PASS" : "FAIL", i, 20, gEventCounts[i]);
    }
 
    unsubscribe(handle, "Device.TestProvider.Event2!", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.Event1!", RBUS_ERROR_SUCCESS);

    /*test negative cases*/

    /*per RDKB-33658 RBUS_ERROR_TIMEOUT must be returned for non-existing events*/
    subscribe(handle, "Device.TestProvider.NonExistingEvent1!", handler1, 0, 500, RBUS_ERROR_TIMEOUT);

    /*since event wasn't successfully subscribed error RBUS_ERROR_INVALID_OPERATION is expected */
    unsubscribe(handle, "Device.TestProvider.NonExistingEvent1!", RBUS_ERROR_INVALID_OPERATION);

    /* Test passing timeout value */

    /*timeout of 20 but it should not take 20 because the event exists now and should return immediately*/
    subscribe(handle, "Device.TestProvider.Event1!", handler1, 20, 500, RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.Event1!", RBUS_ERROR_SUCCESS);

    /*timeout of -1 (use default) which should return immediately*/
    subscribe(handle, "Device.TestProvider.Event1!", handler1, -1, 500, RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.Event1!", RBUS_ERROR_SUCCESS);

    /*test provider specific error can be received*/
    subscribe(handle, "Device.TestProvider.ErrorSubHandlerEvent!", handler1, -1, 500, RBUS_ERROR_ACCESS_NOT_ALLOWED);

    /* subscribe with timeout that should fail after the timeout reached */
    subscribe(handle, "Device.TestProvider.NonExistingEvent1!", handler1, 20, 21000, RBUS_ERROR_TIMEOUT);

    /* subscribe with default timeout that should fail after the default timeout reached */
    subscribe(handle, "Device.TestProvider.NonExistingEvent1!", handler1, -1, (maxTimeout+1)*1000, RBUS_ERROR_TIMEOUT);

    /* Test async subscribe with timeout succeeds and returns immediately */
    /* ARRISXB3-11307: Increased the max elapsed value to address random failure issue faced with ARRIS XB3 */
    subscribeAsync(handle, "Device.TestProvider.Event1!", handler1, 20, RBUS_ERROR_SUCCESS, RBUS_ERROR_SUCCESS, 0, 300);
    if(gAsyncSuccess == 1)
    {
        while((!rbusEvent_IsSubscriptionExist(handle, "Device.TestProvider.Event1!", NULL)) && (quit_counter--))
        {
            usleep(1000000);
        }
        unsubscribe(handle, "Device.TestProvider.Event1!", RBUS_ERROR_SUCCESS);
    }

    /* Test async subscribe with default timeout succeeds and returns immediately */
    /* ARRISXB3-11307: Increased the max elapsed value to address random failure issue faced with ARRIS XB3 */
    subscribeAsync(handle, "Device.TestProvider.Event1!", handler1, -1, RBUS_ERROR_SUCCESS, RBUS_ERROR_SUCCESS, 0, 300);
    if(gAsyncSuccess == 1)
    {
        while((!rbusEvent_IsSubscriptionExist(handle, "Device.TestProvider.Event1!", NULL)) && (quit_counter--))
        {
            usleep(1000000);
        }
        unsubscribe(handle, "Device.TestProvider.Event1!", RBUS_ERROR_SUCCESS);
    }

    /* Test async subscribe can get provider specific error */
    /* ARRISXB3-11307: Increased the max elapsed value to address random failure issue faced with ARRIS XB3 */
    subscribeAsync(handle, "Device.TestProvider.ErrorSubHandlerEvent!", handler1, -1, RBUS_ERROR_SUCCESS, RBUS_ERROR_ACCESS_NOT_ALLOWED, 0, 300);


    /* Test subscribe with default timeout fails after the default timeout reached */
    subscribeAsync(handle, "Device.TestProvider.NonExistingEvent1!", handler1, -1, RBUS_ERROR_SUCCESS, RBUS_ERROR_TIMEOUT, maxTimeout*1000, (maxTimeout+1)*1000);


    /* Test subscribe with timeout fails after the default timeout reached */
    subscribeAsync(handle, "Device.TestProvider.NonExistingEvent1!", handler1, maxTimeout, RBUS_ERROR_SUCCESS, RBUS_ERROR_TIMEOUT, maxTimeout*1000, (maxTimeout+1)*1000);


    /* Test async subscribe can succeed after provider comes up and receive 5 subsequent events callbacks*/
    gEventCounts[2] = 0;
    rbus_setInt(handle, "Device.TestProvider.TestProviderNotFound", 1);
    subscribeAsync(handle, "Device.TestProvider.ProviderNotFoundEvent1!", handlerProviderNotFound1, maxTimeout, RBUS_ERROR_SUCCESS, RBUS_ERROR_SUCCESS, 20000, 31000);
    sleep(5);
    /* ARRISXB3-11307: Changed the check from fixed value(5) check to greater than or equal to 4 as ARRIS XB3 receives only 4 events sometimes */
    TALLY(success = gEventCounts[2] >= 4);
    printf("%s rbusEvent_SubscribeAsync Device.TestProvider.ProviderNotFoundEvent1! expectNumEvents=%d actualNumEvent=%d\n", 
        success ? "PASS" : "FAIL", 4, gEventCounts[2]);
    unsubscribe(handle, "Device.TestProvider.ProviderNotFoundEvent1!", RBUS_ERROR_SUCCESS);

    /*test subscribing to events inside rows*/

    /* add three rows */
    rc = rbusTable_addRow(handle, "Device.TestProvider.EventsTable.", NULL, NULL);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_addRow Device.TestProvider.EventsTable.1.\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");
    rc = rbusTable_addRow(handle, "Device.TestProvider.EventsTable.", NULL, NULL);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_addRow Device.TestProvider.EventsTable.2.\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");
    rc = rbusTable_addRow(handle, "Device.TestProvider.EventsTable.", NULL, NULL);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_addRow Device.TestProvider.EventsTable.3.\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");

    /*subscribe to stuff on each row*/
    subscribe(handle, "Device.TestProvider.EventsTable.1.Event", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.EventsTable.2.Event", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.EventsTable.3.Event", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.EventsTable.1.Prop", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.EventsTable.2.Prop", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);
    subscribe(handle, "Device.TestProvider.EventsTable.3.Prop", eventsTableHandler, 0, 500, RBUS_ERROR_SUCCESS);

    sleep(10);

    for(i = 0; i < 3; ++i)
    {
        TALLY(gEventTable[i] >= 9);
        printf("%s Device.TestProvider.EventsTable.%d.Event expectedMinEventCount=%d actualEventCount=%d\n", 
            gEventTable[i] >= 9 ? "PASS" : "FAIL", i, 9, gEventTable[i]);
    }

    for(i = 3; i < 6; ++i)
    {
        TALLY(gEventTable[i] >= 3);
        printf("%s Device.TestProvider.EventsTable.%d.Data expectedMinEventCount=%d actualEventCount=%d\n", 
            gEventTable[i]  >= 3 ? "PASS" : "FAIL", i-3, 3, gEventTable[i]);
    }

    /*test subscribing to non-existing rows*/
    /*The path is registered but the row is missing, so the request will reach the provider and the provider will return RBUS_ERROR_INVALID_EVENT*/
    subscribe(handle, "Device.TestProvider.EventsTable.4.Event", eventsTableHandler, 0, 500, RBUS_ERROR_ELEMENT_DOES_NOT_EXIST);
    subscribe(handle, "Device.TestProvider.EventsTable.4.Prop", eventsTableHandler, 0, 500, RBUS_ERROR_ELEMENT_DOES_NOT_EXIST);

    /*The path is not registered so it will never reach the provider and we should get RBUS_ERROR_TIMEOUT*/
    subscribe(handle, "Device.TestProvider.EventsTable.1.NonExisting", eventsTableHandler, 0, 500, RBUS_ERROR_TIMEOUT);

    unsubscribe(handle, "Device.TestProvider.EventsTable.1.Event", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.EventsTable.2.Event", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.EventsTable.3.Event", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.EventsTable.1.Prop", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.EventsTable.2.Prop", RBUS_ERROR_SUCCESS);
    unsubscribe(handle, "Device.TestProvider.EventsTable.3.Prop", RBUS_ERROR_SUCCESS);

    /* remove three rows */
    rc = rbusTable_removeRow(handle, "Device.TestProvider.EventsTable.1");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_removeRow Device.TestProvider.EventsTable.1\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");
    rc = rbusTable_removeRow(handle, "Device.TestProvider.EventsTable.2");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_removeRow Device.TestProvider.EventsTable.2\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");
    rc = rbusTable_removeRow(handle, "Device.TestProvider.EventsTable.3");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s rbusTable_removeRow Device.TestProvider.EventsTable.3\n", rc == RBUS_ERROR_SUCCESS ? "PASS" : "FAIL");

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_Subscribe");
}
