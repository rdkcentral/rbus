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
    rbusMessage_SetString(*response, destination);
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
    (void) argv;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rtLog_SetLevel(RT_LOG_INFO);

    if((err = rbuscore_openBrokerConnection("obj_lookup")) == RBUSCORE_SUCCESS)
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

    rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, NULL, handle_get}};
    rbuscore_registerMethodTable("foo", table, 1); 
    rbuscore_registerMethodTable("bar", table, 1);

    rbuscore_addElement("foo", "foox.element1");
    rbuscore_addElement("foo", "foox.element2");
    rbuscore_addElement("bar", "barx.element2");
    rbuscore_addElement("bar", "barx.element1");
    rbuscore_addElement("foo", "common.element1");
    rbuscore_addElement("bar", "common.element2");

/* Needed to test the rtmessage not the rbuscore */
#if 0
    const int in_length = 12;
    const char *inputs[] = {"foo", "foox.element1", "foox.element2", "bar", "barx.element1", "barx.element2", "abcd", "foox.", "barx.", "common.element1", "common.element2", "common."};
    char **output = NULL;
    if(RBUSCORE_SUCCESS == rbuscore_discoverElementObjects(inputs, in_length, &output))
    {
        printf("Multi-lookup returned success. Printing mapping information...\n");
        for(int i = 0; i < in_length; i++)
        {
            printf("%s mapped to %s\n", inputs[i], output[i]);
            free(output[i]);
        }
        free(output);
    }
#endif

    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
