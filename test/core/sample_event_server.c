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
#include <string.h>
#include "rbus_core.h"

#include "rtLog.h"

static char data1[100] = "obj1 init init init";
static char data2[100] = "obj2 init init init";

static int handle_get(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) hdr;
    rbusMessage_Init(response);
    printf("%s::%s %s, ptr %p\n", destination, method, (const char *)user_data, user_data);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, (const char *)user_data);
    return 0;
}

static int handle_set(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) hdr;
    rtError err = RT_OK;
    const char * payload = NULL;
    printf("calling set %s\n", (const char *)user_data);
    printf("%s::%s %s, \n", destination, method, (const char *)user_data);
    if((err = rbusMessage_GetString(request, &payload) == RT_OK)) //TODO: Should payload be freed?
    {
        strncpy((char *)user_data, payload, sizeof(data1));
    }
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}

static int callback(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) response;
    (void) destination;
    (void) method;
    (void) hdr;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);
    return 0;
}

static int sub_callback(const char * object,  const char * event, const char * listener, int added, const rbusMessage filter, void* data)
{
    (void)filter;
    printf("Received sub_callback object=%s event=%s listerner=%s added=%d data=%p\n", object, event, listener, added, data);
    return 0;
}

#define OBJ1_NAME "foo"
#define OBJ2_NAME "bar"

int main(int argc, char *argv[])
{
    (void) argc;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: sample_server <server object name>\n");
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection(argv[1])) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }


    if((err = rbuscore_registerObj(OBJ1_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
    if((err = rbuscore_registerObj(OBJ2_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)data1, handle_set}, {METHOD_GETPARAMETERVALUES, (void *)data1, handle_get}};
    rbuscore_registerMethodTable(OBJ1_NAME, table, 2);
    table[0].user_data = (void *)data2;
    table[1].user_data = (void *)data2;
    rbuscore_registerMethodTable(OBJ2_NAME, table, 2);
    rbuscore_addElement(OBJ1_NAME, "obj1_alias");
    rbuscore_addElement(OBJ1_NAME, "obj1_alias2");

    rbuscore_registerEvent(OBJ1_NAME, "event1", NULL, NULL);
    rbuscore_registerEvent(OBJ1_NAME, "event1", NULL, NULL); //Negative test.
    rbuscore_registerEvent(OBJ1_NAME, "event2", NULL, NULL);
    rbuscore_registerEvent(OBJ2_NAME, NULL, NULL, NULL); //Test with empty event name. This is equivalent to the school of thought that "object name == event"

    //support rbus events being elements
    rbuscore_addElement(OBJ1_NAME, "event4");
    char data4[] = "data4";
    rbuscore_registerEvent(OBJ1_NAME, "event4", sub_callback, data4);

    {
        //Run some negative test cases first.
        rbusMessage msg3;
        rbusMessage_Init(&msg3);
        rbusMessage_SetString(msg3, "efgh");
        rbuscore_publishEvent(OBJ2_NAME, "non_event", msg3);
        rbuscore_publishEvent("non_object", "event3", msg3);
        rbusMessage_Release(msg3);
    }

    int i = 0;
    for(;;)
    {
        rbusMessage msg1, msg2, msg4;
        rbusMessage_Init(&msg1);
        rbusMessage_Init(&msg2);
        rbusMessage_Init(&msg4);
        rbusMessage_SetString(msg1, "bar");
        rbusMessage_SetString(msg2, "beef");
        rbusMessage_SetString(msg4, "two");
        rbuscore_publishEvent(OBJ1_NAME, "event1", msg1);
        rbuscore_publishEvent(OBJ1_NAME, "event2", msg2);
        rbuscore_publishEvent(OBJ1_NAME, "event4", msg4);
        rbusMessage_Release(msg1);
        rbusMessage_Release(msg2);
        rbusMessage_Release(msg4);
        i++; 
        if(i < 10)
        {
            rbusMessage msg3;
            rbusMessage_Init(&msg3);
            rbusMessage_SetString(msg3, "efgh");
            rbuscore_publishEvent(OBJ2_NAME, NULL, msg3);
            rbusMessage_Release(msg3);
        }
        if(10 == i)
        {
            rbuscore_unregisterEvent(OBJ2_NAME, NULL);
            rbuscore_unregisterObj(OBJ2_NAME);
        }
        sleep(4);
    }
    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
