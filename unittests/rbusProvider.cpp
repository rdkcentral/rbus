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
#include <errno.h>

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

void getCompileTime(struct tm *t);

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

typedef enum _rbus_legacy_support
{
    RBUS_LEGACY_STRING = 0,    /**< Null terminated string                                           */
    RBUS_LEGACY_INT,           /**< Integer (2147483647 or -2147483648) as String                    */
    RBUS_LEGACY_UNSIGNEDINT,   /**< Unsigned Integer (ex: 4,294,967,295) as String                   */
    RBUS_LEGACY_BOOLEAN,       /**< Boolean as String (ex:"true", "false"                            */
    RBUS_LEGACY_DATETIME,      /**< ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ) as String                 */
    RBUS_LEGACY_BASE64,        /**< Base64 representation of data as String                          */
    RBUS_LEGACY_LONG,          /**< Long (ex: 9223372036854775807 or -9223372036854775808) as String */
    RBUS_LEGACY_UNSIGNEDLONG,  /**< Unsigned Long (ex: 18446744073709551615) as String               */
    RBUS_LEGACY_FLOAT,         /**< Float (ex: 1.2E-38 or 3.4E+38) as String                         */
    RBUS_LEGACY_DOUBLE,        /**< Double (ex: 2.3E-308 or 1.7E+308) as String                      */
    RBUS_LEGACY_BYTE,
    RBUS_LEGACY_NONE
} rbusLegacyDataType_t;

rbusError_t getVCHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  char const* name = rbusProperty_GetName(property);
  (void)handle;
  (void)opts;
  rbusValue_t value;

  rbusValue_Init(&value);
  if(strcmp("Device.rbusProvider.Param1",name) == 0)
  {
    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata = 0;  /*the actual value to send back*/
    static int32_t mydelta = 1; /*how much to change the value by*/
    static int32_t mycount = 0; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

    mycount++;

    if((mycount % myfreq) == 0)
    {
      mydata += mydelta;
      if(mydata == mymax)
        mydelta = -1;
      else if(mydata == mymin)
        mydelta = 1;
    }

    printf("Provider: Called get handler for [%s] val=[%d]\n", name, mydata);

    rbusValue_SetInt32(value, mydata);
  } else if(strcmp("Device.rbusProvider.DateTime",name) == 0) {
    rbusDateTime_t timeVal;
    memset(&timeVal,0,sizeof(timeVal));
    struct tm compileTime;
    getCompileTime(&compileTime);
    memcpy(&(timeVal.m_time), &compileTime, sizeof(struct tm));
    rbusValue_SetTime(value, &(timeVal));
  } else if(strcmp("Device.rbusProvider.Object",name) == 0) {
    rbusObject_t obj = NULL;
    rbusObject_Init(&obj, name);
    rbusValue_SetObject(value, obj);
    rbusObject_Release(obj);
  } else if(strcmp("Device.rbusProvider.Property",name) == 0) {
    rbusProperty_t prop = NULL;
    rbusProperty_Init(&prop, name, NULL);
    rbusValue_SetProperty(value, prop);
    rbusProperty_Release(prop);
  }
  else if(strcmp("Device.rbusProvider.Int16",name) == 0)
    rbusValue_SetInt16(value, GTEST_VAL_INT16);
  else if(strcmp("Device.rbusProvider.Int32",name) == 0)
    rbusValue_SetInt32(value, GTEST_VAL_INT32);
  else if(strcmp("Device.rbusProvider.Int64",name) == 0)
    rbusValue_SetInt64(value, GTEST_VAL_INT64);
  else if(strcmp("Device.rbusProvider.UInt16",name) == 0)
    rbusValue_SetUInt16(value, GTEST_VAL_UINT16);
  else if(strcmp("Device.rbusProvider.UInt32",name) == 0)
    rbusValue_SetUInt32(value, GTEST_VAL_UINT32);
  else if(strcmp("Device.rbusProvider.UInt64",name) == 0)
    rbusValue_SetUInt64(value, GTEST_VAL_UINT64);
  else if(strcmp("Device.rbusProvider.Single",name) == 0)
    rbusValue_SetSingle(value, GTEST_VAL_SINGLE);
  else if(strcmp("Device.rbusProvider.Double",name) == 0)
    rbusValue_SetDouble(value, GTEST_VAL_DOUBLE);
  else if(strcmp("Device.rbusMultiProvider0.Param1",name) == 0)
    rbusValue_SetString(value, name);
  else if(strcmp("Device.rbusMultiProvider1.Param1",name) == 0)
    rbusValue_SetString(value, name);
  else if(strcmp("Device.rbusMultiProvider2.Param1",name) == 0)
    rbusValue_SetString(value, name);

  rbusProperty_SetValue(property, value);
  rbusValue_Release(value);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
  (void)handle;
  (void)opts;
  char const* name = rbusProperty_GetName(property);
  rbusValue_t value = rbusProperty_GetValue(property);
  char *val = NULL;
  rbusError_t rc = RBUS_ERROR_SUCCESS;

  if(!value) return RBUS_ERROR_BUS_ERROR;

  val = rbusValue_ToString(value,NULL,0);
  printf("setHandler called: property=%s value %s\n", name,val);

  if(strcmp(val,"register_row") == 0) {
    rc = rbusTable_registerRow(handle, "Device.rbusProvider.PartialPath", 1, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
    if(RBUS_ERROR_SUCCESS == rc)
    {
      rc = rbusTable_unregisterRow(handle, "Device.rbusProvider.PartialPath");
      EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
    }
  } else if(strcmp(val,"unregister_row_fail") == 0) {
    rc = rbusTable_unregisterRow(handle, "Device.rbusProvider.PartialPath123");
    EXPECT_EQ(rc,RBUS_ERROR_INVALID_INPUT);
  }

  free(val);
  return rc;
}

