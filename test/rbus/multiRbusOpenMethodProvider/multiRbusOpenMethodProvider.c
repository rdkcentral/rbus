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
#include <rtMemory.h>

int loopFor = 40;
rbusHandle_t handle1;
rbusHandle_t handle2;

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

static void* asyncMethodFunc(void *p)
{
    MethodData* data;
    rbusObject_t outParams;
    rbusValue_t value;
    rbusError_t err;

    printf("%s enter\n", __FUNCTION__);

    sleep(20);

    data = p;

    rbusValue_Init(&value);
    rbusValue_SetString(value, "Async Method Response");

    rbusObject_Init(&outParams, NULL);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);
    
    printf("%s sending response\n", __FUNCTION__);
    err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_SUCCESS, outParams);
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
    rbusValue_t value;

    printf("methodHandler called: %s\n", methodName);
    rbusObject_fwrite(inParams, 1, stdout);

    if(strcmp(methodName, "Device.Methods.SimpleMethod()") == 0)
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Simple Method Response");
        rbusObject_SetValue(outParams, "value", value);
        rbusValue_Release(value);

        return RBUS_ERROR_SUCCESS;
    }
    else if(strcmp(methodName, "Device.Methods.SimpleMethod1()") == 0)
    {
        rbusValue_t value1,value2;
        rbusValue_Init(&value1);
        rbusValue_SetInt32(value1, 56789);
        rbusObject_SetValue(outParams, "error_code", value1);
        rbusValue_Release(value1);

        rbusValue_Init(&value2);
        rbusValue_SetString(value2, "Provider Specific Error Code");
        rbusObject_SetValue(outParams, "error_string", value2);
        rbusValue_Release(value2);

        return RBUS_ERROR_INVALID_METHOD;

    }
    else if(strcmp(methodName, "Device.Methods.AsyncMethod()") == 0)
    {
        pthread_t pid;
        MethodData* data = rt_malloc(sizeof(MethodData));
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
    else
    {
        return RBUS_ERROR_BUS_ERROR;
    }
}


int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc1 = RBUS_ERROR_SUCCESS;
    int rc2 = RBUS_ERROR_SUCCESS;

    char componentName[] = "multiRbusOpenMethodProvider";

    rbusDataElement_t dataElements[3] = {
        {"Device.Methods.SimpleMethod()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.Methods.SimpleMethod1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.Methods.AsyncMethod()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}}
    };

    printf("multiRbusOpenMethodProvider: start\n");

    rc1 = rbus_open(&handle1, componentName);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: First rbus_open of handle1 err: %d\n", rc1);
        goto exit2;
    }

    rc2 = rbus_open(&handle2, componentName);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: Second rbus_open of handle2 err: %d\n", rc2);
        goto exit1;
    }

    rc1 = rbus_regDataElements(handle1, 3, dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_regDataElements handle1 err: %d\n", rc1);
    }

    rc2 = rbus_regDataElements(handle2, 3, dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements handle2 err: %d\n", rc2);
    }


    while (loopFor != 0)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
        loopFor--;
    }

    rc1 = rbus_unregDataElements(handle1, 3, dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_unregDataElements handle1 err: %d\n", rc1);
    }

    rc2 = rbus_unregDataElements(handle2, 3, dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_unregDataElements handle2 err: %d\n", rc2);
    }

   rc2 = rbus_close(handle2);
   if(rc2 != RBUS_ERROR_SUCCESS)
   {
      printf("provider: rbus_close handle2 err: %d\n", rc2);
   }
exit1:
   rc1 = rbus_close(handle1);
   if(rc1 != RBUS_ERROR_INVALID_HANDLE)
   {
      printf("provider: rbus_close handle1 err: %d\n", rc1);
   }

exit2:
    printf("multiRbusOpenMethodProvider: exit\n");
    return rc2;
}
