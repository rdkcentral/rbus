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

#include <rbus.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../common/test_macros.h"

static int gDuration = 2;


int getDurationPartialPath()
{
    return gDuration;
}

static void test(rbusHandle_t handle, char const* query, int expectCount, ...)
{
    va_list vl;
    rbusError_t rc;
    rbusProperty_t props = NULL;
    rbusProperty_t next;
    int actualCount = 0; 
    char const* expectedValue;
    rbusValue_t actualValue;

    printf("test partial path query=%s expectCount=%d\n", query, expectCount);

    rc = rbus_getExt(handle, 1, &query, &actualCount, &props);

    printf("test partial path rc=%d actualCount=%d\n", rc, actualCount);

    TEST(rc == RBUS_ERROR_SUCCESS);

    if(rc == RBUS_ERROR_SUCCESS)
    {
        TEST(actualCount == expectCount);

        if(actualCount == expectCount)
        {
            next = props; 

            va_start(vl,expectCount);

            while(next)
            {
                expectedValue = va_arg(vl, char const*);
                actualValue = rbusProperty_GetValue(next);

                TEST(actualValue != NULL && rbusValue_GetType(actualValue) == RBUS_STRING && 
                    !strcmp(rbusValue_GetString(actualValue, NULL), expectedValue));

                //rbusProperty_fwrite(next, 1, stdout);
                next =  rbusProperty_GetNext(next);
            }

            va_end(vl);
        }

        rbusProperty_Release(props);
    }

    return;
}

static void test1(rbusHandle_t handle, char const* prop)
{
    rbusError_t rc;
    char* value = NULL;

    printf("test1 prop=%s\n", prop);

    rc = rbus_getStr(handle, prop, &value);

    TEST(rc == RBUS_ERROR_SUCCESS);

    if(rc == RBUS_ERROR_SUCCESS)
    {
        TEST(!strcmp(prop, value));
        free(value);
    }
}

void testPartialPath(rbusHandle_t handle, int* countPass, int* countFail)
{
    test(handle, "Device.TestProvider.PartialPath1.", 20,
        "Device.TestProvider.PartialPath1.1.Param1",
        "Device.TestProvider.PartialPath1.1.Param2",
        "Device.TestProvider.PartialPath1.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.1.SubObject1.Param4",
        "Device.TestProvider.PartialPath1.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath1.2.Param1",
        "Device.TestProvider.PartialPath1.2.Param2",
        "Device.TestProvider.PartialPath1.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.2.SubObject1.Param4",
        "Device.TestProvider.PartialPath1.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath1.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7",
        "Device.TestProvider.PartialPath1.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.", 20,
        "Device.TestProvider.PartialPath2.1.Param1",
        "Device.TestProvider.PartialPath2.1.Param2",
        "Device.TestProvider.PartialPath2.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.1.SubObject1.Param4",
        "Device.TestProvider.PartialPath2.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath2.2.Param1",
        "Device.TestProvider.PartialPath2.2.Param2",
        "Device.TestProvider.PartialPath2.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.2.SubObject1.Param4",
        "Device.TestProvider.PartialPath2.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath2.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7",
        "Device.TestProvider.PartialPath2.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.1.", 7,
        "Device.TestProvider.PartialPath1.1.Param1",
        "Device.TestProvider.PartialPath1.1.Param2",
        "Device.TestProvider.PartialPath1.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.1.SubObject1.Param4",
        "Device.TestProvider.PartialPath1.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.1.", 7,
        "Device.TestProvider.PartialPath2.1.Param1",
        "Device.TestProvider.PartialPath2.1.Param2",
        "Device.TestProvider.PartialPath2.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.1.SubObject1.Param4",
        "Device.TestProvider.PartialPath2.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.", 13,
        "Device.TestProvider.PartialPath1.2.Param1",
        "Device.TestProvider.PartialPath1.2.Param2",
        "Device.TestProvider.PartialPath1.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.2.SubObject1.Param4",
        "Device.TestProvider.PartialPath1.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath1.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7",
        "Device.TestProvider.PartialPath1.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.", 13,
        "Device.TestProvider.PartialPath2.2.Param1",
        "Device.TestProvider.PartialPath2.2.Param2",
        "Device.TestProvider.PartialPath2.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.2.SubObject1.Param4",
        "Device.TestProvider.PartialPath2.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7",
        "Device.TestProvider.PartialPath2.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7",
        "Device.TestProvider.PartialPath2.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.1.SubObject1.", 2,
        "Device.TestProvider.PartialPath1.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.1.SubObject1.Param4");

    test(handle, "Device.TestProvider.PartialPath2.1.SubObject1.", 2,
        "Device.TestProvider.PartialPath2.1.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.1.SubObject1.Param4");

    test(handle, "Device.TestProvider.PartialPath1.1.SubTable.1.", 3,
        "Device.TestProvider.PartialPath1.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.1.SubTable.1.", 3,
        "Device.TestProvider.PartialPath2.1.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.SubObject1.", 2,
        "Device.TestProvider.PartialPath1.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath1.2.SubObject1.Param4");

    test(handle, "Device.TestProvider.PartialPath2.2.SubObject1.", 2,
        "Device.TestProvider.PartialPath2.2.SubObject1.Param3",
        "Device.TestProvider.PartialPath2.2.SubObject1.Param4");

    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.1.", 3,
        "Device.TestProvider.PartialPath1.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.1.", 3,
        "Device.TestProvider.PartialPath2.2.SubTable.1.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.2.", 3,
        "Device.TestProvider.PartialPath1.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.2.", 3,
        "Device.TestProvider.PartialPath2.2.SubTable.2.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.3.", 3,
        "Device.TestProvider.PartialPath1.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.3.", 3,
        "Device.TestProvider.PartialPath2.2.SubTable.3.Param5",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7");


    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.", 2,
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.", 2,
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.", 2,
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.", 2,
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.", 2,
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7");

    test(handle, "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.", 2,
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6",
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7");

    /*throw in some individual gets to ensure they are working with this data set*/

    test1(handle,  "Device.TestProvider.PartialPath1.1.Param1");
    test1(handle,  "Device.TestProvider.PartialPath1.1.Param2");
    test1(handle,  "Device.TestProvider.PartialPath1.1.SubObject1.Param3");
    test1(handle,  "Device.TestProvider.PartialPath1.1.SubObject1.Param4");
    test1(handle,  "Device.TestProvider.PartialPath1.1.SubTable.1.Param5");
    test1(handle,  "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath1.2.Param1");
    test1(handle,  "Device.TestProvider.PartialPath1.2.Param2");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubObject1.Param3");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubObject1.Param4");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.1.Param5");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.2.Param5");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.3.Param5");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7");

    test1(handle,  "Device.TestProvider.PartialPath2.1.Param1");
    test1(handle,  "Device.TestProvider.PartialPath2.1.Param2");
    test1(handle,  "Device.TestProvider.PartialPath2.1.SubObject1.Param3");
    test1(handle,  "Device.TestProvider.PartialPath2.1.SubObject1.Param4");
    test1(handle,  "Device.TestProvider.PartialPath2.1.SubTable.1.Param5");
    test1(handle,  "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath2.2.Param1");
    test1(handle,  "Device.TestProvider.PartialPath2.2.Param2");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubObject1.Param3");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubObject1.Param4");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.1.Param5");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.2.Param5");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.3.Param5");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6");
    test1(handle,  "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7");

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_PartialPath");
}

