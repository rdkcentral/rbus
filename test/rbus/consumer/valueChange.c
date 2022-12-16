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
#include "rbus_config.h"

static int gDuration = 67;

typedef struct IntResult
{
    int32_t newValExp;
    int32_t oldValExp;
    int32_t filterExp;
    char byExp[64];
    int32_t newValAct;
    int32_t oldValAct;
    int32_t filterAct;
    char byAct[64];
    int status;
} IntResult;

typedef struct StrResult
{
    char newValExp[64];
    char oldValExp[64];
    int32_t filterExp;
    char byExp[64];
    char newValAct[64];
    char oldValAct[64];
    int32_t filterAct;
    char byAct[64];
    int status;
} StrResult;

typedef struct Counter 
{
    int expected;
    int actual;
} Counter;

static StrResult simpleResults [3] = {
    { "value 1", "value 0", -1, "TestProvider", "", "", 0, "", -1 },  { "value 2", "value 1", -1, "TestProvider", "", "", 0, "", -1 }, { "value 3", "value 2", -1, "TestProvider", "", "", 0, "", -1 }
};

static IntResult intResults[6][5] = {
    {{ 4, 3, 1, "TestProvider", 0, 0, 0, "", -1 }, { 3, 4, 0, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }},
    {{ 3, 2, 1, "TestProvider", 0, 0, 0, "", -1 }, { 2, 3, 0, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }},
    {{ 3, 2, 0, "TestProvider", 0, 0, 0, "", -1 }, { 2, 3, 1, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }},
    {{ 4, 3, 0, "TestProvider", 0, 0, 0, "", -1 }, { 3, 4, 1, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1 }},
    {{ 3, 2, 1, "TestProvider", 0, 0, 0, "", -1 }, { 4, 3, 0, "TestProvider", 0, 0, 0, "", -1 }, { 3, 4, 1, "TestProvider", 0, 0, 0, "", -1 }, { 2, 3, 0, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1}},
    {{ 3, 2, 0, "TestProvider", 0, 0, 0, "", -1 }, { 4, 3, 1, "TestProvider", 0, 0, 0, "", -1 }, { 3, 4, 0, "TestProvider", 0, 0, 0, "", -1 }, { 2, 3, 1, "TestProvider", 0, 0, 0, "", -1 }, {0, 0, 0, "", 0, 0, 0, "", -1}}
};
    
static StrResult strResults[6][5] = {
    {{ "eeee", "dddd", 1, "TestProvider", "", "", 0, "", -1 }, { "dddd", "eeee", 0, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}},
    {{ "dddd", "cccc", 1, "TestProvider", "", "", 0, "", -1 }, { "cccc", "dddd", 0, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}},
    {{ "dddd", "cccc", 0, "TestProvider", "", "", 0, "", -1 }, { "cccc", "dddd", 1, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}},
    {{ "eeee", "dddd", 0, "TestProvider", "", "", 0, "", -1 }, { "dddd", "eeee", 1, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}, {"", "", 0, "", "", "", 0, "", -1}},
    {{ "dddd", "cccc", 1, "TestProvider", "", "", 0, "", -1 }, { "eeee", "dddd", 0, "TestProvider", "", "", 0, "", -1 }, { "dddd", "eeee", 1, "TestProvider", "", "", 0, "", -1 }, { "cccc", "dddd", 0, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}},
    {{ "dddd", "cccc", 0, "TestProvider", "", "", 0, "", -1 }, { "eeee", "dddd", 1, "TestProvider", "", "", 0, "", -1 }, { "dddd", "eeee", 0, "TestProvider", "", "", 0, "", -1 }, { "cccc", "dddd", 1, "TestProvider", "", "", 0, "", -1 }, {"", "", 0, "", "", "", 0, "", -1}}
};

static IntResult byResults[3] = {
    { 1000, 0, -1, "TestConsumer", 0, 0, 0, "", -1}, { 2000, 1000, -1, "TestConsumer", 0, 0, 0, "", -1 }, { 3000, 2000, -1, "TestConsumer", 0, 0, 0, "", -1 }
};