rbusError_t ppTableGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  char const* name = rbusProperty_GetName(property);

  (void)handle;
  (void)opts;

  printf(
      "ppTableGetHandler called:\n" \
      "\tproperty=%s\n",
      name);

  return RBUS_ERROR_SUCCESS;
}
rbusError_t ppTableAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
  (void)handle;
  (void)aliasName;

  if(!strcmp(tableName, "Device.rbusProvider.PartialPath"))
  {
    static int instanceNumber = 1;
    *instNum = instanceNumber++;
  }

  printf("partialPathTableAddRowHandler table=%s instNum=%d\n", tableName, *instNum);
  return RBUS_ERROR_SUCCESS;
}

rbusError_t ppTableRemRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
  (void)handle;
  (void)rowName;
  return RBUS_ERROR_SUCCESS;
}

rbusError_t ppParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
  rbusValue_t value;
  char const* name = rbusProperty_GetName(property);

  (void)handle;
  (void)opts;

  printf(
      "ppParamGetHandler called:\n" \
      "\tproperty=%s\n",
      name);

  if(!strcmp(name, "Device.rbusProvider.PartialPath.1.Param1") ||
      !strcmp(name, "Device.rbusProvider.PartialPath.1.Param2") ||
      !strcmp(name, "Device.rbusProvider.PartialPath.2.Param1") ||
      !strcmp(name, "Device.rbusProvider.PartialPath.2.Param2")
    )
  {
    /*set value to the name of the parameter so consumer can easily verify result*/
    rbusValue_Init(&value);
    rbusValue_SetString(value, name);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
  }
  else
  {
    printf("ppParamGetHandler invalid name %s\n", name);
    return RBUS_ERROR_BUS_ERROR;
  }
}

static void* asyncMethodFunc(void *p)
{
    MethodData* data;
    rbusObject_t outParams;
    rbusValue_t value;
    rbusError_t err;
    char buff[256];
    char* str;

    printf("%s enter\n", __FUNCTION__);

    sleep(3);

    data = (MethodData*)p;

    rbusObject_Init(&outParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, "MethodAsync_2()");

    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    str = rbusValue_ToString(rbusObject_GetValue(data->inParams, "param1"), NULL, 0);
    snprintf(buff, 255, "Async Method Response inParams=%s\n", str);
    free(str);
    rbusValue_SetString(value, buff);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);

    printf("%s sending response\n", __FUNCTION__);
    err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_INVALID_INPUT, outParams);
    if(err != RBUS_ERROR_SUCCESS)
    {
        printf("%s rbusMethod_SendAsyncResponse failed err:%d\n", __FUNCTION__, err);
    }

    rbusObject_Release(data->inParams);
    rbusObject_Release(outParams);

    free(data);

    printf("%s exit\n", __FUNCTION__);

    return NULL;
}

