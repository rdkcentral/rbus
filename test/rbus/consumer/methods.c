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
#include "../common/test_macros.h"

int getDurationMethods()
{
    return 12;
}

static bool asyncCalled = false;
static rbusError_t asyncError = RBUS_ERROR_SUCCESS;
static int asyncCount = 0;

void testOutParams(rbusObject_t outParams, char const* name)
{
    rbusValue_t val = rbusObject_GetValue(outParams, "name");

    rbusObject_fwrite(outParams, 1, stdout);

    TEST(val != NULL);

    if(val)
    {
        TEST(strcmp(rbusValue_GetString(val,NULL), name) == 0);
    }
}

static void asyncMethodHandler1(
    rbusHandle_t handle, 
    char const* methodName, 
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler1 called: method=%s  error=%d\n", methodName, error);

    asyncCalled = true;

    TEST(error == asyncError);

    if(error == RBUS_ERROR_SUCCESS)
    {
        testOutParams(params, "MethodAsync1()");
    }
}

static void asyncMethodHandler2(
    rbusHandle_t handle, 
    char const* methodName, 
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler2 called: method=%s  error=%d\n", methodName, error);

    asyncCount++;

    TEST(error == asyncError);

    if(error == RBUS_ERROR_SUCCESS)
    {
        testOutParams(params, "MethodAsync2()");
    }
}

static void asyncMethodHandler3(
    rbusHandle_t handle,
    char const* methodName,
    rbusError_t error,
    rbusObject_t params)
{
    (void)handle;

    printf("asyncMethodHandler3 called: method=%s  error=%d\n", methodName, error);

    asyncCount++;

    testOutParams(params, "MethodAsync3()");
}