static IntResult noAutoPubResults[2][2] = {
    {{ 5, 4, 0, "TestProvider", 0, 0, 0, "", -1 }, {4, 5, 1, "TestProvider", 0, 0, 0, "", -1 }}, 
    {{ 6, 5, 1, "TestProvider", 0, 0, 0, "", -1 }, {5, 6, 0, "TestProvider", 0, 0, 0, "", -1 }}
};

Counter simpleCounter = {3,0};
Counter intCounter[6] = {{2,0}, {2,0}, {2,0}, {2,0}, {4,0}, {4,0}};
Counter strCounter[6] = {{2,0}, {2,0}, {2,0}, {2,0}, {4,0}, {4,0}};
Counter byCounter = {3,0};
Counter noAutoPubCounter[2] = {{2,0},{2,0}};

void rbusValueChange_SetPollingPeriod(int seconds);

int getDurationValueChange()
{
    return gDuration;
}

static void simpleVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int count = simpleCounter.actual;    
    rbusValue_t byVal;
    const char* byComponent = NULL;

    PRINT_TEST_EVENT("test_ValueChange_intVCHandler", event, subscription);

    if(++simpleCounter.actual > simpleCounter.expected)
    {
        printf("test_ValueChange_simpleVCHandler Actual events exceeds expected\n");
        return;
    }

    byVal = rbusObject_GetValue(event->data, "by");
    if(byVal)
        byComponent = rbusValue_GetString(byVal, NULL);

    simpleResults[count].status = 1;
    strncpy(simpleResults[count].newValAct, rbusObject_GetValue(event->data, "value") ? rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL) : "null", 64);
    strncpy(simpleResults[count].oldValAct, rbusObject_GetValue(event->data, "value") ? rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL) : "null", 64);
    simpleResults[count].filterAct = rbusObject_GetValue(event->data, "filter") ? rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")) : -1;
    if(byComponent)
        strncpy(simpleResults[count].byAct, byComponent, 64);
}

static void intVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int index = atoi(&event->name[strlen(event->name)-1]);
    int count = intCounter[index].actual;    
    rbusValue_t byVal;
    const char* byComponent = NULL;

    PRINT_TEST_EVENT("test_ValueChange_intVCHandler", event, subscription);

    if(++intCounter[index].actual > intCounter[index].expected)
    {
        printf("test_ValueChange_intVCHandler %d Actual events exceeds expected\n", index);
        return;
    }

    byVal = rbusObject_GetValue(event->data, "by");
    if(byVal)
        byComponent = rbusValue_GetString(byVal, NULL);

    intResults[index][count].status = 1;
    intResults[index][count].newValAct = rbusObject_GetValue(event->data, "value") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "value")) : -1;
    intResults[index][count].oldValAct = rbusObject_GetValue(event->data, "oldValue") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "oldValue")) : -1;
    intResults[index][count].filterAct = rbusObject_GetValue(event->data, "filter") ? rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")) : -1;
    if(byComponent)
        strncpy(intResults[index][count].byAct, byComponent, 64);
}

static void stringVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int index = atoi(&event->name[strlen(event->name)-1]);
    int count = strCounter[index].actual;    
    rbusValue_t byVal;
    const char* byComponent = NULL;

    PRINT_TEST_EVENT("test_ValueChange_stringVCHandler", event, subscription);

    if(++strCounter[index].actual > strCounter[index].expected)
    {
        printf("test_ValueChange_strVCHandler %d Actual events exceeds expected\n", index);
        return;
    }

    byVal = rbusObject_GetValue(event->data, "by");
    if(byVal)
        byComponent = rbusValue_GetString(byVal, NULL);

    strResults[index][count].status = 1;
    strncpy(strResults[index][count].newValAct, rbusObject_GetValue(event->data, "value") ? rbusValue_GetString(rbusObject_GetValue(event->data, "value"), NULL) : "null", 64);
    strncpy(strResults[index][count].oldValAct, rbusObject_GetValue(event->data, "value") ? rbusValue_GetString(rbusObject_GetValue(event->data, "oldValue"), NULL) : "null", 64);
    strResults[index][count].filterAct = rbusObject_GetValue(event->data, "filter") ? rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")) : -1;
    if(byComponent)
        strncpy(strResults[index][count].byAct, byComponent, 64);

}

