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

static rbusHandle_t handle;
static void eventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusValue_t someText;

    someText = rbusObject_GetValue(event->data, "someText");

    printf("Consumer receiver General event 1 %s\n", event->name);

    if(someText)
        printf("  someText: %s\n", rbusValue_GetString(someText, NULL));

    printf("  My user data: %s\n", (char*)subscription->userData);

    (void)handle;
}

static void* rbusThreadFunc (void *arg)
{
    char *param = (char *)arg;
    rbusValue_t val;
    int ret = 0;

    printf("%s param %s\n",__func__,param);
    if((strcmp(param,"rbus_obj_block") == 0) || (strcmp(param,"rbus_obj_nonblock") == 0))
    {
	ret = rbus_get(handle, param, &val);

	if(ret != RBUS_ERROR_SUCCESS)
	    printf("error in getting %s ret %d %s\n",param,ret,rbusError_ToString(ret));
	else {
	    char *p = rbusValue_ToString(val, NULL, 0);
	    printf("value op : p %s\n",p);
	    free(p);
	}
    } else if((strcmp(param,"rbus_event") == 0)) {
	ret = rbusEvent_Subscribe(handle, "rbus_event", eventHandler, NULL,  0);
	if(ret != RBUS_ERROR_SUCCESS)
	    printf("error in getting %s ret %d %s\n",param,ret,rbusError_ToString(ret));
	else
	    printf("sub success\n");
    }

    return arg;
}

int main(int argc, char *argv[])
{
    int rc;
    pthread_t tid1,tid2;

    (void)argc;
    (void)argv;

    printf("constumer: start\n");

    rc = rbus_open(&handle, "rbusSubConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
	printf("consumer: rbus_open failed: %d\n", rc);
	goto exit1;
    }
    rbus_setLogLevel(RBUS_LOG_DEBUG);
    pthread_create(&tid1, NULL, rbusThreadFunc, (void *)"rbus_obj_block");
    sleep(1);
    //pthread_create(&tid2, NULL, rbusThreadFunc, (void *)"rbus_obj_nonblock");
    pthread_create(&tid2, NULL, rbusThreadFunc, (void *)"rbus_event");

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    sleep(100);
    rbus_close(handle);
exit1:
    printf("constumer: exit\n");
    return 0;
}


