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

static char data1[100] = "wifi init init init";

static int handle_get(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) destination;
    (void) method;
    (void) request;
    (void) hdr;
    rbusMessage_Init(response);
    printf("calling get %s, ptr %p\n", (const char *)user_data, user_data);
    rbusMessage_SetInt32(*response,  RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response,  (const char *)user_data);
    return 0;
}

static int handle_set(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) destination;
    (void) method;
    (void) hdr;
    rtError err = RT_OK;
    const char * payload = NULL;
    printf("calling set %s\n", (const char *)user_data);
    if((err = rbusMessage_GetString(request,  &payload) == RT_OK)) //TODO: Should payload be freed?
    {
        strncpy((char *)user_data, payload, sizeof(data1));
    }
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response,  RBUSCORE_SUCCESS);
    return 0;
}

static int callback(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) destination;
    (void) method;
    (void) user_data;
    (void) response;
    (void) hdr;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);
    return 0;
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection("wildcard_server")) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }


    if((err = rbuscore_registerObj("foo", callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
    if((err = rbuscore_registerObj("bar", callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)data1, handle_set}, {METHOD_GETPARAMETERVALUES, (void *)data1, handle_get}};
    rbuscore_registerMethodTable("foo", table, 2);
    rbuscore_registerMethodTable("bar", table, 2);
    rbuscore_addElement("foo", "footree.1");
    rbuscore_addElement("foo", "footree.2");
    rbuscore_addElement("foo", "footree.3");
    rbuscore_addElement("foo", "footree.4");
    rbuscore_addElement("foo", "footree.5");

    rbuscore_addElement("bar", "bartree.1");
    rbuscore_addElement("bar", "bartree.2");
    rbuscore_addElement("bar", "bartree.3");
    rbuscore_addElement("bar", "bartree.4");
    rbuscore_addElement("bar", "bartree.5");
    
    printf("Press enter to remove object foo.\n");
    getchar();
    rbuscore_unregisterObj("foo");
    
    printf("Press enter to remove some elements of bar.\n");
    getchar();
    rbuscore_removeElement("bar", "bartree.2");
    rbuscore_removeElement("bar", "bartree.3");
    
    printf("Press enter to test some negative cases.\n");
    getchar();
    rbuscore_removeElement("bar", "bartree.3");
    rbuscore_unregisterObj("foo");
    
    printf("Press enter to remove bar as well.\n");
    getchar();
    rbuscore_unregisterObj("bar");
    
    printf("Press enter to disconnect and exit.\n");
    getchar();
    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
