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

static void asyncMethodHandler(
    rbusHandle_t handle,
    char const* methodName,
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler called: %s\n  error=%d", methodName, error);
    if(error == RBUS_ERROR_SUCCESS)
    {
        rbusObject_fwrite(params, 1, stdout);
    }
}

int main(int argc, char *argv[])
{
    rbusHandle_t handle;
    rbusObject_t inParams;
    rbusValue_t value;
    int rc = RBUS_ERROR_SUCCESS;

    (void)(argc);
    (void)(argv);

    printf("constumer: start\n");

    rc = rbus_open(&handle, "Discover_Elements_Consumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, "Device.");
    rbusObject_SetValue(inParams, "topLevelPath", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetBoolean(value, true);
    rbusObject_SetValue(inParams, "firstLevelOnly", value);
    rbusObject_SetValue(inParams, "returnProperties", value);
    rbusObject_SetValue(inParams, "returnMethods", value);
    rbusObject_SetValue(inParams, "returnEvents", value);
    rbusValue_Release(value);

    rc = rbusMethod_InvokeAsync(handle, "Device.X_RDK-SupportedDM.DiscoverSupportedDM", inParams, asyncMethodHandler, 0);
    rbusObject_Release(inParams);
    printf("consumer: rbusMethod_InvokeAsync(Device.X_RDK-SupportedDM.DiscoverSupportedDM) %s\n",
        rc == RBUS_ERROR_SUCCESS ? "success" : "fail");

    sleep(5);

    rbus_close(handle);

exit1:
    printf("consumer: exit\n");
    return rc;
}