static void byVCHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)(handle);
    int count = byCounter.actual;   
    rbusValue_t byVal;
    const char* byComponent = NULL;

    PRINT_TEST_EVENT("test_ValueChange_byVCHandler", event, subscription);

    if(++byCounter.actual > byCounter.expected)
    {
        printf("test_ValueChange_byVCHandler Actual events exceeds expected\n");
        return;
    }

    byVal = rbusObject_GetValue(event->data, "by");
    if(byVal)
        byComponent = rbusValue_GetString(byVal, NULL);

    byResults[count].status = 1;
    byResults[count].newValAct = rbusObject_GetValue(event->data, "value") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "value")) : -1;
    byResults[count].oldValAct = rbusObject_GetValue(event->data, "oldValue") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "oldValue")) : -1;
    byResults[count].filterAct = rbusObject_GetValue(event->data, "filter") ? rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")) : -1;
    if(byComponent)
        strncpy(byResults[count].byAct, byComponent, 64);
}

static void noAutoPubHandler(rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription,
    int idx)
{
    (void)handle;
    (void)subscription;
    int count = noAutoPubCounter[idx].actual;    
    rbusValue_t byVal;
    const char* byComponent = NULL;

    if(++noAutoPubCounter[idx].actual > noAutoPubCounter[idx].expected)
    {
        printf("test_ValueChange_noAutoPubHandlerHandler Actual events exceeds expected\n");
        return;
    }

    byVal = rbusObject_GetValue(event->data, "by");
    if(byVal)
        byComponent = rbusValue_GetString(byVal, NULL);

    noAutoPubResults[idx][count].status = 1;
    noAutoPubResults[idx][count].newValAct = rbusObject_GetValue(event->data, "value") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "value")) : -1;
    noAutoPubResults[idx][count].oldValAct = rbusObject_GetValue(event->data, "oldValue") ? rbusValue_GetInt32(rbusObject_GetValue(event->data, "oldValue")) : -1;
    noAutoPubResults[idx][count].filterAct = rbusObject_GetValue(event->data, "filter") ? rbusValue_GetBoolean(rbusObject_GetValue(event->data, "filter")) : -1;
    if(byComponent)
        strncpy(noAutoPubResults[idx][count].byAct, byComponent, 64);
}
static void noAutoPub1Handler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    PRINT_TEST_EVENT("test_ValueChange_noAutoPub1Handler", event, subscription);
    noAutoPubHandler(handle, event, subscription, 0);
}

static void noAutoPub2Handler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    PRINT_TEST_EVENT("test_ValueChange_noAutoPub2Handler", event, subscription);
    noAutoPubHandler(handle, event, subscription, 1);
}

void testSimpleValueChange(rbusHandle_t handle)
{
    int rc;
    int i;
    int maxWait;

    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.VCParam", simpleVCHandler, NULL, 0);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_ValueChange rbusEvent_Subscribe rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

    maxWait = rc == RBUS_ERROR_SUCCESS ? 15 : 0;
    while(simpleCounter.actual < simpleCounter.expected && maxWait > 0)
    {
        sleep(1);
        maxWait--;
    }

    for(i = 0; i < simpleCounter.expected; ++i)
    {
        int pass = 
            !strcmp(simpleResults[i].newValAct, simpleResults[i].newValExp) &&
            !strcmp(simpleResults[i].oldValAct, simpleResults[i].oldValExp) &&
            simpleResults[i].filterAct == simpleResults[i].filterExp &&
            !strcmp(simpleResults[i].byAct, simpleResults[i].byExp);
        TALLY(pass);
        printf("%s _test_ValueChange Device.TestProvider.VCParam %d expect=[value:%s oldValue:%s filter:%d by:%s] actual=[value:%s oldValue:%s filter:%d by:%s]\n", 
                pass ? "PASS" : "FAIL",
                i,
                simpleResults[i].newValExp, 
                simpleResults[i].oldValExp, 
                simpleResults[i].filterExp,
                simpleResults[i].byExp,
                simpleResults[i].newValAct, 
                simpleResults[i].oldValAct, 
                simpleResults[i].filterAct,
                simpleResults[i].byAct);
    }

    rc = rbusEvent_Unsubscribe(handle, "Device.TestProvider.VCParam");
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_ValueChange rbusEvent_Unsubscribe rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);    
}

