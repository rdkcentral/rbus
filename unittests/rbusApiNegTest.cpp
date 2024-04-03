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
#include "gtest/gtest.h"
#include <rbus.h>
#include "rbus_handle.h"
#include "rbus_property.h"

static void subscribeHandler(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
  (void)handle;
}

static void asyncMethodHandler(
    rbusHandle_t handle,
    char const* methodName,
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;
}

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

TEST(rbusOpenNegTest, test1)
{
    const char *componentName = "rbusApi";
    int rc = RBUS_ERROR_BUS_ERROR;

    rc = rbus_open(NULL, componentName);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusOpenNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_BUS_ERROR;
    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));

    rc = rbus_open(&handle,NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusCloseNegTest, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;

    rc = rbus_close(NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusDisCompDataNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    int numElements = 0;
    char** elementNames = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc=rbus_discoverComponentDataElements(handle, NULL, false, &numElements, &elementNames);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusDisCompDataNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    int numElements = 0;
    char** elementNames = NULL;

    rc=rbus_discoverComponentDataElements(handle, "rbusApi", false, &numElements, &elementNames);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusregDataNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_regDataElements(handle, 1, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusregDataNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusDataElement_t dataElements[2] = {
        {(char *)"Device.Api.Event1"},
        {(char *)"Device.Api.Event2"}
    };

    rc = rbus_regDataElements(handle, 1, dataElements);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusregDataNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusDataElement_t dataElements[2] = {
        {(char *)"Device.Api.Event1"},
        {(char *)"Device.Api.Event2"}
    };

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_regDataElements(handle, 0, dataElements);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusUnregDataNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc=rbus_unregDataElements(handle, 1, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusUnregDataNegTest, test2)
{
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    rbusDataElement_t dataElements[2] = {
        {(char *)"Device.Api.Event1"},
        {(char *)"Device.Api.Event2"}
    };

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc=rbus_unregDataElements(handle, 0, dataElements);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusUnregDataNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusDataElement_t dataElements[2] = {
        {(char *)"Device.Api.Event1"},
        {(char *)"Device.Api.Event2"}
    };

    rc=rbus_unregDataElements(handle, 1, dataElements);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusgetExtNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    int actualCount = 10;
    const char *params = "Device.rbusProvider.PartialPath.*";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getExt(handle, 1, &params, &actualCount, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetExtNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t props;
    const char *params = "Device.rbusProvider.PartialPath.*";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getExt(handle, 1, &params, NULL, &props);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetExtNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t props;
    int actualCount = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getExt(handle, 1, NULL, &actualCount, &props);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetExtNegTest, test4)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t props;
    int actualCount = 0;
    const char *params = "Device.rbusProvider.PartialPath.*";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getExt(handle, 0, &params, &actualCount, &props);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetExtNegTest, test5)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t props;
    int actualCount = 0;
    const char *params = "Device.rbusProvider.PartialPath.*";

    rc = rbus_getExt(handle, 1, &params, &actualCount, &props);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusgetIntNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    int value = 0;

    rc = rbus_getInt(handle, "Device.rbusProvider1", &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusgetIntNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    int value = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getInt(handle, NULL, &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetIntNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getInt(handle, "Device.rbusProvider1", NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetUintNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    unsigned int value = 0;

    rc = rbus_getUint(handle, "Device.rbusProvider1", &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusgetUintNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    unsigned int value = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getUint(handle, NULL, &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetUintNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getUint(handle, "Device.rbusProvider1", NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetStrNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    char* value = NULL;

    rc = rbus_getStr(handle, "Device.rbusProvider.", &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusgetStrNegTest, test2)
{
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char* value = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getStr(handle, NULL, &value);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusgetStrNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    char* value = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getStr(handle, "Device.rbusProvider.", NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSetNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusValue_t value = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_set(handle, "test.params", value, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSetNegTest, test2)
{
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    rbusValue_t value = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rbusValue_Init(&value);
    rc = rbus_set(handle, NULL, value, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    rbusValue_Release(value);
    free(handle);
}

TEST(rbusSetNegTest, test3)
{
    rbusHandle_t handle = NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusValue_t value = NULL;

    rbusValue_Init(&value);
    rc = rbus_set(handle, "test.params", value, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    rbusValue_Release(value);
}

TEST(rbusSetMultiNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t next;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setMulti(handle, 0, next, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSetMultiNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setMulti(handle, 1, NULL, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSetMultiNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusProperty_t next=NULL;
    rbusValue_t setVal1 = NULL;
    rbusValue_Init(&setVal1);
    rbusValue_SetFromString(setVal1, RBUS_STRING, "Gtest_set_multi_1");
    rbusProperty_Init(&next, NULL, setVal1);

    rc = rbus_setMulti(handle, 1, next, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    rbusProperty_Release(next);
    rbusValue_Release(setVal1);
}

TEST(rbusSetIntNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbus_setInt(handle, "Device.rbusProvider.", 1);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSetIntNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setInt(handle, NULL, 1);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSetUintNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbus_setUInt(handle, "Device.rbusProvider.", 1);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSetUintNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setUInt(handle, NULL, 1);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSetStrNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbus_setStr(handle, "Device.rbusProvider.", "Hello");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSetStrNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setStr(handle, "Device.rbusProvider.", NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSetstrNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_setStr(handle, NULL, "Hello");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusTabAddRowNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    uint32_t instNum;

    rc=rbusTable_addRow(handle, "Device.rbusProvider.", "test", &instNum);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusTabAddRowNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    uint32_t instNum;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc=rbusTable_addRow(handle, NULL, NULL, &instNum);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusTabRemRowNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc=rbusTable_removeRow(handle,"Device.rbusProvider.");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusTabRemRowNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc=rbusTable_removeRow(handle,NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusTabRegRowNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbusTable_registerRow(handle, "Device.rbusProvider.PartialPath", 1, "test");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusTabRegRowNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusTable_registerRow(handle, NULL, 1, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusTabUnregRowNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbusTable_unregisterRow(handle, "Device.rbusProvider.");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusTabUnregRowNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusTable_unregisterRow(handle, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSubsNegTest, test1)
{
    rbusHandle_t handle=NULL;
    static char userData[] = "My Data";
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_Subscribe(handle,"Device.rbusProvider.", NULL, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSubsNegTest, test2)
{
    rbusHandle_t handle=NULL;
    static char userData[] = "My Data";
    rbusEventHandler_t handler;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_Subscribe(handle, NULL, handler, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSubsNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusEventHandler_t handler;
    static char userData[] = "My Data";

    rc = rbusEvent_Subscribe(handle,"Device.rbusProvider.", handler, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSubAsyncNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusEventHandler_t handler = NULL;
    static char userData[] = "My Data";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeAsync(handle, "Device.rbusProvider.", handler, NULL, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSubAsyncNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    static char userData[] = "My Data";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeAsync(handle, "Device.rbusProvider.", NULL,subscribeHandler, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSubAsyncNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusEventHandler_t handler;
    static char userData[] = "My Data";

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeAsync(handle, NULL, handler,subscribeHandler, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusSubAsyncNegTest, test4)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusEventHandler_t handler;
    static char userData[] = "My Data";

    rc = rbusEvent_SubscribeAsync(handle, "Device.rbusProvider.", handler,subscribeHandler, userData, 30);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}


TEST(rbusSubExAsyncNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.rbusProvider.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeExAsync(handle, &subscription, 0, subscribeHandler, 0);
    free(eventReceiveHandler);
    free(handle);
}

TEST(rbusSubExAsyncNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.rbusProvider.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeExAsync(handle, &subscription, 1, NULL, 0);
    free(eventReceiveHandler);
    free(handle);
}

TEST(rbusSubExAsyncNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeExAsync(handle, NULL, 1, subscribeHandler, 0);
    free(handle);
}

TEST(rbusSubExAsyncNegTest, test4)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.rbusProvider.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};

    rc = rbusEvent_SubscribeExAsync(handle, &subscription, 1, subscribeHandler, 0);
    free(eventReceiveHandler);
}

TEST(rbusUnsubNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    rc = rbusEvent_Unsubscribe(handle, "Device.Provider1.");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusUnsubNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_Unsubscribe(handle, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}

TEST(rbusUnsubExNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.rbusProvider.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_UnsubscribeEx(handle, &subscription, 0);
    free(eventReceiveHandler);
    free(handle);
}

TEST(rbusUnsubExNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.rbusProvider.Param1", NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};

    rc = rbusEvent_UnsubscribeEx(handle, &subscription, 1);
    free(eventReceiveHandler);
}

TEST(rbusUnsubExNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_UnsubscribeEx(handle, NULL, 1);
    free(handle);
}

TEST(rbusSubExNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.Provider1.Param1", NULL, 0, 0, eventReceiveHandler, NULL, NULL, NULL};

    rc = rbusEvent_SubscribeEx(handle, &subscription, 1, 0);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(eventReceiveHandler);
}

TEST(rbusSubExNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    void* eventReceiveHandler;
    eventReceiveHandler=(void *)malloc(100);
    rbusEventSubscription_t subscription = {"Device.Provider1.Param1", NULL, 0, 0, eventReceiveHandler, NULL, NULL, NULL};

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeEx(handle, &subscription, 0, 0);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(eventReceiveHandler);
    free(handle);
}

TEST(rbusSubExNegTest, test3)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_SubscribeEx(handle, NULL, 1, 0);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_HANDLE);
    free(handle);
}


TEST(rbusPublishNegTest, test1)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusEvent_Publish(handle, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusPulishNegTest, test2)
{
    rbusHandle_t handle=NULL;
    int rc = RBUS_ERROR_SUCCESS;
    rbusEvent_t event = {0};
    rbusObject_t data;
    rbusObject_Init(&data, NULL);
    event.name = "Device.Provider1";
    event.data = data;
    event.type = RBUS_EVENT_GENERAL;

    rc = rbusEvent_Publish(handle, &event);
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
    rbusObject_Release(data);
}

TEST(rbusSessionNegTest, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_createSession(handle , NULL);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSessionNegTest, test2)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    unsigned int sessionId = 0;

    rc = rbus_createSession(handle , &sessionId);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSessionNegTest, test3)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    unsigned int newSessionId = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_getCurrentSession(handle , NULL);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    free(handle);
}

TEST(rbusSessionNegTest, test4)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    unsigned int newSessionId = 0;

    rc = rbus_getCurrentSession(handle , &newSessionId);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
}

TEST(rbusSessionNegTest, test5)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    unsigned int sessionId = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_closeSession(handle, 0);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
    free(handle);
}

TEST(rbusSessionNegTest, test6)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    unsigned int sessionId = 0;

    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbus_closeSession(handle, sessionId);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
    free(handle);
}

TEST(rbusInvokeNegTest, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle;
    rbusObject_t inParams = NULL, outParams = NULL;
    const char *componentName = "rbusApi";

    rc=rbus_open(&handle,componentName);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);

    rbusObject_Init(&inParams, NULL);
    rc = rbusMethod_Invoke(handle, NULL, inParams, &outParams);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    if(outParams)
        rbusObject_Release(outParams);
    if(inParams)
       rbusObject_Release(inParams);
    rc=rbus_close(handle);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
}

TEST(rbusInvokeNegTest, test2)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusObject_t inParams = NULL, outParams = NULL;
    const char *method = "Device.rbusProvider.Method()";

    rbusObject_Init(&inParams, NULL);
    rc = rbusMethod_Invoke(NULL, method, inParams, &outParams);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    if(outParams)
        rbusObject_Release(outParams);
    if(inParams)
       rbusObject_Release(inParams);
}

TEST(rbusInvokeNegTest, test3)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle;
    rbusObject_t inParams = NULL, outParams = NULL;
    const char *method = "Device.rbusProvider.Method123()";
    const char *componentName = "rbusApi";

    rc=rbus_open(&handle,componentName);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);

    rbusObject_Init(&inParams, NULL);
    rc = rbusMethod_Invoke(handle, method, inParams, &outParams);
    EXPECT_EQ(rc, RBUS_ERROR_DESTINATION_NOT_FOUND);
    if(outParams)
        rbusObject_Release(outParams);
    if(inParams)
       rbusObject_Release(inParams);
    rc=rbus_close(handle);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
}

TEST(rbusInvokeAsyncNegTest, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    rbusObject_t inParams = NULL;

    rbusObject_Init(&inParams, NULL);
    rc = rbusMethod_InvokeAsync(handle, "Device.Methods.AsyncMethod()", inParams, asyncMethodHandler, 0);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    rbusObject_Release(inParams);
}

TEST(rbusInvokeAsyncNegTest, test2)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    rbusObject_t inParams = NULL;

    rbusObject_Init(&inParams, NULL);
    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusMethod_InvokeAsync(handle, NULL, inParams, asyncMethodHandler, 0);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_HANDLE);
    rbusObject_Release(inParams);
    free(handle);
}

TEST(rbusInvokeAsyncNegTest, test3)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusHandle_t handle = NULL;
    rbusObject_t inParams = NULL;

    rbusObject_Init(&inParams, NULL);
    handle = (struct _rbusHandle *) malloc(sizeof(struct _rbusHandle));
    rc = rbusMethod_InvokeAsync(handle, "Device.Methods.AsyncMethod()", inParams, NULL, 0);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_HANDLE);
    rbusObject_Release(inParams);
    free(handle);
}

TEST(rbusSendAsyncResNegTest, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rbusObject_t outParams;
    rbusObject_Init(&outParams, NULL);

    rc = rbusMethod_SendAsyncResponse(NULL, RBUS_ERROR_INVALID_INPUT, outParams);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
    if(outParams)
        rbusObject_Release(outParams);
}

TEST(rbusLogHandler, test1)
{
    int rc = RBUS_ERROR_BUS_ERROR;
    rc = rbus_registerLogHandler(NULL);
    EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
}

