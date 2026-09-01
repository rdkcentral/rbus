
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
#include <rbus.h>
int runtime = 60;

static void eventReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void) handle;
    rbusValue_t value = rbusObject_GetValue(event->data, "value");

    printf("Consumer receiver 1st general event %s\n", event->name);

    if(value)
        printf("  Value: %s\n", rbusValue_GetString(value, NULL));

    printf("  My user data: %s\n", (char*)subscription->userData);
}

int main(int argc, char *argv[])
{
    int rc1 = RBUS_ERROR_SUCCESS;
    int rc2 = RBUS_ERROR_SUCCESS;
    const char* component1 = "multiRbusOpenEventConsumer";
    const char* component2 = "EventConsumer";
    rbusHandle_t handle1;
    rbusHandle_t handle2;
    rbusEventSubscription_t subscription = {"Device.Sample.InitialEvent1!", NULL, 0, 0, eventReceiveHandler, NULL, NULL, NULL, true};

    rc1 = rbus_open(&handle1, component1);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open handle1 failed: %d\n", rc1);
        goto exit1;
    }

    rc2 = rbus_open(&handle2, component2);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open handle2 failed: %d\n", rc2);
        goto exit2;
    }
    if(argc == 2 && (strcmp(argv[1],"1")==0))
    {
        rc2 = rbusEvent_SubscribeEx(
            handle2,
            &subscription,
            1,
            60);
	if(rc2 != RBUS_ERROR_SUCCESS)
	{
		printf("consumer: rbusEvent_Subscribe handle2 failed with err:%d\n", rc2);
	}
	else
		printf("consumer: Subscribed to Device.Sample.InitialEvent1! with handle2, component:%s\n", component2);

	sleep(1);
	goto exit;
    }
    if((argc == 2 && (strcmp(argv[1],"2")==0)) || argc == 1)
    {
	printf("handle1 and handle2 Subscribing to Device.Provider1.Event1!\n");
	rc1 = rbusEvent_SubscribeEx(
            handle1,
            &subscription,
            1,
            60);
	if(rc1 != RBUS_ERROR_SUCCESS)
	{
            printf("consumer: rbusEvent_Subscribe handle1 failed with err:%d\n", rc1);
	}
	else
            printf("consumer: Subscribed to Device.Sample.InitialEvent1! with handle1, component:%s\n", component1);
	sleep(1);
	rc2 = rbusEvent_SubscribeEx(
            handle2,
            &subscription,
            1,
            60);
	if(rc2 != RBUS_ERROR_SUCCESS)
	{
            printf("consumer: rbusEvent_Subscribe handle2 failed with err:%d\n", rc2);
	}
	else
            printf("consumer: Subscribed to Device.Sample.InitialEvent1! with handle2, component:%s\n", component2);

	sleep(1);
    }

    rc1 = rbusEvent_UnsubscribeEx(
            handle1,
	    &subscription,
            1);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("Unsubscribing handle1 err:%d\n", rc1);
    }
    else
        printf("consumer: UnSubscribed to Device.Sample.InitialEvent1! with handle1\n");
 
exit:
    rc2 = rbusEvent_UnsubscribeEx(
            handle2,
            &subscription,
	    1);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("Unsubscribing handle2 failed :%d\n", rc2);
    }
    else
        printf("consumer: UnSubscribed to Device.Sample.InitialEvent1! with handle2\n");

    rc2 = rbus_close(handle2);
    if(rc2 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_close handle2 err: %d\n", rc2);
    }

exit2:
    rc1 = rbus_close(handle1);
    if(rc1 != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_close handle1 failed: %d\n", rc1);
    }
exit1:
    return rc2;
}