void testTypesValueChange(rbusHandle_t handle)
{
    int rc;
    int i;
    rbusValue_t intVal, strVal;
    rbusFilter_t filter[12];
    int maxWait;
    int needMore;

    /*
     * test subscribeEx with all filter types
     * currently testing only ints and strings
     */
    rbusValue_Init(&intVal);
    rbusValue_SetInt32(intVal, 3);

    rbusValue_Init(&strVal);
    rbusValue_SetString(strVal, "dddd");

    rbusFilter_InitRelation(&filter[0], RBUS_FILTER_OPERATOR_GREATER_THAN, intVal);
    rbusFilter_InitRelation(&filter[1], RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[2], RBUS_FILTER_OPERATOR_LESS_THAN, intVal);
    rbusFilter_InitRelation(&filter[3], RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[4], RBUS_FILTER_OPERATOR_EQUAL, intVal);
    rbusFilter_InitRelation(&filter[5], RBUS_FILTER_OPERATOR_NOT_EQUAL, intVal);

    rbusFilter_InitRelation(&filter[6], RBUS_FILTER_OPERATOR_GREATER_THAN, strVal);
    rbusFilter_InitRelation(&filter[7], RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[8], RBUS_FILTER_OPERATOR_LESS_THAN, strVal);
    rbusFilter_InitRelation(&filter[9], RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[10], RBUS_FILTER_OPERATOR_EQUAL, strVal);
    rbusFilter_InitRelation(&filter[11], RBUS_FILTER_OPERATOR_NOT_EQUAL, strVal);

    rbusEventSubscription_t subscription[12] = {
        {"Device.TestProvider.VCParamInt0", filter[0], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamInt1", filter[1], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamInt2", filter[2], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamInt3", filter[3], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamInt4", filter[4], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamInt5", filter[5], 0, 0, intVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr0", filter[6], 0, 0, stringVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr1", filter[7], 0, 0, stringVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr2", filter[8], 0, 0, stringVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr3", filter[9], 0, 0, stringVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr4", filter[10], 0, 0, stringVCHandler, NULL, NULL, NULL, false},
        {"Device.TestProvider.VCParamStr5", filter[11], 0, 0, stringVCHandler, NULL, NULL, NULL, false}
    };

    rc = rbusEvent_SubscribeEx(handle, subscription, 12, 0);

    rbusValue_Release(intVal);
    rbusValue_Release(strVal);
    for(i = 0; i < 12; ++i)
        rbusFilter_Release(filter[i]);

    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_ValueChange rbusEvent_SubscribeEx rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);


    maxWait = rc == RBUS_ERROR_SUCCESS ? 48 : 0;
    needMore = 1;
    while(needMore && maxWait > 0)
    {
        sleep(1);
        maxWait--;
        needMore = 0;
        for(i = 0; i < 6; ++i)
        {
            /* ARRISXB3-11307 : Added string check also to address failure with ARRISXB3*/
            if((intCounter[i].actual < intCounter[i].expected) || (strCounter[i].actual <  strCounter[i].expected))
                needMore = 1;
        }
    }

    for(i = 0; i < 6; ++i)
    {
        int j;
        int pass;
        
        pass  = intCounter[i].actual >= intCounter[i].expected;
        TALLY(pass);
        printf("%s _test_ValueChange Device.TestProvider.VCParamInt%d expected count=%d actual count=%d\n", pass ? "PASS" : "FAIL", i, intCounter[i].expected, intCounter[i].actual);
        for(j = 0; j < intCounter[i].expected; ++j)
        {
            int pass = 
                intResults[i][j].newValAct == intResults[i][j].newValExp &&
                intResults[i][j].oldValAct == intResults[i][j].oldValExp &&
                intResults[i][j].filterAct == intResults[i][j].filterExp &&
                !strcmp(intResults[i][j].byAct, intResults[i][j].byExp);
            TALLY(pass);
            printf("%s _test_ValueChange Device.TestProvider.VCParamInt%d expect=[value:%d oldValue:%d filter:%d by:%s] actual=[value:%d oldValue:%d filter:%d by:%s]\n", 
                    pass ? "PASS" : "FAIL", 
                    i,
                    intResults[i][j].newValExp, 
                    intResults[i][j].oldValExp, 
                    intResults[i][j].filterExp,
                    intResults[i][j].byExp,
                    intResults[i][j].newValAct, 
                    intResults[i][j].oldValAct, 
                    intResults[i][j].filterAct,
                    intResults[i][j].byAct);
        }
    }

    for(i = 0; i < 6; ++i)
    {
        int j;
        int pass;
        
        pass  = strCounter[i].actual >= strCounter[i].expected;
        TALLY(pass);
        printf("%s _test_ValueChange Device.TestProvider.VCParamStr%d expected count=%d actual count=%d\n", pass ? "PASS" : "FAIL", i, intCounter[i].expected, intCounter[i].actual);
        for(j = 0; j < strCounter[i].expected; ++j)
        {
            int pass = 
                !strcmp(strResults[i][j].newValAct, strResults[i][j].newValExp) &&
                !strcmp(strResults[i][j].oldValAct, strResults[i][j].oldValExp) &&
                strResults[i][j].filterAct == strResults[i][j].filterExp &&
                !strcmp(strResults[i][j].byAct, strResults[i][j].byExp);
            TALLY(pass);
            printf("%s _test_ValueChange Device.TestProvider.VCParamStr%d expect=[value:%s oldValue:%s filter:%d by:%s] actual=[value:%s oldValue:%s filter:%d by:%s]\n", 
                    pass ? "PASS" : "FAIL",
                    i,
                    strResults[i][j].newValExp, 
                    strResults[i][j].oldValExp, 
                    strResults[i][j].filterExp,
                    strResults[i][j].byExp,
                    strResults[i][j].newValAct, 
                    strResults[i][j].oldValAct, 
                    strResults[i][j].filterAct,
                    strResults[i][j].byAct);
        }
    }

    rc = rbusEvent_UnsubscribeEx(handle, subscription, 12);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_ValueChange rbusEvent_UnsubscribeEx rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);    
}

