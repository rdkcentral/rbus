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

int loopFor = 30;
rbusHandle_t handle1;
rbusHandle_t handle2;

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

rbusError_t multiRbusProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusGetHandlerOptions_t* opts);
rbusError_t multiRbusProvider_SampleDataSetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    int rc1 = RBUS_ERROR_SUCCESS;
    int rc2 = RBUS_ERROR_SUCCESS;

    char componentName[] = "multiRbusOpenRbusGetProvider";

    rbusDataElement_t dataElements = {"Device.multiRbusOpenGetTestProvider.Value.RBUS_INT32", RBUS_ELEMENT_TYPE_PROPERTY, {multiRbusProvider_SampleDataGetHandler, multiRbusProvider_SampleDataSetHandler, NULL, NULL, NULL, NULL}};

    printf("provider: start\n");

    rc1 = rbus_open(&handle1, componentName);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: First rbus_open for handle1 err: %d\n", rc1);
        goto exit1;
    }

    rc2 = rbus_open(&handle2, componentName);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: Second rbus_open for handle2 err: %d\n", rc2);
        goto exit2;
    }

    rc1 = rbus_regDataElements(handle1, 1, &dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_regDataElements for handle1 err: %d\n", rc1);
    }

    rc2 = rbus_regDataElements(handle2, 1, &dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements for handle2 err: %d\n", rc2);
    }


    while (loopFor != 0)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
        loopFor--;
    }

    rc1 = rbus_unregDataElements(handle1, 1, &dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_unregDataElements for handle1 err: %d\n", rc1);
    }
    rc2 = rbus_unregDataElements(handle2, 1, &dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_unregDataElements for handle2 err: %d\n", rc2);
    }

    rc2 = rbus_close(handle2);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_close handle2 err: %d\n", rc2);
    }

exit2:
    rc1 = rbus_close(handle1);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("consumer: rbus_close handle1 err: %d\n", rc1);
    }
exit1:
    printf("provider: exit\n");
    return rc2;
}


rbusError_t multiRbusProvider_SampleDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    rbusValueType_t type = rbusValue_GetType(value);

    if(strcmp(name, "Device.multiRbusOpenGetTestProvider.Value.RBUS_INT32") == 0)
    {
        if (type != RBUS_INT32)
        {
            printf("%s Called Set handler with wrong data type\n", name);
            return RBUS_ERROR_INVALID_INPUT;
        }
        else
        {
            rbusProperty_SetValue(prop, value);
            //printf("%s Called Set handler with value = %d\n", name, rbusValue_GetInt32(value));
        }
    }

    return RBUS_ERROR_SUCCESS;
}


rbusError_t multiRbusProvider_SampleDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    int intData = 0;
    char const* name;

    rbusValue_Init(&value);
    name = rbusProperty_GetName(property);

    if(strcmp(name, "Device.multiRbusOpenGetTestProvider.Value.RBUS_INT32") == 0)
    {
        intData += 101;
        printf("Called get handler for [%s] & value is %d\n", name, intData);
        rbusValue_SetInt32(value, intData);
    }
    else
    {
        printf("Cant Handle [%s]\n", name);
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