static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
  (void)handle;
  (void)asyncHandle;
  rbusValue_t value;
  rbusError_t rc = RBUS_ERROR_BUS_ERROR;

  printf("methodHandler called: %s\n", methodName);
  rbusObject_fwrite(inParams, 1, stdout);

  if(strstr(methodName, "Method()")) {
    rbusValue_Init(&value);
    rbusValue_SetString(value, "Method1()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);
    rc = RBUS_ERROR_SUCCESS;
  } else if(strstr(methodName, "Method11()")) {
    rc = RBUS_ERROR_INVALID_OPERATION;
  } else if(strstr(methodName, "Method123()")) {
    rbusValue_t value1, value2;
    rbusValue_Init(&value1);
    rbusValue_Init(&value2);
    rbusValue_Init(&value);
    rbusValue_SetString(value, "Method123()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);
    rbusValue_SetInt32(value1, RBUS_ERROR_INVALID_OPERATION);
    rbusValue_SetString(value2, "RBUS_ERROR_INVALID_OPERATION");
    rbusObject_SetValue(outParams, "error_code", value1);
    rbusObject_SetValue(outParams, "error_string", value2);
    rbusValue_Release(value1);
    rbusValue_Release(value2);
    rc = RBUS_ERROR_INVALID_OPERATION;
  } else if(strstr(methodName, "MethodAsync1()")) {
    sleep(4);
    rbusValue_Init(&value);
    rbusValue_SetString(value, "MethodAsync1()");
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);
    rc = RBUS_ERROR_SUCCESS;
  }
  if(strstr(methodName, "MethodAsync_2()"))
  {
     pthread_t pid;
     MethodData* data = (MethodData*)malloc(sizeof(MethodData));
     data->asyncHandle = asyncHandle;
     data->inParams = inParams;

     rbusObject_Retain(inParams);
     if(pthread_create(&pid, NULL, asyncMethodFunc, data) || pthread_detach(pid))
     {
         printf("%s failed to spawn thread\n", __FUNCTION__);
         return RBUS_ERROR_BUS_ERROR;
     }
     return RBUS_ERROR_ASYNC_RESPONSE;
  }
  printf("methodHandler %s\n",(RBUS_ERROR_SUCCESS == rc) ? "success": "fail");
  return rc;
}