void testByValueChange(rbusHandle_t handle)
{
    int rc;
    int i;
    int maxWait;

    /*test the 'by' field*/
    rc = rbusEvent_Subscribe(handle, "Device.TestProvider.VCParamBy", byVCHandler, NULL, 0);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_ValueChange rbusEvent_Subscribe VCParamBy rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);

    for(i = 0; i < 3; ++i)
    {
        int pass;
        
        rc = rbus_setInt(handle, "Device.TestProvider.VCParamBy", (i+1)*1000);
        pass = rc == RBUS_ERROR_SUCCESS;
        printf("%s _test_ValueChange Device.TestProvider.VCParamBy %d rbus_setInt rc=%d\n", pass ? "PASS" : "FAIL", i, rc);

        maxWait = pass ? 5 : 0;
        while(byCounter.actual == i && maxWait > 0)
        {
            sleep(1);
            maxWait--;
        }

        pass  = byCounter.actual == i+1;
        TALLY(pass);
        if(pass)
            printf("PASS _test_ValueChange Device.TestProvider.VCParamBy received event\n");
        else
            printf("FAIL _test_ValueChange Device.TestProvider.VCParamBy didn't receive event\n");

        pass = 
            byResults[i].newValAct == byResults[i].newValExp &&
            byResults[i].oldValAct == byResults[i].oldValExp &&
            byResults[i].filterAct == byResults[i].filterExp &&
            !strcmp(byResults[i].byAct, byResults[i].byExp);
        TALLY(pass);
        printf("%s _test_ValueChangeDevice.TestProvider.VCParamBy %d expect=[value:%d oldValue:%d filter:%d by:%s] actual=[value:%d oldValue:%d filter:%d by:%s]\n", 
                pass ? "PASS" : "FAIL", 
                i,
                byResults[i].newValExp, 
                byResults[i].oldValExp, 
                byResults[i].filterExp,
                byResults[i].byExp,
                byResults[i].newValAct, 
                byResults[i].oldValAct, 
                byResults[i].filterAct,
                byResults[i].byAct);
    }    
}

