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
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <rbuscore.h>
#include <rbus.h>
#include "rbusProviderConsumer.h"
#include <math.h>
#include <cfloat>
#define MIN(a,b) ((a) < (b) ? (a) : (b))

void hasProviderStarted(const char *provider);
void isElementPresent(rbusHandle_t handle, const char *elementName);
void getCompileTime(struct tm *t);
#define FILTER_VAL 3

static char gtest_err[64];

static bool asyncCalled = false;

void testOutParams(rbusObject_t outParams, char const* name, rbusError_t error)
{
    rbusValue_t val = rbusObject_GetValue(outParams, "name");

    printf("--STDOUT OutParams---\n");
    rbusObject_fwrite(outParams, 1, stdout);

    EXPECT_EQ(error, RBUS_ERROR_INVALID_INPUT);
}

static int exec_rbus_get_test(rbusHandle_t handle, const char *param)
{
  int rc = RBUS_ERROR_BUS_ERROR;
  rbusValue_t val = NULL;
  rbusValueType_t type = RBUS_NONE;

  isElementPresent(handle, param);
  rc = rbus_get(handle, param, &val);
  EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != rc) goto exit;

  rc = RBUS_ERROR_BUS_ERROR;
  type = rbusValue_GetType(val);
  if( ((0 == strcmp(param,"Device.rbusProvider.Int16")) && (RBUS_INT16 == type) && (GTEST_VAL_INT16 == rbusValue_GetInt16(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.Int64")) && (RBUS_INT64 == type) && (GTEST_VAL_INT64 == rbusValue_GetInt64(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.Int32")) && (RBUS_INT32 == type) && (GTEST_VAL_INT32 == rbusValue_GetInt32(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.UInt16")) && (RBUS_UINT16 == type) && (GTEST_VAL_UINT16 == rbusValue_GetUInt16(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.UInt64")) && (RBUS_UINT64 == type) && (GTEST_VAL_UINT64 == rbusValue_GetUInt64(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.UInt32")) && (RBUS_UINT32 == type) && (GTEST_VAL_UINT32 == rbusValue_GetUInt32(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.Single")) && (RBUS_SINGLE == type) && (GTEST_VAL_SINGLE == rbusValue_GetSingle(val))) ||
      ((0 == strcmp(param,"Device.rbusProvider.Double")) && (RBUS_DOUBLE == type) && (GTEST_VAL_DOUBLE == rbusValue_GetDouble(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegLong")) && (RBUS_INT64 == type) && (GTEST_VAL_INT64 == rbusValue_GetInt64(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegInt32")) && (RBUS_INT32 == type) && (GTEST_VAL_INT32 == rbusValue_GetInt32(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegULong")) && (RBUS_UINT64 == type) && (GTEST_VAL_UINT64 == rbusValue_GetUInt64(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegFloat")) && (RBUS_SINGLE == type) && (GTEST_VAL_SINGLE == rbusValue_GetSingle(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegUInt32")) && (RBUS_UINT32 == type) && (GTEST_VAL_UINT32 == rbusValue_GetUInt32(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegBoolean")) && (RBUS_BOOLEAN == type) && (GTEST_VAL_BOOL == rbusValue_GetBoolean(val))) ||
      ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegString")) && (RBUS_STRING == type) && (strcmp(rbusValue_GetString(val,NULL), GTEST_VAL_STRING) == 0))
    ) {
    rc = RBUS_ERROR_SUCCESS;

  } else if ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegByte")) && (RBUS_BYTE == type)) {
      rc = (rbusValue_GetByte(val) == 65) ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
  } else if ((0 == strcmp(param,"Device.rbuscoreProvider.GetLegBase64")) && (RBUS_BYTES == type)) {

    int len = 0;
    const uint8_t *ptr = rbusValue_GetBytes(val, &len);
    if(ptr)
      rc = (memcmp(ptr, GTEST_VAL_STRING, len) == 0) ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;

  } else if((0 == strcmp(param,"Device.rbuscoreProvider.GetLegDouble")) && (RBUS_DOUBLE == type)) {

    double retval=rbusValue_GetDouble(val);
    rc = ((abs(GTEST_VAL_DOUBLE-retval) < abs(MIN(GTEST_VAL_DOUBLE,retval))) * DBL_EPSILON) ?  RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;

  } else if((0 == strcmp(param,"Device.rbusProvider.DateTime") ||
        (0 == strcmp(param,"Device.rbuscoreProvider.GetLegDateTime"))) && (RBUS_DATETIME == type)) {

    struct tm compileTime;
    struct tm checkTime;
    rbusDateTime_t *rcTime = NULL;
    char CHECK_TIME[20];
    char COMPILE_TIME[20];

    rcTime = (rbusDateTime_t *)rbusValue_GetTime(val);
    getCompileTime(&compileTime);
    strftime(COMPILE_TIME, sizeof(COMPILE_TIME), "%x - %I:%M%p", &compileTime);
    printf("Formatted date & time : %s\n", COMPILE_TIME );

    rbusValue_UnMarshallRBUStoTM(&checkTime, rcTime);
    strftime(CHECK_TIME, sizeof(CHECK_TIME), "%x - %I:%M%p", &checkTime);
    printf("Formatted date & time : %s\n", CHECK_TIME );

    if(strcmp(COMPILE_TIME, CHECK_TIME)==0)
	    rc = RBUS_ERROR_SUCCESS;
    else
	    rc = RBUS_ERROR_BUS_ERROR;
  } else if((0 == strcmp(param,"Device.rbusProvider.Object")) && (RBUS_OBJECT == type)) {

    rbusObject_t obj = rbusValue_GetObject(val);
    if(obj)
      rc = (0 == strcmp(rbusObject_GetName(obj), "Device.rbusProvider.Object")) ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;

  } else if((0 == strcmp(param,"Device.rbusProvider.Property")) && (RBUS_PROPERTY == type)) {

    rbusProperty_t prop = rbusValue_GetProperty(val);
    if(prop)
      rc = (0 == strcmp(rbusProperty_GetName(prop), "Device.rbusProvider.Property")) ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;

  }

