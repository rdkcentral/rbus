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

//TODO handle filter matching

int runtime = 70;

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool*     autoPublish)
 {
     (void)handle;
     (void)filter;
     (void)interval;

     printf(
             "eventSubHandler called:\n" \
             "\taction=%s\n" \
             "\teventName=%s\n",
             action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
             eventName);

     *autoPublish = true;
     return RBUS_ERROR_SUCCESS;
 }

rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)opts;

    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata = 0;  /*the actual value to send back*/
    static int32_t mydelta = 1; /*how much to change the value by*/
    static int32_t mycount = 0; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=7; /*keep value between mymin and mymax*/

    rbusValue_t value;

    mycount++;

    if((mycount % myfreq) == 0) 
    {
        mydata += mydelta;
        if(mydata == mymax)
            mydelta = -1;
        else if(mydata == mymin)
            mydelta = 1;
    }

    printf("Called get handler for [%s] val=[%d]\n", name, mydata);

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, mydata);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle1 = NULL;
    rbusHandle_t handle2 = NULL;
    int rc1 = RBUS_ERROR_SUCCESS;
    int rc2 = RBUS_ERROR_SUCCESS;

    char componentName[] = "multiRbusOpenProvider";

    rbusDataElement_t dataElements[1] = {{"Device.Provider1.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, eventSubHandler, NULL}}};

    printf("provider: start\n");
    rc1 = rbus_open(&handle1, componentName);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: First rbus_open handle1 err: %d\n", rc1);
        goto exit1;
    }

    printf("provider0: handle1:%p: %d\n", (void*)handle1,rc1);
    rc2 = rbus_open(&handle2, componentName);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: Second rbus_open handle2 err: %d\n", rc2);
        goto exit2;
    }

    printf("provider1: handle1:%p: %d\n", (void*)handle1,rc1);
    rc1 = rbus_regDataElements(handle1, 1, dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_regDataElements handle1 err: %d\n", rc1);
    }
    rc2 = rbus_regDataElements(handle2, 1, dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements handle2 err: %d\n", rc2);
    }

    sleep(runtime);

    rc1 = rbus_unregDataElements(handle1, 1, dataElements);
    if(rc1 != RBUS_ERROR_INVALID_HANDLE)
    {
        printf("provider: rbus_unregDataElements handle1 err: %d\n", rc1);
    }

    rc2 = rbus_unregDataElements(handle2, 1, dataElements);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_unregDataElements handle2 err: %d\n", rc2);
    }

   rc2 = rbus_close(handle2);
   if(rc2 != RBUS_ERROR_SUCCESS)
   {
      printf("provider: rbus_close handle2 err: %d\n", rc2);
   }
exit2:
   rc1 = rbus_close(handle1);
   if(rc1 != RBUS_ERROR_INVALID_HANDLE)
   {
      printf("provider: rbus_close handle1 err: %d\n", rc1);
   }
exit1:
   printf("provider: exit with :%d and %d\n",rc1,rc2);
   return rc2;
}