void testNoAutoPubValueChange(rbusHandle_t handle)
{
    int rc;
    int i;
    rbusValue_t intVal;
    rbusFilter_t filter[2];
    int maxWait;
    int needMore;

    rbusValue_Init(&intVal);
    rbusValue_SetInt32(intVal, 5);

    rbusFilter_InitRelation(&filter[0], RBUS_FILTER_OPERATOR_LESS_THAN, intVal);
    rbusFilter_InitRelation(&filter[1], RBUS_FILTER_OPERATOR_GREATER_THAN, intVal);

    rbusEventSubscription_t subscription[2] = {
        {"Device.TestProvider.NoAutoPubInt", filter[0], 0, 0, noAutoPub1Handler, NULL, NULL, NULL, false},
        {"Device.TestProvider.NoAutoPubInt", filter[1], 0, 0, noAutoPub2Handler, NULL, NULL, NULL, false}
    };

    rc = rbusEvent_SubscribeEx(handle, subscription, 2, 0);

    rbusValue_Release(intVal);
    for(i = 0; i < 2; ++i)
        rbusFilter_Release(filter[i]);

    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_NoAutoPubValueChange rbusEvent_SubscribeEx rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);   

    maxWait = rc == RBUS_ERROR_SUCCESS ? 48 : 0;
    needMore = 1;
    while(needMore && maxWait > 0)
    {
        sleep(1);
        maxWait--;
        needMore = 0;
        for(i = 0; i < 2; ++i)
        {
            if(noAutoPubCounter[i].actual < noAutoPubCounter[i].expected)
                needMore = 1;
        }
    }

    for(i = 0; i < 2; ++i)
    {
        int j;
        int pass;
        
        pass  = noAutoPubCounter[i].actual >= noAutoPubCounter[i].expected;
        TALLY(pass);
        printf("%s _test_NoAutoPubValueChange Device.TestProvider.VCParamInt%d expected count=%d actual count=%d\n", pass ? "PASS" : "FAIL", i, noAutoPubCounter[i].expected, noAutoPubCounter[i].actual);
        for(j = 0; j < noAutoPubCounter[i].expected; ++j)
        {
            int pass = 
                noAutoPubResults[i][j].newValAct == noAutoPubResults[i][j].newValExp &&
                noAutoPubResults[i][j].oldValAct == noAutoPubResults[i][j].oldValExp &&
                noAutoPubResults[i][j].filterAct == noAutoPubResults[i][j].filterExp &&
                !strcmp(noAutoPubResults[i][j].byAct, noAutoPubResults[i][j].byExp);
            TALLY(pass);
            printf("%s _test_NoAutoPubValueChange Device.TestProvider.VCParamInt%d expect=[value:%d oldValue:%d filter:%d by:%s] actual=[value:%d oldValue:%d filter:%d by:%s]\n", 
                    pass ? "PASS" : "FAIL", 
                    i,
                    noAutoPubResults[i][j].newValExp, 
                    noAutoPubResults[i][j].oldValExp, 
                    noAutoPubResults[i][j].filterExp,
                    noAutoPubResults[i][j].byExp,
                    noAutoPubResults[i][j].newValAct, 
                    noAutoPubResults[i][j].oldValAct, 
                    noAutoPubResults[i][j].filterAct,
                    noAutoPubResults[i][j].byAct);
        }
    }

    rc = rbusEvent_UnsubscribeEx(handle, subscription, 2);
    TALLY(rc == RBUS_ERROR_SUCCESS);
    printf("%s _test_NoAutoPubValueChange rbusEvent_UnsubscribeEx rc=%d\n", rc == RBUS_ERROR_SUCCESS ? "PASS":"FAIL", rc);    
}

void testValueChange(rbusHandle_t handle, int* countPass, int* countFail)
{
    rbusConfig_Get()->valueChangePeriod = 1;
    testSimpleValueChange(handle);
    testTypesValueChange(handle);
    testByValueChange(handle);
    testNoAutoPubValueChange(handle);
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ValueChange");
}