void testMethods(rbusHandle_t handle, int* countPass, int* countFail)
{
    rbusError_t err = 0;
    uint32_t instNum1, instNum2;
    char table2[RBUS_MAX_NAME_LENGTH];
    char row1[RBUS_MAX_NAME_LENGTH];
    char row2[RBUS_MAX_NAME_LENGTH];
    char method1[RBUS_MAX_NAME_LENGTH];
    char method11[RBUS_MAX_NAME_LENGTH];
    char method2[RBUS_MAX_NAME_LENGTH];
    char method3[RBUS_MAX_NAME_LENGTH];
    rbusObject_t inParams;
    rbusObject_t outParams;
    rbusValue_t value;
    int i, rc;

    if((err = rbusTable_addRow(handle, "Device.TestProvider.Table1.", "method1", &instNum1)) != RBUS_ERROR_SUCCESS)
    {
        TEST(0);
        goto exit1;
    }

    snprintf(row1, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Table1.%u", instNum1);

    rc = snprintf(table2, RBUS_MAX_NAME_LENGTH, "%s.Table2.", row1);
    if(rc >= RBUS_MAX_NAME_LENGTH)
        printf("Format Truncation error at table2 - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);

    if((err = rbusTable_addRow(handle, table2, "method2", &instNum2)) != RBUS_ERROR_SUCCESS)
    {
        TEST(0);
        goto exit1;
    }

    snprintf(row2, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Table1.%u.Table2.%u", instNum1, instNum2);

    snprintf(method11, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Method11()");
    snprintf(method3, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Method3()");
    snprintf(method1, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Table1.%u.Method1()", instNum1);
    snprintf(method2, RBUS_MAX_NAME_LENGTH, "Device.TestProvider.Table1.[method1].Table2.[method2].Method2()");

    rbusObject_Init(&inParams, NULL);

    rbusValue_Init(&value);
    rbusValue_SetString(value, "param1");
    rbusObject_SetValue(inParams, "param1", value);
    rbusValue_Release(value);

    /*
     * Call method Device.TestProvider.Table1.Method1()
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", method1);
    err = rbusMethod_Invoke(handle, method1, inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", method1,
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    if(err == RBUS_ERROR_SUCCESS)
    {
        testOutParams(outParams, "Method1()");
        rbusObject_Release(outParams);
    }

    /*
     * Call method Device.TestProvider.Method11()
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", method11);
    err = rbusMethod_Invoke(handle, method11, inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", method11,
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err != RBUS_ERROR_SUCCESS);
    if(err != RBUS_ERROR_SUCCESS)
    {
        printf("Print outparams\n");
        rbusObject_fwrite(outParams, 1, stdout);
        rbusObject_Release(outParams);
    }

    /*
     * Call method Device.TestProvider.Method3()
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", method3);
    err = rbusMethod_Invoke(handle, method3, inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", method3,
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err != RBUS_ERROR_SUCCESS);
    if(err != RBUS_ERROR_SUCCESS)
    {
        printf("Print outparams\n");
        testOutParams(outParams, "Method3()");
        rbusObject_Release(outParams);
    }

    /*
     * Call method Device.TestProvider.Table1.[methods].Table2.[methods].Method2()
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", method2);
    err = rbusMethod_Invoke(handle, method2, inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", method2,
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    if(err == RBUS_ERROR_SUCCESS)
    {
        testOutParams(outParams, "Method2()");
        rbusObject_Release(outParams);
    }

    /*
     * Call method Device.TestProvider.MethodAsync1() with blocking rbusMethod_Invoke
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", "Device.TestProvider.MethodAsync1()");
    err = rbusMethod_Invoke(handle, "Device.TestProvider.MethodAsync1()", inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", "Device.TestProvider.MethodAsync1()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    if(err == RBUS_ERROR_SUCCESS)
    {
        testOutParams(outParams, "MethodAsync1()");
        rbusObject_Release(outParams);
    }

    /*
     * Call method Device.TestProvider.MethodAsync1() with non-blocking rbusMethod_InvokeAsync, default timeout
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s, 0) \n#\n", "Device.TestProvider.MethodAsync1()");
    asyncCalled = false;
    asyncError = RBUS_ERROR_SUCCESS;
    err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.MethodAsync1()", inParams, asyncMethodHandler1, 0);
    printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.MethodAsync1()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    sleep(5);
    TEST(asyncCalled == true);

    /*
     * Call method Device.TestProvider.MethodAsync1() with non-blocking rbusMethod_InvokeAsync, with timeout
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s, 2) \n#\n", "Device.TestProvider.MethodAsync1()");
    asyncCalled = false;
    asyncError = RBUS_ERROR_TIMEOUT;
    err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.MethodAsync1()", inParams, asyncMethodHandler1, 2);
    printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.MethodAsync1()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    sleep(5);
    TEST(asyncCalled == true);


    /*
     * Call method Device.TestProvider.Table1.MethodAsync2() once
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s, 0) \n#\n", "Device.TestProvider.Table1.[method1].MethodAsync2()");
    asyncCount = 0;
    asyncError = RBUS_ERROR_SUCCESS;
    err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.Table1.[method1].MethodAsync2()", inParams, asyncMethodHandler2, 0);
    printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.Table1.[method1].MethodAsync2()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    sleep(5);
    TEST(asyncCount == 1);

    /*
     * Call method Device.TestProvider.Table1.MethodAsync3() once
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s, 0) \n#\n", "Device.TestProvider.MethodAsync3()");
    asyncCount = 0;
    asyncError = RBUS_ERROR_SUCCESS;
    err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.MethodAsync3()", inParams, asyncMethodHandler3, 0);
    printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.MethodAsync3()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    sleep(5);
    TEST(asyncCount == 1);

    /*
     * Call method Device.TestProvider.Table1.MethodAsync2() 5 times in parallel
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s, 0) \n#\n", "Device.TestProvider.Table1.[method1].MethodAsync2()");
    asyncCount = 0;
    for(i = 0; i < 5; ++i)
    {
        /* Note that rbusMethod_InvokeAsync does not copy inParams.  It passes inParams to a background thread and calls rbusMethod_Invoke
           on that thread.  This means we cannot reused inParams when running async methods in parallel.   So we create a unique
           rbusObject_t for each async invoke.
        */
        rbusObject_t inParams2;
        rbusObject_Init(&inParams2, NULL);
        asyncError = RBUS_ERROR_SUCCESS;
        rbusValue_Init(&value);
        rbusValue_SetInt32(value, i);
        rbusObject_SetValue(inParams2, "param1", value);
        rbusValue_Release(value);
        err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.Table1.[method1].MethodAsync2()", inParams2, asyncMethodHandler2, 0);
        printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.Table1.[method1].MethodAsync2()",
            err == RBUS_ERROR_SUCCESS ? "success" : "fail");
        TEST(err == RBUS_ERROR_SUCCESS);
        rbusObject_Release(inParams2);
    }
    sleep(5);
    TEST(asyncCount == 5);

    /*
     *  Call non-existing method with Invoke
     */
    printf("\n##########################################\n# TEST rbusMethod_Invoke(%s) \n#\n", "Device.TestProvider.ShouldNotExist()");
    err = rbusMethod_Invoke(handle, "Device.TestProvider.ShouldNotExist()", inParams, &outParams);
    printf("consumer: rbusMethod_Invoke(%s) %s\n", "Device.TestProvider.ShouldNotExist()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err != RBUS_ERROR_SUCCESS);

    /*
     *  Call non-existing method with InvokeAsync
     */
    printf("\n##########################################\n# TEST rbusMethod_InvokeAsync(%s) \n#\n", "Device.TestProvider.ShouldNotExist()");
    asyncCalled = false;
    asyncError = RBUS_ERROR_DESTINATION_NOT_FOUND;
    err = rbusMethod_InvokeAsync(handle, "Device.TestProvider.ShouldNotExist()", inParams, asyncMethodHandler1, 0);
    printf("consumer: rbusMethod_InvokeAsync(%s) %s\n", "Device.TestProvider.ShouldNotExist()",
        err == RBUS_ERROR_SUCCESS ? "success" : "fail");
    TEST(err == RBUS_ERROR_SUCCESS);
    sleep(1);
    TEST(asyncCalled == true);

    rbusObject_Release(inParams);

    if(outParams)
       rbusObject_Release(outParams);
    /*cleanup*/
    rbusTable_removeRow(handle, row2);
    rbusTable_removeRow(handle, row1);

   goto exit1;

exit1:
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("_test_Methods");
}
