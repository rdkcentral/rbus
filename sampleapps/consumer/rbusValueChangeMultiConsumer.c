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

static void processHandler(
    char const* func, 
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
    rbusValue_t filter = rbusObject_GetValue(event->data, "filter");

    printf("%s: { event:%s ", func, event->name);
    if(newValue)
        printf(" new:%d ", rbusValue_GetInt32(newValue));
    if(oldValue)
        printf(" old:%d ", rbusValue_GetInt32(oldValue));
    if(filter)
        printf(" filter:%d ", rbusValue_GetBoolean(filter));
    if(subscription->userData)
        printf(" user:%s ", (char*)subscription->userData);
    printf("}\n");
}

static void handler1(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    processHandler(__FUNCTION__, handle, event, subscription);
}
static void handler2(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    processHandler(__FUNCTION__, handle, event, subscription);
}
static void handler3(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    processHandler(__FUNCTION__, handle, event, subscription);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle1, handle2, handle3;
    rbusFilter_t filter1, filter2, filter3;
    rbusValue_t value1, value2, value3;

    //rbus_setLogLevel(RBUS_LOG_DEBUG);

    rbusValue_Init(&value1);
    rbusValue_SetInt32(value1, 1);
    rbusFilter_InitRelation(&filter1, RBUS_FILTER_OPERATOR_GREATER_THAN, value1);

    rbusValue_Init(&value2);
    rbusValue_SetInt32(value2, 3);
    rbusFilter_InitRelation(&filter2, RBUS_FILTER_OPERATOR_GREATER_THAN, value2);

    rbusValue_Init(&value3);
    rbusValue_SetInt32(value3, 5);
    rbusFilter_InitRelation(&filter3, RBUS_FILTER_OPERATOR_GREATER_THAN, value3);


    rbusEventSubscription_t subs1[] = {
        {"Device.Provider1.Param1", filter1, 0, 0, handler1, "handle1_Param1", NULL, NULL},
        {"Device.X_RDKCENTRAL-COM_XDNS.LuckyNumber", filter1, 0, 0, handler1, "handle1_LuckyNumber", NULL, NULL}
    };
    rbusEventSubscription_t subs2[] = {
        {"Device.Provider1.Param1", filter2, 0, 0, handler2, "handle2_Param1", NULL, NULL},
         {"Device.X_RDKCENTRAL-COM_XDNS.LuckyNumber", filter2, 0, 0, handler2, "handle2_LuckyNumber", NULL, NULL}
    };
    rbusEventSubscription_t subs3[] = {
        {"Device.Provider1.Param1", filter3, 0, 0, handler3, "handle3_Param1", NULL, NULL},
        {"Device.X_RDKCENTRAL-COM_XDNS.LuckyNumber", filter3, 0, 0, handler3, "handle3_LuckyNumber", NULL, NULL}
    };


    if((rc = rbus_open(&handle1, "EventConsumer1")) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }

    if((rc = rbus_open(&handle2, "EventConsumer2")) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }

    if((rc = rbus_open(&handle3, "EventConsumer3")) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        return -1;
    }

    if((rc = rbusEvent_SubscribeEx(handle1, subs1, 2, 0)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_SubscribeEx failed: %d\n", rc);
        return -1;
    }

    if((rc = rbusEvent_SubscribeEx(handle2, subs2, 2, 0)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_SubscribeEx failed: %d\n", rc);
        return -1;
    }

    if((rc = rbusEvent_SubscribeEx(handle3, subs3, 2, 0)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_SubscribeEx failed: %d\n", rc);
        return -1;
    }

    printf("##############\n#\n# Event for 1, 2, and 3 \n#\n##############\n");

    sleep(15);

    if((rc = rbusEvent_UnsubscribeEx(handle1, subs1, 2)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_UnsubscribeEx failed: %d\n", rc);
        return -1;
    }    

    printf("##############\n#\n# Event for 2 and 3 \n#\n##############\n");

    sleep(15);

    if((rc = rbusEvent_UnsubscribeEx(handle2, subs2, 2)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_UnsubscribeEx failed: %d\n", rc);
        return -1;
    }   

    printf("##############\n#\n# Event for 3 \n#\n##############\n");

    sleep(15);

    if((rc = rbusEvent_UnsubscribeEx(handle3, subs3, 2)) != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbusEvent_UnsubscribeEx failed: %d\n", rc);
        return -1;
    }   

    printf("##############\n#\n# Event for 0 \n#\n##############\n");

    sleep(15);

    rbus_close(handle1);
    rbus_close(handle2);
    rbus_close(handle3);
    return rc;
}