int rbusProvider(rbusGtest_t test, pid_t pid, int *consumer_status)
{
  rbusHandle_t handle;
  int rc = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  char *componentName = NULL;

  rbusDataElement_t dataElements[] = {
    {(char *)"Device.rbusProvider.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, setHandler, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Param3", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, setHandler, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Int16", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Int32", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, setHandler, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Int64", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.UInt16", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.UInt32", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, setHandler, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.UInt64", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Single", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Double", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Object", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Property", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.DateTime", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.PartialPath.{i}.", RBUS_ELEMENT_TYPE_TABLE, {ppTableGetHandler, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
    {(char *)"Device.rbusProvider.PartialPath.{i}.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, setHandler, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.PartialPath.{i}.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
    {(char *)"Device.rbusProvider.Method()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
    {(char *)"Device.rbusProvider.Method11()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
    {(char *)"Device.rbusProvider.Method123()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
    {(char *)"Device.rbusProvider.MethodAsync1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
    {(char *)"Device.rbusProvider.MethodAsync_2()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}}
  };
#define elements_count sizeof(dataElements)/sizeof(dataElements[0])

  componentName = strdup(__func__);
  printf("%s: start\n",componentName);
  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != rc) goto exit2;

  if(RBUS_GTEST_ASYNC_SUB4 == test)
    sleep(7);

  rc = rbus_regDataElements(handle, elements_count, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != rc) goto exit1;

  if(RBUS_GTEST_GET1 == test ||
      RBUS_GTEST_GET_EXT1 == test ||
      RBUS_GTEST_SET4 == test ||
      RBUS_GTEST_SET_MULTI4 == test ||
      RBUS_GTEST_SET_MULTI5 == test)
  {
    rc |= rbusTable_addRow(handle, "Device.rbusProvider.PartialPath.", NULL, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
    rc |= rbusTable_addRow(handle, "Device.rbusProvider.PartialPath.", NULL, NULL);
    EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  }

  wait_ret = waitpid(pid, consumer_status, 0);
  EXPECT_EQ(wait_ret,pid);

  if(wait_ret != pid) printf("%s: waitpid() failed %d: %s\n",__func__,errno,strerror(errno));
  rc = (wait_ret != pid) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;

  if(RBUS_GTEST_SET4 == test )
  {
    rc |= rbusTable_removeRow(handle,"Device.rbusProvider.PartialPath.0");
    EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  }

  rc |= rbus_unregDataElements(handle, elements_count, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

exit1:
  rc |= rbus_close(handle);

exit2:
  free(componentName);
  printf("%s: exit\n",__func__);
  return rc;
}

int rbusProvider1(int runtime,int should_exit)
{
  rbusHandle_t handle;
  int rc = RBUS_ERROR_BUS_ERROR;

  char *componentName = NULL;
  rbusDataElement_t dataElements[] = {
    {(char *)"Device.rbusProvider.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}}
  };
#define elements_count sizeof(dataElements)/sizeof(dataElements[0])

  printf("%s: start\n",__func__);

  componentName = strdup(__func__);
  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != rc) goto exit2;

  rc = rbus_regDataElements(handle, 1, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != rc) goto exit1;

  sleep(runtime);

  if(!should_exit)
    goto exit2;

exit1:
  rc |= rbus_close(handle);
exit2:
  free(componentName);
  printf("%s: exit\n",__func__);
  return rc;
}

int rbusMultiProvider(int index)
{
  rbusHandle_t handle;
  int rc = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  sigset_t set;
  int sig;
  char el_name[64] = {0};
  char componentName[32] = {0};

  rbusDataElement_t dataElements[] = {
    {NULL, RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler, NULL, NULL, NULL, NULL, NULL}}
  };
#define elements_count sizeof(dataElements)/sizeof(dataElements[0])

  snprintf(componentName,sizeof(componentName),"%s%d",__func__,index);
  printf("%s: start\n",componentName);
  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != rc) goto exit2;

  snprintf(el_name,sizeof(el_name),"Device.%s.Param1",componentName);
  dataElements[0].name = el_name;
  rc = rbus_regDataElements(handle, elements_count, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != rc) goto exit1;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);
  sigwait(&set, &sig);
  printf("%s Got signal %d: %s\n", componentName, sig, strsignal(sig));

  rc |= rbus_unregDataElements(handle, elements_count, dataElements);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

exit1:
  rc |= rbus_close(handle);

exit2:
  printf("%s: exit\n",componentName);
  return rc;
}

static int handle_get(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
  (void) message;
  (void) method;
  (void) hdr;
  char buffer[100] = {0};
  rbusGtest_t *test = (rbusGtest_t *)user_data;

  rbusMessage_Init(response);

  rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
  rbusMessage_SetInt32(*response, 1);
  if(RBUS_GTEST_GET24 != *test)
    rbusMessage_SetString(*response, destination);

  switch(*test)
  {
    case RBUS_GTEST_GET13:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_UNSIGNEDINT);
      snprintf(buffer, sizeof(buffer), "%d", GTEST_VAL_UINT32);
      break;
    case RBUS_GTEST_GET14:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_BOOLEAN);
      snprintf(buffer, sizeof(buffer), "%d", GTEST_VAL_BOOL);
      break;
    case RBUS_GTEST_GET15:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_LONG);
      snprintf(buffer, sizeof(buffer), "%" PRIi64, GTEST_VAL_INT64);
      break;
    case RBUS_GTEST_GET16:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_UNSIGNEDLONG);
      snprintf(buffer, sizeof(buffer), "%" PRIu64, GTEST_VAL_UINT64);
      break;
    case RBUS_GTEST_GET17:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_FLOAT);
      snprintf(buffer, sizeof(buffer), "%.15f", GTEST_VAL_SINGLE);
      break;
    case RBUS_GTEST_GET18:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_DOUBLE);
      snprintf(buffer, sizeof(buffer), "%.15f", GTEST_VAL_DOUBLE);
      break;
    case RBUS_GTEST_GET19:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_BYTE);
      snprintf(buffer, sizeof(buffer), "%s", "A");
      break;
    case RBUS_GTEST_GET20:
      {
        struct tm compileTime;
        char buf[80] = {0};

        getCompileTime(&compileTime);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &compileTime);

        rbusMessage_SetInt32(*response, RBUS_LEGACY_DATETIME);
        snprintf(buffer, sizeof(buffer), "%s", buf);
      }
      break;
    case RBUS_GTEST_GET21:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_BASE64);
      snprintf(buffer, sizeof(buffer), "%s", GTEST_VAL_STRING);
      break;
    case RBUS_GTEST_GET22:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_STRING);
      snprintf(buffer, sizeof(buffer), "%s", GTEST_VAL_STRING);
      break;
    case RBUS_GTEST_GET23:
      rbusMessage_SetInt32(*response, RBUS_LEGACY_INT);
      snprintf(buffer, sizeof(buffer), "%d", GTEST_VAL_INT32);
      break;
  }

  rbusMessage_SetString(*response, buffer);

  return 0;
}