exit:
  rbusValue_Release(val);

  return rc;
}

static int exec_rbus_multi_test(rbusHandle_t handle, int expectedRc, int numProps, const char *param1, const char *param2)
{
  int rc = RBUS_ERROR_BUS_ERROR;
  rbusProperty_t properties = NULL;
  rbusValue_t setVal1 = NULL, setVal2 = NULL;
  rbusProperty_t next = NULL, last = NULL;

  rbusValue_Init(&setVal1);
  rbusValue_SetFromString(setVal1, RBUS_STRING, "Gtest_set_multi_1");

  rbusValue_Init(&setVal2);
  rbusValue_SetFromString(setVal2, RBUS_STRING, "Gtest_set_multi_2");

  rbusProperty_Init(&next, param1, setVal1);
  rbusProperty_Init(&last, param2, setVal2);
  rbusProperty_SetNext(next, last);

  rc = rbus_setMulti(handle, numProps, next, NULL);
  EXPECT_EQ(rc,expectedRc);

  rbusValue_Release(setVal1);
  rbusValue_Release(setVal2);
  rbusProperty_Release(next);
  rbusProperty_Release(last);

  return rc;
}

static int exec_rbus_set_test(rbusHandle_t handle, int expectedRc, const char *param, const char *paramValue)
{
  int rc = RBUS_ERROR_BUS_ERROR;
  rbusValue_t value = NULL;

  if(NULL == paramValue && NULL == param)
  {
    rbusValue_Init(&value);
    rc = rbus_set(handle, "test.params", value, NULL);
    rbusValue_Release(value);
  }
  else
  {
    if(paramValue)
    {
      rbusValue_Init(&value);
      rbusValue_SetString(value, (char*)paramValue);
    }
    rc = rbus_set(handle, param, value, NULL);
  }

  EXPECT_EQ(rc,expectedRc);
  if(paramValue)
    rbusValue_Release(value);

  return rc;
}

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
  (void)handle;

  rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
  rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
  rbusValue_t filter = rbusObject_GetValue(event->data, "filter");

  printf("Consumer receiver ValueChange event for param %s\n", event->name);

  if(newValue)
    printf("  New Value: %d\n", rbusValue_GetInt32(newValue));

  if(oldValue)
    printf("  Old Value: %d\n", rbusValue_GetInt32(oldValue));

  if(filter) {
    printf("  filter: %d\n", rbusValue_GetBoolean(filter));
    if(rbusValue_GetBoolean(filter) == 1) {
      int val = rbusValue_GetInt32(newValue);
      snprintf(gtest_err, sizeof(gtest_err), "%s", (val != FILTER_VAL) ? ("Invalid value is set") : "");
    }
  }

  if(subscription->userData)
    printf("User data: %s\n", (char*)subscription->userData);
}

