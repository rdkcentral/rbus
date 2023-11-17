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

static char buffer[100];

static char data[100] = "init init init";

static int handle_get(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) message;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, data);
    return 0;
}

static int handle_set(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rtError err = RT_OK;
    const char * payload = NULL;
    if((err = rbusMessage_GetString(request, &payload) == RT_OK)) 
    {
        strncpy(data, payload, sizeof(data));
    }
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}

static void handle_unknown(const char * destination, const char * method, rbusMessage message, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) message;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
}

static int callback(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);

    /* Craft response message.*/
    handle_unknown(destination, method, message, response, hdr);
    return 0;
}

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

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", argv[1]);
    printf("Registering object %s\n", buffer);

    if((err = rbuscore_registerObj(buffer, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set}, {METHOD_GETPARAMETERVALUES, NULL, handle_get}};
    rbuscore_registerMethodTable(buffer, table, 2); 
    buffer[0] = 1;
    if((err = rbuscore_registerObj(buffer, callback, NULL)) == RBUSCORE_SUCCESS) //Negative test case
    {
        printf("Successfully registered object.\n");
    }
    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
