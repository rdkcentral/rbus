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

#define     TotalParams   6

char            componentName[] = "rbusSetMultiTestConsumer";
rbusHandle_t    handle;
char const*     paramNames[1] = {
    "Device.Provider1.Param3"
    };

int main(int argc, char *argv[])
{
    int rc;
    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }
    rbus_setLogLevel(RBUS_LOG_DEBUG);
    rbusValue_t value;
    rbusProperty_t props = NULL;
    int actualCount = 0;
    rc = rbus_getExt(handle, 1, &paramNames[0], &actualCount, &props);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf ("rbus_get failed for [%s] with error [%d]\n", paramNames[0], rc);
        rbus_close(handle);
        goto exit1;
    }
    value = rbusProperty_GetValue(props);
    printf("The value is = [%d]\n", rbusValue_GetInt32(value));
    rbusProperty_Release(props);

    {
        
        
        int rc = RBUS_ERROR_BUS_ERROR;
        rbusValue_t setVal1 = NULL, setVal2 = NULL;
        rbusProperty_t next = NULL, next1 = NULL, last = NULL;

        rbusValue_Init(&setVal1);
        rbusValue_SetFromString(setVal1, RBUS_STRING, "Gtest_set_multi_1");

        rbusValue_Init(&setVal2);
        rbusValue_SetFromString(setVal2, RBUS_STRING, "RBUS_test");

        rbusProperty_Init(&next,"Device.Provider1.Param1" , setVal1);
        rbusProperty_Init(&next1,"Device.Provider1.Param2" , setVal1);
        rbusProperty_Init(&last, "Device.Provider1.Param3", setVal2);
        rbusProperty_SetNext(next, next1);
        rbusProperty_SetNext(next1, last);
        char* failedElement = NULL;
        rc = rbus_setMultiExt(handle, 3, next, NULL, 0, &failedElement);
        printf("Result: %d", rc);
        if(failedElement)
            free(failedElement);
        rbusValue_Release(setVal1);
        rbusValue_Release(setVal2);
        rbusProperty_Release(next);
        rbusProperty_Release(next1);
        rbusProperty_Release(last);
    }
    rbus_close(handle);
exit1:
    printf("consumer: exit\n");
    return 0;
}