static void eventReceiveHandler1(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    (void)handle;
    printf("\n => ReceiveHandlerConsumer: %s\n", __FUNCTION__);
    printf(" Consumer receiver Value event for param %s\n", event->name);
}

static void eventReceiveHandler2(
        rbusHandle_t handle,
        rbusEvent_t const* event,
        rbusEventSubscription_t* subscription)
{
    (void)handle;
    printf("\n => ReceiveHandlerConsumer: %s\n", __FUNCTION__);
    printf("Consumer receiver Value event for param %s\n", event->name);
    if (event->type == 6)
    {
        printf("\nConsumer received duration complete event\n");
        printf("*******************************************\n");
    }
}

static void asyncMethodHandler(
    rbusHandle_t handle,
    char const* methodName,
    rbusError_t error,
    rbusObject_t params)
{
  (void)handle;

  printf("%s called: method=%s  error=%d\n",__func__, methodName, error);

  asyncCalled = true;

}

static void asyncMethodHandler1(
    rbusHandle_t handle,
    char const* methodName,
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler2 called: method=%s  error=%d\n", methodName, error);

    testOutParams(params, "MethodAsync_2()",error);

}

static void subscribeHandler(
    rbusHandle_t handle,
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
  (void)handle;

  printf("subscribeHandler called:  error val=%d\n", error);
}


