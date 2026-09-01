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
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>

int loopFor = 50;
rbusError_t GetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusGetHandlerOptions_t* opts);
rbusError_t SetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts);

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    rbusHandle_t handle;
    int ret = RBUS_ERROR_SUCCESS;

    char componentName[] = "testTimeoutValuesProvider";

    rbusDataElement_t dataElements = {"Device.TestTimeoutValues.Value.RBUS_INT32", RBUS_ELEMENT_TYPE_PROPERTY, {GetHandler, SetHandler, NULL, NULL, NULL, NULL}};

    printf("provider: start\n");

    ret = rbus_open(&handle, componentName);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        printf("provider: First rbus_open for handle1 err: %d\n", ret);
        goto exit1;
    }

    ret = rbus_regDataElements(handle, 1, &dataElements);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements for handle2 err: %d\n", ret);
    }

    while (loopFor != 0)
    {
        printf("provider: exiting in %d seconds\n", loopFor);
        sleep(1);
        loopFor--;
    }

    ret = rbus_unregDataElements(handle, 1, &dataElements);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_unregDataElements for handle err: %d\n", ret);
    }

    ret = rbus_close(handle);
    if(ret != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("consumer: rbus_close handle err: %d\n", ret);
    }
exit1:
    printf("provider: exit\n");
    return ret;
}


rbusError_t SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    printf("%s Called Set handler with value = %d\n", name, rbusValue_GetInt32(value));
    sleep(5);
    return RBUS_ERROR_SUCCESS;
}


rbusError_t GetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    int intData = 0;
    char const* name;

    rbusValue_Init(&value);
    name = rbusProperty_GetName(property);

    if(strcmp(name, "Device.TestTimeoutValues.Value.RBUS_INT32") == 0)
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
    
    sleep(5);

    return RBUS_ERROR_SUCCESS;
}

