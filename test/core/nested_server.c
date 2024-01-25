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

static char data[100] = "init init init";
static const char * object_name;
static const char * nested_object_name;
static int handle_get(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) message;
    (void) hdr;
    printf("Handling %s call for %s\n", method, destination);
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, data);

    if(0 == strncmp(object_name, destination, strlen(object_name)))
    {
        /* Make the nested call */
        rbusMessage nestedResponse;
        if(RBUSCORE_SUCCESS != rbuscore_invokeRemoteMethod(nested_object_name, METHOD_GETPARAMETERVALUES,
                    NULL, 1000, &nestedResponse))
        {
            printf("Nested call failed.\n");
        }
        else
        {
            printf("Nested call returned.\n");
            rbusMessage_Release(nestedResponse);
        }
    }
    printf("Exiting %s call for %s\n", method, destination);
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
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: rbus_nested_test_server <top level object> <nested object>\n");
    if(3 > argc)
        return 1;
    object_name = argv[1];
    nested_object_name = argv[2];

    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection(argv[1])) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }

    if((err = rbuscore_registerObj(object_name, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }

    if((err = rbuscore_registerObj(nested_object_name, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
    rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, NULL, handle_get}};
    rbuscore_registerMethodTable(object_name, table, 1); 
    rbuscore_registerMethodTable(nested_object_name, table, 1); 
    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