int rbusConsumer(rbusGtest_t test, pid_t pid, int runtime)
{
  int rc = RBUS_ERROR_BUS_ERROR;
  char user_data[32] = {0};
  rbusHandle_t handle;
  char *consumerName = NULL;
  static pid_t pid_arr[3] = {0};
  const char* event_param = "Device.rbusProvider.Param1";
  char* data[2] = { "My Data1", "My Data2"};
  rbusEventSubscription_t subscription = {event_param, NULL, 0, 0, (void *)eventReceiveHandler, NULL, 0};
  rbusEventSubscription_t sub[] = {
      {"Device.rbusProvider.Param1", NULL, 2, 0, (void *)eventReceiveHandler1, data[0], NULL, NULL, false},
      {"Device.rbusProvider.Param2", NULL, 2, 5, (void *)eventReceiveHandler2, data[1], NULL, NULL, false}
  };
  if(RBUS_GTEST_GET_EXT2 == test)
  {
    int i = 0;
    for(i = 0 ; i < 3 ; i++)
    {
      if(0 == pid_arr[i])
      {
        pid_arr[i] = pid;
        if(2 != i)
          return 0;
      }
    }
  }

  printf("%s: start \n",__func__);
  consumerName = strdup(__func__);
  rc = rbus_open(&handle, consumerName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != rc) goto exit;

  memset(gtest_err,0,sizeof(gtest_err));
  switch(test)
  {
    case RBUS_GTEST_FILTER1:
      {
        /* subscribe to all value change events on property "Device.rbusProvider.Param1" */
        isElementPresent(handle, event_param);
        strcpy(user_data,"My User Data");
        rc = rbusEvent_Subscribe(handle, event_param, eventReceiveHandler, user_data, 0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);

        rbusEvent_Unsubscribe(handle, event_param);
      }
      break;
    case RBUS_GTEST_FILTER2:
      {
        /* subscribe using filter to value change events on property "Device.rbusProvider.Param1"
           setting filter to: value >= 3.
         */
        rbusFilter_t filter;
        rbusValue_t filterValue;

        isElementPresent(handle, event_param);
        rbusValue_Init(&filterValue);
        rbusValue_SetInt32(filterValue, FILTER_VAL);

        rbusFilter_InitRelation(&filter, RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL, filterValue);

        subscription.filter = filter;

        rc = rbusEvent_SubscribeEx(handle, &subscription, 1, 0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        rbusValue_Release(filterValue);
        rbusFilter_Release(filter);
        sleep(runtime);

        rc |= rbusEvent_UnsubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        rc |= (strlen(gtest_err)) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;
      }
      break;
    case RBUS_GTEST_ASYNC_SUB1:
      {
        isElementPresent(handle, event_param);
        strcpy(user_data,"My User Data");
        rc = rbusEvent_SubscribeAsync(handle, event_param, eventReceiveHandler, subscribeHandler, user_data,0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);

        rc |= rbusEvent_Unsubscribe(handle, event_param);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_ASYNC_SUB2:
      {
        isElementPresent(handle, event_param);
        rc = rbusEvent_SubscribeExAsync(handle, &subscription, 1, subscribeHandler, 0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);

        rc |= rbusEvent_UnsubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_ASYNC_SUB3:
      {
        isElementPresent(handle, event_param);
        rc = rbusEvent_SubscribeExAsync(handle, &subscription, 1, subscribeHandler, -1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);

        rc |= rbusEvent_UnsubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_ASYNC_SUB4:
      {
        rc = rbusEvent_SubscribeExAsync(handle, &subscription, 1, subscribeHandler, (runtime/2));
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);

        rc |= rbusEvent_UnsubscribeEx(handle, &subscription, 1);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_ASYNC_SUB5:
      {
        strcpy(user_data,"My User Data");
        isElementPresent(handle, event_param);
        rc = rbusEvent_SubscribeAsync(handle, event_param, eventReceiveHandler, subscribeHandler, user_data,0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);
      }
      break;
    case RBUS_GTEST_INTERVAL_SUB1:
      {
        rc = rbusEvent_SubscribeEx(handle, sub, 2, 0);
        EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

        sleep(runtime);
        rc = rbusEvent_UnsubscribeEx(handle, sub, 1);
        EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_SET1:
      {
        const char *param = "Device.rbusProvider.Param2";
        isElementPresent(handle, param);
        rc = exec_rbus_set_test(handle, RBUS_ERROR_SUCCESS, param, "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET2:
      {
        const char *param = "Device.rbusProvider.PartialPath.1.Param2";
        isElementPresent(handle, param);
        rc = exec_rbus_set_test(handle, RBUS_ERROR_INVALID_OPERATION, param, "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET3:
      {
        const char *param = "Device.rbusProvider.Param2";
        isElementPresent(handle, param);
        rc = exec_rbus_set_test(handle, RBUS_ERROR_DESTINATION_NOT_REACHABLE, "Device.rbusProvider.Param4", "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET4:
      {
        const char *param = "Device.rbusProvider.PartialPath.1.Param1";
        isElementPresent(handle, param);
        rc = exec_rbus_set_test(handle, RBUS_ERROR_SUCCESS, param, "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET5:
      {
        rc = exec_rbus_set_test(handle, RBUS_ERROR_INVALID_INPUT, NULL, NULL);
      }
      break;
    case RBUS_GTEST_SET6:
      {
        rc = exec_rbus_set_test(NULL, RBUS_ERROR_INVALID_INPUT, "Device.rbusProvider.Param2", "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET7:
      {
        rc = exec_rbus_set_test(handle, RBUS_ERROR_INVALID_INPUT, NULL, "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET8:
      {
        rc = exec_rbus_set_test(handle, RBUS_ERROR_INVALID_INPUT, "Device.rbusProvider.Param2", NULL);
      }
      break;
    case RBUS_GTEST_SET9:
      {
        const char *param = "Device.rbusProvider.Int32";
        isElementPresent(handle, param);
        rc = rbus_setInt(handle, param, -10);
      }
      break;
    case RBUS_GTEST_SET10:
      {
        const char *param = "Device.rbusProvider.UInt32";
        isElementPresent(handle, param);
        rc = rbus_setUInt(handle, param, 10);
      }
      break;
    case RBUS_GTEST_SET11:
      {
        const char *param = "Device.rbusProvider.Param2";
        isElementPresent(handle, param);
        rc = rbus_setStr(handle, param, "Gtest set value");
      }
      break;
    case RBUS_GTEST_SET_MULTI1:
      {
        const char *param1 = "Device.rbusProvider.Param2";
        const char *param2 = "Device.rbusProvider.Param3";
        isElementPresent(handle, param1);
        isElementPresent(handle, param2);
        rc = exec_rbus_multi_test(handle, RBUS_ERROR_SUCCESS, 2, param1, param2);
      }
      break;
    case RBUS_GTEST_SET_MULTI2:
      {
        const char *param1 = "Device.rbusProvider.Param2";
        const char *param2 = "Device.rbusProvider.Param3";
        isElementPresent(handle, param1);
        isElementPresent(handle, param2);
        rc = exec_rbus_multi_test(handle, RBUS_ERROR_INVALID_INPUT, 3, param1, param2);
      }
      break;
    case RBUS_GTEST_SET_MULTI3:
      {
        const char *param1 = "Device.rbusProvider.Param2";
        const char *param2 = "Device.rbusProvider.Param4";
        isElementPresent(handle, param1);
        rc = exec_rbus_multi_test(handle, RBUS_ERROR_DESTINATION_NOT_REACHABLE, 2, param1, param2);
      }
      break;
    case RBUS_GTEST_SET_MULTI4:
      {
        const char *param1 = "Device.rbusProvider.Param2";
        const char *param2 = "Device.rbusProvider.PartialPath.1.Param1";
        isElementPresent(handle, param1);
        isElementPresent(handle, param2);
        rc = exec_rbus_multi_test(handle, RBUS_ERROR_SUCCESS, 2, param1, param2);
      }
      break;
    case RBUS_GTEST_SET_MULTI5:
      {
        const char *param1 = "Device.rbusProvider.Param2";
        const char *param2 = "Device.rbusProvider.PartialPath.1.Param2";
        isElementPresent(handle, param1);
        isElementPresent(handle, param2);
        rc = exec_rbus_multi_test(handle, RBUS_ERROR_INVALID_OPERATION, 2, param1, param2);
      }
      break;
    case RBUS_GTEST_GET1:
      {
        rc = rbus_get(NULL, "Device.rbusProvider.PartialPath.1.Param1", NULL);
        EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
      }
      break;
    case RBUS_GTEST_GET2:
      {
        rc = rbus_get(handle, "Device.rbusProvider.Method()", NULL);
        EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
      }
      break;
    case RBUS_GTEST_GET3:
      {
        rc = rbus_get(handle, NULL, NULL);
        EXPECT_EQ(rc, RBUS_ERROR_INVALID_INPUT);
      }
      break;
    case RBUS_GTEST_GET4:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.UInt16");
      }
      break;
    case RBUS_GTEST_GET5:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Int32");
      }
      break;
    case RBUS_GTEST_GET6:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.UInt32");
      }
      break;
    case RBUS_GTEST_GET7:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Int64");
      }
      break;
    case RBUS_GTEST_GET8:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.UInt64");
      }
      break;
    case RBUS_GTEST_GET9:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Single");
      }
      break;
    case RBUS_GTEST_GET10:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Double");
      }
      break;
    case RBUS_GTEST_GET11:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.DateTime");
      }
      break;
    case RBUS_GTEST_GET12:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Int16");
      }
      break;
    case RBUS_GTEST_GET13:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegUInt32");
      }
      break;
    case RBUS_GTEST_GET14:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegBoolean");
      }
      break;
    case RBUS_GTEST_GET15:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegLong");
      }
      break;
    case RBUS_GTEST_GET16:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegULong");
      }
      break;
    case RBUS_GTEST_GET17:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegFloat");
      }
      break;
    case RBUS_GTEST_GET18:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegDouble");
      }
      break;
    case RBUS_GTEST_GET19:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegByte");
      }
      break;
    case RBUS_GTEST_GET20:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegDateTime");
      }
      break;
    case RBUS_GTEST_GET21:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegBase64");
      }
      break;
    case RBUS_GTEST_GET22:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegString");
      }
      break;
    case RBUS_GTEST_GET23:
      {
        rc = exec_rbus_get_test(handle, "Device.rbuscoreProvider.GetLegInt32");
      }
      break;
    case RBUS_GTEST_GET24:
      {
        int value = 0;
        const char *param = "Device.rbuscoreProvider.GetLegCrInt32";
        isElementPresent(handle, param);
        rc = rbus_getInt(handle, param, &value);
        EXPECT_EQ(rc, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
      }
      break;
    case RBUS_GTEST_GET25:
      {
        char* value = NULL;
        const char *param = "Device.rbusProvider.PartialPath.1.Param1";
        isElementPresent(handle, param);
        rc = rbus_getStr(handle, param, &value);

        if(value)
        {
          rc |= strcmp(value,"Device.rbusProvider.PartialPath.1.Param1");
          free(value);
        }
      }
      break;
    case RBUS_GTEST_GET26:
      {
        unsigned int value = 0;
        const char *param = "Device.rbusProvider.UInt32";
        isElementPresent(handle, param);
        rc = rbus_getUint(handle, param, &value);
        rc |= !(value == GTEST_VAL_UINT32);
      }
      break;
    case RBUS_GTEST_GET27:
      {
        int value = 0;
        const char *param = "Device.rbusProvider.Int32";
        isElementPresent(handle, param);
        rc = rbus_getInt(handle, param, &value);
        rc |= !(value == GTEST_VAL_INT32);
      }
      break;
    case RBUS_GTEST_GET28:
      {
        unsigned int value = 0;
        const char *param = "Device.rbusProvider.Double";
        isElementPresent(handle, param);
        rc = rbus_getUint(handle, param, &value);
        EXPECT_EQ(rc, RBUS_ERROR_BUS_ERROR);
      }
      break;
    case RBUS_GTEST_GET29:
      {
        rc = rbus_get(handle, "Device.rbusProvider.", NULL);
        EXPECT_EQ(rc, RBUS_ERROR_ACCESS_NOT_ALLOWED);
      }
      break;
    case RBUS_GTEST_GET30:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Object");
      }
      break;
    case RBUS_GTEST_GET31:
      {
        rc = exec_rbus_get_test(handle, "Device.rbusProvider.Property");
      }
      break;
    case RBUS_GTEST_GET_EXT1:
      {
        rbusProperty_t props = NULL;
        rbusProperty_t next;
        int actualCount = 0;
        rbusValue_t actualValue;
        const char *params = "Device.rbusProvider.PartialPath.";

        isElementPresent(handle, params);

        rc = rbus_getExt(handle, 1, &params, &actualCount, &props);
        if(rc == RBUS_ERROR_SUCCESS)
        {
          next = props;
          while(next)
          {
            actualValue = rbusProperty_GetValue(next);
            if(actualValue != NULL && rbusValue_GetType(actualValue) == RBUS_STRING)
              printf("val %s\n", rbusValue_GetString(actualValue, NULL));

            //rbusProperty_fwrite(next, 1, stdout);
            next = rbusProperty_GetNext(next);
          }
          rbusProperty_Release(props);
        }
      }
      break;
    case RBUS_GTEST_GET_EXT2:
      {
        rbusProperty_t props = NULL;
        rbusProperty_t next;
        int actualCount = 0;
        rbusValue_t actualValue;
        char *str = NULL;
        const char *params[3] = {
          "Device.rbusMultiProvider0.Param1",
          "Device.rbusMultiProvider1.Param1",
          "Device.rbusMultiProvider2.Param1",
        };

        isElementPresent(handle, params[0]);
        isElementPresent(handle, params[1]);
        isElementPresent(handle, params[2]);

        rc = rbus_getExt(handle, 3, params, &actualCount, &props);
        if(rc == RBUS_ERROR_SUCCESS)
        {
          next = props;
          while(next)
          {
            actualValue = rbusProperty_GetValue(next);
            if(actualValue != NULL && rbusValue_GetType(actualValue) == RBUS_STRING)
            {
              str = (char *)rbusValue_GetString(actualValue, NULL);
              if((0 != strcmp(str,"Device.rbusMultiProvider0.Param1")) &&
                  (0 != strcmp(str,"Device.rbusMultiProvider1.Param1")) &&
                  (0 != strcmp(str,"Device.rbusMultiProvider2.Param1")))
              {
                rc = RBUS_ERROR_BUS_ERROR;
                break;
              }
            }

            next = rbusProperty_GetNext(next);
          }
          rbusProperty_Release(props);
        }

        kill(pid_arr[0],SIGUSR1);
        kill(pid_arr[1],SIGUSR1);
        kill(pid_arr[2],SIGUSR1);
        EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_DISC_COMP1:
      {
        int i;
        char const* elementNames1[] = {"Device.rbusProvider.Param1"};
        int numComponents = 0;
        char **componentName = NULL;

        rc = rbus_discoverComponentName(handle,1,elementNames1,&numComponents,&componentName);
        if(RBUS_ERROR_SUCCESS == rc) {
          printf ("Discovered components are,\n");
          for(i=0;i<numComponents;i++)
          {
            printf("rbus_discoverComponentName %s: %s\n", elementNames1[i],componentName[i]);
            free(componentName[i]);
          }
          free(componentName);
        }
      }
      break;
    case RBUS_GTEST_DISC_COMP2:
      {
        char const* elementNames1[] = {"Device.rbusProvider.Param1"};
        int numComponents = 0;
        char **componentName = NULL;

        rc = rbus_discoverComponentName(NULL,1,elementNames1,&numComponents,&componentName);
      }
      break;
    case RBUS_GTEST_DISC_COMP3:
      {
        char const* elementNames1[] = {"Device.rbusProvider.Param1"};
        int numComponents = 0;
        char **componentName = NULL;

        rc = rbus_discoverComponentName(handle,0,elementNames1,&numComponents,&componentName);
      }
      break;
    case RBUS_GTEST_DISC_COMP4:
      {
        char const* elementNames1[] = {NULL};
        int numComponents = 0;
        char **componentName = NULL;

        rc = rbus_discoverComponentName(handle,1,elementNames1,&numComponents,&componentName);
      }
      break;
    case RBUS_GTEST_METHOD1:
      {
        rbusObject_t inParams = NULL, outParams = NULL;
        rbusProperty_t prop = NULL;
        const char *method = "Device.rbusProvider.Method()";

        isElementPresent(handle, method);
        rbusObject_Init(&inParams, NULL);
        rbusProperty_Init(&prop, "param_names", NULL) ;
        rbusObject_SetProperty(inParams,prop);

        rc = rbusMethod_Invoke(handle, method, inParams, &outParams);
        rbusObject_Release(inParams);
        EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
        rbusProperty_Release(prop);

        if(RBUS_ERROR_SUCCESS == rc)
          rbusObject_Release(outParams);
      }
      break;
    case RBUS_GTEST_METHOD2:
      {
        rbusObject_t inParams = NULL, outParams = NULL;
        rbusValue_t value = NULL;
        rbusProperty_t prop = NULL;
        rbusValueType_t type = RBUS_STRING;
        bool ret = false;
        const char *method = "Device.rbusProvider.Method()";

        isElementPresent(handle, method);
        rbusValue_Init(&value);
        ret = rbusValue_SetFromString(value, type, "test_value");
        EXPECT_EQ(ret, true);
        if(true == ret)
        {
          rbusObject_Init(&inParams, NULL);
          rbusProperty_Init(&prop, "param_values", value);
          rbusObject_SetProperty(inParams,prop);
          rbusValue_Release(value);

          rc = rbusMethod_Invoke(handle, method, inParams, &outParams);
          rbusObject_Release(inParams);
          EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
          rbusProperty_Release(prop);
          if(rc == RBUS_ERROR_SUCCESS)
          {
            rbusObject_Release(outParams);
          }
        }
      }
      break;
    case RBUS_GTEST_METHOD3:
      {
        rbusObject_t inParams = NULL, outParams = NULL;
        rbusProperty_t prop = NULL;
        const char *method = "Device.rbusProvider.Method123()";

        isElementPresent(handle, method);
        rbusObject_Init(&inParams, NULL);
        rbusProperty_Init(&prop, "param_names", NULL) ;
        rbusObject_SetProperty(inParams,prop);

        rc = rbusMethod_Invoke(handle, method, inParams, &outParams);
        rbusObject_Release(inParams);
        EXPECT_NE(rc, RBUS_ERROR_SUCCESS);
        rc=0;
        rbusProperty_Release(prop);
        if(outParams)
        {
          rbusObject_fwrite(outParams, 1, stdout);
          rbusObject_Release(outParams);
        }
      }
      break;
    case RBUS_GTEST_METHOD4:
      {
        rbusObject_t inParams = NULL, outParams = NULL;
        rbusProperty_t prop = NULL;
        const char *method = "Device.rbusProvider.Method11()";

        isElementPresent(handle, method);
        rbusObject_Init(&inParams, NULL);
        rbusProperty_Init(&prop, "param_names", NULL) ;
        rbusObject_SetProperty(inParams,prop);

        rc = rbusMethod_Invoke(handle, method, inParams, &outParams);
        rbusObject_Release(inParams);
        EXPECT_NE(rc, RBUS_ERROR_SUCCESS);
        rc=0;
        rbusProperty_Release(prop);
        if(outParams)
        {
          rbusObject_fwrite(outParams, 1, stdout);
          rbusObject_Release(outParams);
        }
      }
      break;
    case RBUS_GTEST_METHOD_ASYNC:
      {
        rbusObject_t inParams;
        rbusValue_t value;

        rbusObject_Init(&inParams, NULL);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "param1");
        rbusObject_SetValue(inParams, "param1", value);
        rbusValue_Release(value);

        asyncCalled = false;
        rc = rbusMethod_InvokeAsync(handle, "Device.rbusProvider.MethodAsync1()", inParams, asyncMethodHandler, 0);
        printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.rbusProvider.MethodAsync1()",
            rc == RBUS_ERROR_SUCCESS ? "success" : "fail");
        sleep(runtime);
        rbusObject_Release(inParams);

      }
      break;
    case RBUS_GTEST_METHOD_ASYNC1:
      {

         rbusObject_t inParams;
         rbusValue_t value;

         rbusObject_Init(&inParams, NULL);
         rbusValue_Init(&value);
         rbusValue_SetString(value, "param1");
         rbusObject_SetValue(inParams, "param1", value);
         rbusValue_Release(value);

         asyncCalled = false;
         rc = rbusMethod_InvokeAsync(handle, "Device.rbusProvider.MethodAsync_2()", inParams, asyncMethodHandler1, 0);
         printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.rbusProvider.MethodAsync_2()",
         rc == RBUS_ERROR_SUCCESS ? "success" : "fail");
         sleep(runtime);
         rbusObject_Release(inParams);
      }
      break;
    case RBUS_GTEST_REG_ROW:
      {
        const char *param = "Device.rbusProvider.Param2";
        isElementPresent(handle, param);
        isElementPresent(handle, "Device.rbusProvider.PartialPath.");
        rc = exec_rbus_set_test(handle, RBUS_ERROR_SUCCESS, param, "register_row");
      }
      break;
    case RBUS_GTEST_UNREG_ROW:
      {
        const char *param = "Device.rbusProvider.Param2";
        isElementPresent(handle, param);
        rc = exec_rbus_set_test(handle, RBUS_ERROR_INVALID_INPUT, param, "unregister_row_fail");
      }
      break;
  }

  rc |= rbus_close(handle);
exit:
  free(consumerName);

  printf("%s: exit \n",__func__);
  return rc;
}