int rbuscoreProvider(rbusGtest_t test, pid_t pid, int *consumer_status)
{
  rbusCoreError_t err = RBUSCORE_ERROR_GENERAL;
  int rc = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  const char *object_name = NULL;
  rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, &test, handle_get}};

  printf("%s: start \n",__func__);
  switch(test)
  {
    case RBUS_GTEST_GET13: object_name = "Device.rbuscoreProvider.GetLegUInt32";   break;
    case RBUS_GTEST_GET14: object_name = "Device.rbuscoreProvider.GetLegBoolean";  break;
    case RBUS_GTEST_GET15: object_name = "Device.rbuscoreProvider.GetLegLong";     break;
    case RBUS_GTEST_GET16: object_name = "Device.rbuscoreProvider.GetLegULong";    break;
    case RBUS_GTEST_GET17: object_name = "Device.rbuscoreProvider.GetLegFloat";    break;
    case RBUS_GTEST_GET18: object_name = "Device.rbuscoreProvider.GetLegDouble";   break;
    case RBUS_GTEST_GET19: object_name = "Device.rbuscoreProvider.GetLegByte";    break;
    case RBUS_GTEST_GET20: object_name = "Device.rbuscoreProvider.GetLegDateTime"; break;
    case RBUS_GTEST_GET21: object_name = "Device.rbuscoreProvider.GetLegBase64";   break;
    case RBUS_GTEST_GET22: object_name = "Device.rbuscoreProvider.GetLegString";   break;
    case RBUS_GTEST_GET23: object_name = "Device.rbuscoreProvider.GetLegInt32";    break;
    case RBUS_GTEST_GET24: object_name = "Device.rbuscoreProvider.GetLegCrInt32";  break;
  }

  err = rbus_openBrokerConnection(object_name);
  EXPECT_EQ(err,RBUSCORE_SUCCESS);
  if(RBUSCORE_SUCCESS != err) goto exit1;

  err = rbus_registerObj(object_name, handle_get, NULL);
  EXPECT_EQ(err,RBUSCORE_SUCCESS);
  if(RBUSCORE_SUCCESS != err) goto exit2;

  err = rbus_registerMethodTable(object_name, table, 1);
  EXPECT_EQ(err,RBUSCORE_SUCCESS);
  if(RBUSCORE_SUCCESS != err) goto exit2;

  wait_ret = waitpid(pid, consumer_status, 0);
  EXPECT_EQ(wait_ret,pid);

  if(wait_ret != pid) printf("%s: waitpid() failed %d: %s\n",__func__,errno,strerror(errno));
  rc = (wait_ret != pid) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;

exit2:
  err = rbus_closeBrokerConnection();
  EXPECT_EQ(err,RBUSCORE_SUCCESS);
  rc |= (RBUSCORE_SUCCESS == err) ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;

exit1:
  printf("%s: exit\n",__func__);
  return rc;
}
