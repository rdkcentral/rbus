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
/*****************************************
Test server for unit test client testing
******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "rbuscore.h"

#include "rbus_test_util.h"


static char data[100] = "init init init";

static test_struct_t test_struct1 = {{0},0,0};
static test_struct_t test_struct2 = {{0},0,0};
static test_array_data_t test_array1 = {{0},0};

static int binary_data_size = 0;

student_details_t student_data[100] = {{{0},{0}}};
int count = 0;

void reset_stored_data()
{
    memset(data, 0, sizeof(data));
    strncpy(data, "init init init", sizeof(data));

    memset(&test_struct1, 0, sizeof(test_struct_t));
    memset(&test_struct2, 0, sizeof(test_struct_t));

    for(int i = 0; i < count; i++)
        memset((student_data + i), 0, sizeof(student_details_t));
    count = 0;
}

int handle_get1(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, data);
    return 0;
}

int handle_set1(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) response;
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

int handle_get2(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    printf("%s::%s %s, ptr\n", destination, method, (const char *)user_data);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, (const char *)user_data);
    return 0;
}

int handle_recursive_get(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    static const int reference_value = 0xabcd;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    const char * level_2_object = NULL;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rbusMessage level_2_response;
    rbusMessage_Init(response);
    if(RBUSCORE_SUCCESS == rbusMessage_GetString(request, &level_2_object) && (0 < strlen(level_2_object)))
    {
        rbusMessage outbound;
        rbusMessage_Init(&outbound);
        rbusMessage_SetString(outbound, "");
        ret = rbuscore_invokeRemoteMethod(level_2_object, METHOD_GETPARAMETERVALUES, outbound, 1000, &level_2_response);
        int result;
        int payload = 0;
        if(RBUSCORE_SUCCESS == ret )
        {
            if((RBUSCORE_SUCCESS == rbusMessage_GetInt32(level_2_response, &result)) && 
                    (RBUSCORE_SUCCESS == rbusMessage_GetInt32(level_2_response, &payload)) && 
                    (payload == reference_value))
            {
                ret = RBUSCORE_SUCCESS;
            }
            else
                ret = RBUSCORE_ERROR_GENERAL;
            rbusMessage_Release(level_2_response);
        }
        rbusMessage_SetInt32(*response, ret);
    }
    else
    {
        rbusMessage_SetInt32(*response, ret);
        rbusMessage_SetInt32(*response, reference_value);
    }
    return 0;
}
int handle_set2(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) response;
    (void) destination;
    (void) method;
    (void) hdr;
    rtError err = RT_OK;
    const char * payload = NULL;
    printf("calling set %s\n", (const char *)user_data);
    printf("%s::%s %s, \n", destination, method, (const char *)user_data);
    if((err = rbusMessage_GetString(request, &payload) == RT_OK))
    {
        strncpy((char *)user_data, payload, sizeof(data));
    }
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}
int handle_getStudentInfo(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    int i;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    for(i = 0; i <count; i++)
    {
       if(0 == strncmp(student_data[i].object_name, (char *)user_data, 50))
          rbusMessage_SetString(*response, student_data[i].student_name);
    }

    return 0;
}

int handle_setStudentInfo(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) response;
    (void) destination;
    (void) method;
    (void) hdr;
    rtError err = RT_OK;
    const char * payload = NULL;

    if(NULL != user_data)
    {
        strncpy(student_data[count].object_name, (char *)user_data,sizeof(student_data[count].object_name));
    }
    if((err = rbusMessage_GetString(request, &payload) == RT_OK))
    {
        strncpy(student_data[count].student_name, payload, 50);
    }
    count++;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}

int handle_getBinaryData(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetBytes(*response, (void *)&test_array1, sizeof(test_array1));
    return 0;
}

int handle_setBinaryDataSize(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;

    rbusMessage_GetInt32(request, &binary_data_size);

    printf("Value set for binary data size : %d \n", binary_data_size);

    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}

int handle_getLargeBinaryData(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    uint8_t data[1024] = {0};
    uint8_t i = 0;
    char chunk_name[10] = {0};

    rbusMessage_Init(response);
    for(i = 1; i <= binary_data_size; i++)
    {
        snprintf(chunk_name, 10, "chunk%u", i);
        memset(data, (i + '0'), sizeof(data));
        rbusMessage_SetBytes(*response, data, sizeof(data));
    }
    return 0;
}

int handle_setBinaryData(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rtError err = RT_OK;
    const test_array_data_t *payload;
    unsigned int size = 0;

    if((err = rbusMessage_GetBytes(request, (const uint8_t**)&payload, &size) == RT_OK))
    {
        test_array1 = *payload;
    }
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    return 0;
}

int handle_getAttributes1(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    //rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, test_struct1.name);
    rbusMessage_SetInt32(*response, test_struct1.age);
    rbusMessage_SetString(*response, data);
    return 0;
}
int handle_setAttributes1(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, data);
    rtError err = RT_OK;
    const char * name = NULL;
    if((err = rbusMessage_GetString(request, &name) == RT_OK))
    {
        strncpy(test_struct1.name, name, sizeof(test_struct1.name));
        //printf("Value set to name: %s \n", name);
    }
    rbusMessage_GetInt32(request, &test_struct1.age);
    return 0;
}

int handle_getAttributes2(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    //rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, test_struct2.name);
    rbusMessage_SetInt32(*response, test_struct2.age);
    rbusMessage_SetString(*response, data);
    return 0;
}

int handle_timeout(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) destination;
    (void) method;
    (void) user_data;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetString(*response, "time out success");
    rtError err = RT_OK;

    const char * test_name = NULL;
    int time_out = 0;

    if((err = rbusMessage_GetString(request, &test_name) == RT_OK))
    {
        rbusMessage_GetInt32(request, &time_out);
        //printf(" Test: %s starting a sleep of %d seconds \n", test_name, time_out);
        sleep(time_out);
        //printf(" Test: returning after sleep \n");
    }
    return 0;
}

int handle_setAttributes2(const char * destination, const char * method, rbusMessage request, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_SUCCESS);
    rbusMessage_SetString(*response, data);
    rtError err = RT_OK;
    const char * name = NULL;
    if((err = rbusMessage_GetString(request, &name) == RT_OK))
    {
        strncpy(test_struct2.name, name, sizeof(test_struct2.name));
        //printf("Value set to name: %s \n", name);
    }
    rbusMessage_GetInt32(request, &test_struct2.age);
    return 0;
}

void handle_unknown(const char * destination, const char * method, rbusMessage request, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) request;
    (void) destination;
    (void) method;
    (void) hdr;
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
}

int callback(const char * destination, const char * method, rbusMessage message, void * user_data, rbusMessage *response, const rtMessageHeader* hdr)
{
    (void) user_data;
    (void) hdr;
    printf("Received message in base callback for destination %s, method %s.\n", destination, method);
    fflush(stdout);
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);
    fflush(stdout);
    /* Craft response message.*/
    handle_unknown(destination, method, message, response, hdr);
    return 0;
}

int sub1_callback(const char * object,  const char * event, const char * listener, int added, const rbusMessage filter, void* data)
{
    (void)filter;
    printf("Received sub_callback object=%s event=%s listerner=%s added=%d data=%p\n", object, event, listener, added, data);
    fflush(stdout);
    return 0;
}
