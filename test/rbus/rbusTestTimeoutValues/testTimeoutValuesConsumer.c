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
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>

int main(int argc, char *argv[])
{
    rbusHandle_t handle;
    int ret = RBUS_ERROR_SUCCESS;

    (void)(argc);
    (void)(argv);
    bool fail = false;
    printf("constumer: start\n");

    ret = rbus_open(&handle, "TestTimeoutValuesConsumer");
    if(ret != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open for handle err: %d\n", ret);
        goto exit;
    }
    sleep(5);
    /* Test GET Timeout Value*/
    rbusValue_t value;
    printf("Test Rbus GET Timeout value %s\n", "Device.TestTimeoutValues.Value.RBUS_INT32");
    
    rbusHandle_ConfigGetTimeout(handle, 2000);
    ret = rbus_get(handle, "Device.TestTimeoutValues.Value.RBUS_INT32", &value);
    
    if (ret != RBUS_ERROR_TIMEOUT)
    {

        printf("FAILED Test Rbus GET Timeout value for %s,  Expected : %d, Actual: %d\n", "Device.TestTimeoutValues.Value.RBUS_INT32", RBUS_ERROR_TIMEOUT, ret);
        fail = true;
        goto exit1;
    }
        
    rbusHandle_ConfigGetTimeout(handle, 0);
    ret = rbus_get(handle, "Device.TestTimeoutValues.Value.RBUS_INT32", &value);

    if(ret ==  RBUS_ERROR_SUCCESS)
    {
        rbusValue_Release(value);
        printf("SUCCESS Test Rbus GET Timeout value for %s \n", "Device.TestTimeoutValues.Value.RBUS_INT32");
    }
    else
    {
         printf("FAILED Test Rbus GET Timeout value for %s,  Expected : %d, Actual: %d\n", "Device.TestTimeoutValues.Value.RBUS_INT32", RBUS_ERROR_SUCCESS, ret);
        fail = true;
        goto exit1; 
    }


    /* Test SET Timeout Value*/

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, 29);
    rbusHandle_ConfigSetTimeout(handle, 2000);
    
    ret = rbus_set(handle, "Device.TestTimeoutValues.Value.RBUS_INT32", value, NULL);
    if (ret != RBUS_ERROR_TIMEOUT)
    {

        printf("FAILED Test Rbus SET Timeout value for %s,  Expected : %d, Actual: %d\n", "Device.TestTimeoutValues.Value.RBUS_INT32",               RBUS_ERROR_TIMEOUT, ret);
        fail = true;
        goto exit1;
    }

    rbusHandle_ConfigSetTimeout(handle, 0);
    ret = rbus_set(handle, "Device.TestTimeoutValues.Value.RBUS_INT32", value, NULL);
    rbusValue_Release(value);
    if(ret !=  RBUS_ERROR_SUCCESS)
    {
         printf("FAILED Test Rbus SET Timeout value for %s,  Expected : %d, Actual: %d\n", "Device.TestTimeoutValues.Value.RBUS_INT32", RBUS_ERROR_SUCCESS, ret);
        fail = true;
        goto exit1; 
    }
    else
    {
        printf("SUCCESS Test Rbus SET Timeout value for %s \n", "Device.TestTimeoutValues.Value.RBUS_INT32");
    }

    sleep(5);
    printf("consumer: rbus TestTimeout Values %s \n", 
             ret == RBUS_ERROR_SUCCESS ? "success" : "fail");
exit1:
    ret = rbus_close(handle);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_close handle err: %d\n", ret);
    }
exit:    
    if (fail)
        exit(EXIT_FAILURE);
    printf("consumer: exit\n");
    return ret;
}
