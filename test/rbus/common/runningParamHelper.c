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

#define MAXPATH 100
static char gParamName[MAXPATH+1] = {0};
static bool gIsRunning = false;

static rbusError_t getRunningParamHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    if(strcmp(rbusProperty_GetName(property), gParamName) == 0)
    {
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetBoolean(value, gIsRunning);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);

        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_INVALID_INPUT;
}

static rbusError_t setRunningParamHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    if(strcmp(rbusProperty_GetName(property), gParamName) == 0)
    {
        rbusValue_t value = rbusProperty_GetValue(property);
        if(rbusValue_GetType(value) == RBUS_BOOLEAN)
        {
            gIsRunning = rbusValue_GetBoolean(value);
            printf("_test_:setRunningParamHandler result:SUCCESS value:%s\n", gIsRunning ? "TRUE" : "FALSE");
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            printf("_test_:setRunningParamHandler result:FAIL msg:'unexpected type %d'\n", rbusValue_GetType(value));
        }
    }
    printf("_test_:setRunningParamHandler result:FAIL msg:'unexpected name %s'\n", rbusProperty_GetName(property));
    return RBUS_ERROR_INVALID_INPUT;
}

rbusError_t runningParamProvider_Init(rbusHandle_t handle, char* paramName)
{
    int rc;

    strncpy(gParamName, paramName, MAXPATH);

    rbusDataElement_t dataElement = { paramName, RBUS_ELEMENT_TYPE_PROPERTY, {getRunningParamHandler, setRunningParamHandler, NULL, NULL, NULL, NULL}};

    rc = rbus_regDataElements(handle, 1, &dataElement);
    if(rc == RBUS_ERROR_SUCCESS)
    {
        printf("_test_:runningParamProvider_Init result:SUCCESS\n");
    }
    else
    {
        printf("_test_:runningParamProvider_Init result:FAIL msg:'rbus_regDataElements returned %d'\n", rc);
    }
    return rc;
}

int runningParamProvider_IsRunning()
{
    return gIsRunning;
}

int runningParamConsumer_Set(rbusHandle_t handle, char* paramName, bool running)
{
    int i;
    int loopFor = 1;
    rbusValue_t val;

    rbusValue_Init(&val);
    rbusValue_SetBoolean(val, running);

    if(running)
        loopFor = 5;/*give provider time to get ready*/

    for(i=0; i<loopFor; ++i)
    {
        int rc;
        rc = rbus_set(handle, paramName, val, NULL);
        printf("runningParamConsumer_Set rbus_set=%d\n", rc);
        if(rc == RBUS_ERROR_SUCCESS)
            break;
        if(i<loopFor-1)
            sleep(1);
    }

    rbusValue_Release(val);

    if(i==loopFor)
    {
        printf("_test_:runningParamConsumer_Set result:FAIL\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        printf("_test_:runningParamConsumer_Set result:SUCCESS\n");
        return RBUS_ERROR_SUCCESS;
    }
}
