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
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "rbus_core.h"
#include "rbus_types.h"
#include "rtLog.h"
#include <stdlib.h>
#include <unistd.h>


struct component
{
    std::string component_name;
    std::vector <std::string> leaves;

    component(std::string &name) : component_name(name) {}

    void print_summary()
    {
        printf("Component %s has %d leaves.\n", component_name.c_str(), (int)leaves.size());
    }
    void dump_data()
    {
        for(const auto &entry : leaves)
            printf("%s %s\n", entry.c_str(), component_name.c_str());
    }
};

std::vector <component *> ref_table;


static const int REFERENCE_VALUE = 0xabcd;
static int handle_get(const char * destination, const char * method, rtMessage message, void * user_data, rtMessage *response)
{
    (void) destination;
    (void) method;
    (void) user_data;
    (void) message;
    rtMessage_Create(response);
    //printf("calling get %s\n", destination);
    rbus_SetInt32(*response, MESSAGE_FIELD_RESULT, RBUSCORE_SUCCESS);
    rbus_SetInt32(*response, MESSAGE_FIELD_PAYLOAD, REFERENCE_VALUE);
    return 0;
}

static int callback(const char * destination, const char * method, rtMessage message, void * user_data, rtMessage *response)
{
    (void) destination;
    (void) method;
    (void) user_data;
    (void) response;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rtMessage_ToString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);
    return 0;
}

static int load_datamodel_from_file(const char * filename) 
{
    component * ptr = NULL;
    std::ifstream infile;
    infile.open(filename);
    if(infile.is_open())
    {
        std::string buffer;
        const std::string componentMarker("eRT");

        while(infile >> buffer)
        {
            if(0 == buffer.compare(0, componentMarker.size(), componentMarker))
            {
                ptr = new component(buffer);
                ref_table.push_back(ptr);
            }
            else if(NULL != ptr)
                ptr->leaves.push_back(buffer);
        }
        infile.close();

        /*Dump what we have so far.*/
        for(const auto &c_ptr : ref_table)
            c_ptr->print_summary();
        printf("Completed digesting data.\n");
#if 0
        for(const auto &c_ptr : ref_table)
            c_ptr->dump_data();
#endif
        return 0;
    }
    else
    {
        printf("Error. Couldn't open file %s.\n", filename);
        return 1;
    }
}

static void test_bus(int reps)
{
    rbusCoreError_t err;
    pid_t pid = -1;
    rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, NULL, handle_get}};
    for(const auto &c_ptr : ref_table)
    {
        pid = fork();
        if(0 == pid)
        {
            if((err = rbuscore_openBrokerConnection("loadserver")) == RBUSCORE_SUCCESS)
                printf("Successfully connected to bus.\n");
            
            if((err = rbuscore_registerObj(c_ptr->component_name.c_str(), callback, NULL)) == RBUSCORE_SUCCESS)
                printf("Successfully registered component %s.\n",c_ptr->component_name.c_str()); 
            
            rbuscore_registerMethodTable(c_ptr->component_name.c_str(), table, 1);
            for(const auto &leaf : c_ptr->leaves)
                rbuscore_addElement(c_ptr->component_name.c_str(), leaf.c_str());
            
            pause();
            if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
                printf("Successfully disconnected from bus.\n");
            break;
        }
    }

    if(0 < pid)
    {
        struct timeval start_time, end_time, diff;
        sleep(10);
        printf("Server applications are ready. Press enter to start testing.\n");
        getchar();
        //Test the registered datamodels from the launcher application.
        if((err = rbuscore_openBrokerConnection("loadclient")) == RBUSCORE_SUCCESS)
            printf("Successfully connected to bus.\n");
        gettimeofday(&start_time, NULL);
        while(0 < reps--)
        {
            for(const auto &c_ptr : ref_table)
            {
                rtMessage result;
                int value = 0;
                int op_result = 0;
                err = rbuscore_invokeRemoteMethod(c_ptr->component_name.c_str(), METHOD_GETPARAMETERVALUES, NULL, 1000, &result);
                if((RBUSCORE_SUCCESS != err) ||
                        (RT_OK != rbus_GetInt32(result, MESSAGE_FIELD_RESULT, &op_result)) ||
                        (RT_OK != rbus_GetInt32(result, MESSAGE_FIELD_PAYLOAD, &value)) || 
                        (value != REFERENCE_VALUE))
                {
                    printf("Test failed for %s\n", c_ptr->component_name.c_str());break;
                }
                for(const auto &leaf : c_ptr->leaves)
                {
                    value = 0;
                    err = rbuscore_invokeRemoteMethod(leaf.c_str(), METHOD_GETPARAMETERVALUES, NULL, 1000, &result);
                    if((RBUSCORE_SUCCESS != err) ||
                        (RT_OK != rbus_GetInt32(result, MESSAGE_FIELD_RESULT, &op_result)) ||
                        (RT_OK != rbus_GetInt32(result, MESSAGE_FIELD_PAYLOAD, &value)) || 
                        (value != REFERENCE_VALUE))
                    {
                        printf("Test failed for %s\n", leaf.c_str());break;
                    }
                }
            }
        }
        gettimeofday(&end_time, NULL);
        timersub(&end_time, &start_time, &diff);
        long long now = (diff.tv_sec * 1000000ll + diff.tv_usec);
        printf("Time consumed for test is %llu us\n", now);
        if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
            printf("Successfully disconnected from bus.\n");

    }
}

int main(int argc, char *argv[])
{
    printf("syntax: rbus_load_test <path to datamodel file> <number of repetitions>\n");
    rtLog_SetLevel(RT_LOG_INFO);

    if(3 > argc)
        return 1;

    char *endptr;
    long reps = strtol(argv[2], &endptr, 10);
    if((1 > reps) || (1000 < reps))
        reps = 1;
    load_datamodel_from_file(argv[1]);
    test_bus((int)reps);
    return 0;
}
