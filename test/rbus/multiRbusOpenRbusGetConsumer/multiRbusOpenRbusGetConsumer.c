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
    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;

    (void)(argc);
    (void)(argv);

    printf("constumer: start\n");

    rc = rbus_open(&handle, "multiRbusOpenRbusGetConsumer");
    rc = rbus_open(&handle, "multiRbusOpenRbusGetConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    //TestValueProperty* properties;
    rbusValue_t value;
    TestValueProperty data = { RBUS_INT32, "Device.multiRbusOpenGetTestProvider.Value.RBUS_INT32", {NULL} };

    printf("#################### getINT32Value ######################\n");

    printf("_test_multiRbusOpen rbus_get %s\n", data.name);

    rc = rbus_get(handle, data.name, &value);

    if(rc ==  RBUS_ERROR_SUCCESS)
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>> test success %s >>>>>>>>>>>>>>>>>>>>>>>>>\n", data.name);
        rbusValue_Release(value);
    }
    else
    {
        printf("_test_Value rbus_get failed err:%d\n", rc);
    }

    printf("consumer: multiRbusOpenRbusGetConsumer %s\n", 
    rc == RBUS_ERROR_SUCCESS ? "success" : "fail");

    sleep(5);


exit1:
    printf("consumer: exit\n");
    return rc;
}


