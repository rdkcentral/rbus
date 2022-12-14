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
#include "../common/test_macros.h"

static int gDuration = 5;

extern int gEventCounts[3]; /*from subscribe.c*/

bool testSubscribeHandleEvent(
    char* label,
    int eventIndex,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription) /*from subscribe.c*/;

int getDurationSubscribeEx()
{
    return gDuration;
}

static void handler1(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    testSubscribeHandleEvent("_test_SubscribeEx handle1", 0, event, subscription);
}

static void handler2(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    testSubscribeHandleEvent("_test_SubscribeEx handle2", 1, event, subscription);
}

void testSubscribeEx(rbusHandle_t handle, int* countPass, int* countFail)
{
    int rc = RBUS_ERROR_SUCCESS;
    int i = 0;
    char* data[2] = { "My Data 1", "My Data2" };

    rbusEventSubscription_t subscriptions[2] = {
        {"Device.TestProvider.Event1!", NULL, 0, 0, handler1, data[0], NULL, NULL, false},
        {"Device.TestProvider.Event2!", NULL, 0, 0, handler2, data[1], NULL, NULL, false}
    };

    rc = rbusEvent_SubscribeEx(handle, subscriptions, 2, 0);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_Subscribe rbusEvent_SubscribeEx %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit0;

    sleep(gDuration);

    /* RDKB-38648: Changed the code to check for minimum number of events count to address the issue of test case number varying for each run */
    for(i = 0; i < 2; ++i)
    {
        TALLY(gEventCounts[i] >= 5);
        printf("%s Device.TestProvider.Event%d expectedMinEventCount=%d actualEventCount=%d\n",
                gEventCounts[i] >= 5 ? "PASS" : "FAIL", i, 5, gEventCounts[i]);
    }
    rc = rbusEvent_UnsubscribeEx(handle, subscriptions, 2);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("_test_SubscribeEx rbusEvent_UnsubscribeEx %s rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

exit0:
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_SubscribeEx");
}
