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
#include <unistd.h>
#include "rbus_core.h"

#include "rtLog.h"

#define OBJ1_NAME "foo"
#define OBJ2_NAME "bar"

static void dumpMessage(rbusMessage message)
{
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("dumpMessage: %.*s\n", buff_length, buff);
    free(buff);
}

static int event_callback(const char * object_name,  const char * event_name, rbusMessage message, void * user_data)
{
    (void) user_data;
    printf("In event callback for object %s, event %s.\n", object_name, event_name);
    dumpMessage(message);
    return 0;
}
int main(int argc, char *argv[])
{
    (void) argc;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: sample_client <name of client instance> <destination object name>\n");
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection(argv[1])) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }

    printf("Registering event callback.\n");

    rbuscore_subscribeToEvent("non_object", "event_2", &event_callback, NULL, NULL, NULL); //Negative test case.
    rbuscore_subscribeToEvent("non_object", NULL, &event_callback, NULL, NULL, NULL); //Negative test case.
    rbuscore_subscribeToEvent(OBJ1_NAME, "event3", &event_callback, NULL, NULL, NULL); //Negative test case.
    rbuscore_subscribeToEvent(OBJ1_NAME, "event1", &event_callback, NULL, NULL, NULL);
    rbuscore_subscribeToEvent(OBJ1_NAME, "event1", &event_callback, NULL, NULL, NULL); //Negative test case.
    rbuscore_subscribeToEvent(OBJ1_NAME, "event2", &event_callback, NULL, NULL, NULL);
    rbuscore_subscribeToEvent(OBJ2_NAME, NULL, &event_callback, NULL, NULL, NULL);

    //support rbus events being elements
    rbuscore_subscribeToEvent(NULL, "event4", &event_callback, NULL, NULL, NULL);

    sleep(10);
    rbuscore_unsubscribeFromEvent(OBJ1_NAME, "event2", NULL);
    sleep(1);
#if 1
    /*Pull the object from remote end.*/
    rbusMessage response;
    if((err = rbuscore_pullObj(OBJ1_NAME, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", OBJ1_NAME);
        rbusMessage_GetString(response, &buff);
        printf("Payload: %s\n", buff);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", OBJ1_NAME);
    }

    //Check whether aliases work.

    if((err = rbuscore_pullObj("obj1_alias", 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", "obj1_alias");
        rbusMessage_GetString(response, &buff);
        printf("Payload: %s\n", buff);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", OBJ1_NAME);
    }
    if((err = rbuscore_pullObj("obj1_alias2", 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", "obj1_alias2");
        rbusMessage_GetString(response, &buff);
        printf("Payload: %s\n", buff);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", OBJ1_NAME);
    }
    
    if((err = rbuscore_pullObj(OBJ2_NAME, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", OBJ2_NAME);
        rbusMessage_GetString(response, &buff);
        printf("Payload: %s\n", buff);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", OBJ1_NAME);
    }

    /* Push the object to remote end.*/
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, "foobar");
    if((err = rbuscore_pushObj(OBJ1_NAME, setter, 1000)) == RBUSCORE_SUCCESS)
    {
        printf("Push object %s\n", OBJ1_NAME);
    }
    else
    {
        printf("Could not push object %s. Error: 0x%x\n", OBJ1_NAME, err);
    }

    /* Pull again to make sure that "set" worked. */
    if((err = rbuscore_pullObj(OBJ1_NAME, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        printf("Received object %s\n", OBJ1_NAME);
        rbusMessage_GetString(response, &buff);
        printf("Payload: %s\n", buff);
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", OBJ1_NAME);
    }
#endif
    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
