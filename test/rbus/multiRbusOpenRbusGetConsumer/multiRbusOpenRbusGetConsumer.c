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
#include <testValueHelper.h>

int main(int argc, char *argv[])
{
    rbusHandle_t handle1;
    rbusHandle_t handle2;
    int rc1 = RBUS_ERROR_SUCCESS;
    int rc2 = RBUS_ERROR_SUCCESS;

    (void)(argc);
    (void)(argv);

    printf("constumer: start\n");

    rc1 = rbus_open(&handle1, "multiRbusOpenRbusGetConsumer");
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: First rbus_open for handle1 err: %d\n", rc1);
        goto exit1;
    }

    rc2 = rbus_open(&handle2, "multiRbusOpenRbusGetConsumer");
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: Second rbus_open for handle2 err: %d\n", rc2);
        goto exit2;
    }

    rbusValue_t value;
    TestValueProperty data = { RBUS_INT32, "Device.multiRbusOpenGetTestProvider.Value.RBUS_INT32", {NULL} };

    printf("#################### getINT32Value ######################\n");

    printf("_test_multiRbusOpen rbus_get %s\n", data.name);

    rc1 = rbus_get(handle1, data.name, &value);

    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("_test_Value rbus_get failed err:%d\n", rc1);
    }

    rc2 = rbus_get(handle2, data.name, &value);

    if(rc2 ==  RBUS_ERROR_SUCCESS)
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>rbus_get handle2 test success %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", data.name);
        rbusValue_Release(value);
    }
    else
    {
        printf("_test_Value rbus_get failed err:%d\n", rc2);
    }

    printf("consumer: multiRbusOpenRbusGetConsumer %s with handle2\n", 
             rc2 == RBUS_ERROR_SUCCESS ? "success" : "fail");

    sleep(5);

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
    printf("consumer: exit\n");
    return rc2;
}


