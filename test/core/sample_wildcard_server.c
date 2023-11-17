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
static char data2[100] = "tr69 init init init";

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

#define OBJ1_NAME "obj1"
#define OBJ2_NAME "obj2"
#define OBJ3_NAME "obj22"

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


    if((err = rbuscore_registerObj(OBJ1_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
    if((err = rbuscore_registerObj(OBJ2_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }

    if((err = rbuscore_registerObj(OBJ3_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
    {
        printf("Successfully registered object.\n");
    }
    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)data1, handle_set}, {METHOD_GETPARAMETERVALUES, (void *)data1, handle_get}};
    rbuscore_registerMethodTable(OBJ1_NAME, table, 2);
    table[0].user_data = (void *)data2;
    table[1].user_data = (void *)data2;
    rbuscore_registerMethodTable(OBJ2_NAME, table, 2);
    rbuscore_addElement(OBJ1_NAME, "device.wifi.a");
    rbuscore_addElement(OBJ1_NAME, "device.wifi.aa");
    rbuscore_addElement(OBJ1_NAME, "device.wifi.ab");
    rbuscore_addElement(OBJ1_NAME, "device.wifi.abb");
    rbuscore_addElement(OBJ1_NAME, "device.wifi.b");
    rbuscore_addElement(OBJ1_NAME, "device.wifi.x.y");
    rbuscore_addElement(OBJ2_NAME, "device.tr69.x.y");
    rbuscore_addElement(OBJ2_NAME, "device.wifii.a");
    rbuscore_addElement(OBJ2_NAME, "device.tr69.x.z");

    pause();

    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }
    return 0;
}
