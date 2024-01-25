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
#include "rtConnection.h"

#include <time.h>
#include <sys/time.h>
#include <vector>
#include <atomic>
#include <algorithm>
#define CLOCK_TYPE CLOCK_MONOTONIC

static void timespec_sub(struct timespec * recent, struct timespec * old, struct timespec *difference)
{
    difference->tv_sec = recent->tv_sec - old->tv_sec;
    difference->tv_nsec = recent->tv_nsec - old->tv_nsec;
    if(0 > difference->tv_nsec)
    {
        difference->tv_sec--;
        difference->tv_nsec += 1000000000ll;
    }
}

typedef enum
{
    HYBRID = 0,
    SERVER,
    CLIENT
} op_mode_t;

static char * g_data;
static op_mode_t g_mode;
//Data board.
static std::vector <struct timespec> g_rpc_start;
static std::vector <struct timespec> g_rpc_end;
static std::vector <struct timespec> g_handler_start;
static std::vector <struct timespec> g_handler_end;
static std::atomic <int> g_index(0);

static long long convert_to_us(struct timespec &ts)
{
        return (ts.tv_sec * 1000000ll + ts.tv_nsec / 1000ll);
}

static void print_mean(const std::vector <long long> &end_to_end_us)
{
    long long total = 0;
    for(unsigned int i = 0; i < end_to_end_us.size(); i++)
    {
        total += end_to_end_us[i];
    }
    printf("Average end to end latency (%lld /  %d) =  %lld us\n", total, (int)end_to_end_us.size(), (total / end_to_end_us.size()));
}

static void print_median(std::vector <long long> &end_to_end_us)
{
    std::sort(end_to_end_us.begin(), end_to_end_us.end());
    if(0 == end_to_end_us.size() % 2)
    {
        int mid = end_to_end_us.size() / 2;
        printf("Median end to end latency: %lld us\n", (end_to_end_us[mid] + end_to_end_us[mid - 1]) / 2ll);
    }
    else
        printf("Median end to end latency: %lld us\n", end_to_end_us[end_to_end_us.size() / 2]);
}

static void print_analysis()
{
    printf("Raw data:\n");
    std::vector <long long> end_to_end_us;
    unsigned int num_entries = g_index.load();
    if(0 < num_entries)
    {
        printf("Dumping raw timestamps from all stages:\n");
        for(unsigned int i = 0; i < num_entries; i++)
        {
          printf("%lds %ldns -> %lds %ldns ->%lds %ldns ->%lds %ldns\n",
              g_rpc_start[i].tv_sec, g_rpc_start[i].tv_nsec,
              g_handler_start[i].tv_sec, g_handler_start[i].tv_nsec,
              g_handler_end[i].tv_sec, g_handler_end[i].tv_nsec,
              g_rpc_end[i].tv_sec, g_rpc_end[i].tv_nsec);
        }
        printf("End raw timestamps.\n");
        for(unsigned int i = 0; i < num_entries; i++)
        {
            struct timespec end_to_end, handler_latency, bus_latency_outbound, bus_latency_inbound;
            timespec_sub(&g_rpc_end[i], &g_rpc_start[i], &end_to_end);
            end_to_end_us.push_back(convert_to_us(end_to_end));
            timespec_sub(&g_handler_end[i], &g_handler_start[i], &handler_latency);
            timespec_sub(&g_handler_start[i], &g_rpc_start[i], &bus_latency_outbound);
            timespec_sub(&g_rpc_end[i], &g_handler_end[i], &bus_latency_inbound);
            printf("end to end: %lld us, handler latency: %lld, outbound bus latency: %lld, inbound bus latency: %lld \n",
                    convert_to_us(end_to_end), convert_to_us(handler_latency), convert_to_us(bus_latency_outbound), convert_to_us(bus_latency_inbound));
        }
        print_mean(end_to_end_us);
        print_median(end_to_end_us);
    }
}

static int handle_get(const char * destination, const char * method, rtMessage message, void * user_data, rtMessage *response)
{
    (void) destination;
    (void) user_data;
    (void) message;
    (void) method;
    int index = g_index.load();

    clock_gettime(CLOCK_TYPE, &g_handler_start[index]);
    rtMessage_Create(response);
    rbus_SetInt32(*response, MESSAGE_FIELD_RESULT, RBUSCORE_SUCCESS);
    rbus_SetString(*response, MESSAGE_FIELD_PAYLOAD, g_data);
    clock_gettime(CLOCK_TYPE, &g_handler_end[index]);

    if(SERVER == g_mode)
    {
        struct timespec diff;
        timespec_sub(&g_handler_end[index], &g_handler_start[index], &diff);
        printf("Handler latency: %lld us\n", convert_to_us(diff)); 
    }
    return 0;
}


static void handle_unknown(const char * destination, const char * method, rtMessage message, rtMessage *response)
{
    (void) message;
    (void) destination;
    (void) method;
    rtMessage_Create(response);
    rbus_SetInt32(*response, MESSAGE_FIELD_RESULT, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
}

static int callback(const char * destination, const char * method, rtMessage message, void * user_data, rtMessage *response)
{
    (void) user_data;
    (void) destination;
    (void) method;
    printf("Received message in base callback.\n");
    char* buff = NULL;
    uint32_t buff_length = 0;

    rtMessage_ToString(message, &buff, &buff_length);
    printf("%s\n", buff);
    free(buff);

    /* Craft response message.*/
    handle_unknown(destination, method, message, response);
    return 0;
}

#define APPLICATION_NAME "_rbus_perf"
#define OBJECT_NAME "_rbus_perf_server"
static const int MAX_DATA_SIZE = 1024 * 1024 * 10; 
static const int MIN_DATA_SIZE = 1;
static const int MIN_REPS = 1;
static const int MIN_SEPARATION = 0;

static void run_test(int reps, int separation)
{
    rbusCoreError_t err;
    for(int i = 0; i < reps; i++)
    {
        rtMessage result;
        int index = g_index.load();
        
        clock_gettime(CLOCK_TYPE, &g_rpc_start[index]);
        err = rbuscore_invokeRemoteMethod(OBJECT_NAME, METHOD_GETPARAMETERVALUES, NULL, 1000, &result);
        clock_gettime(CLOCK_TYPE, &g_rpc_end[index]);

        if(RBUSCORE_SUCCESS != err)
        {
            printf("RPC failed after %d runs.\n", i);
            break;
        }
        else
            rtMessage_Release(result);
        g_index++;
        if(0 != separation)
            usleep(1000 * separation);
    }
}

int main(int argc, char *argv[])
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("syntax: rbus_perf <server|client|hybrid> <size of data per transaction> <number of reps> <separation between transactions in milliseconds> <taint|clear> <broker_address>\n");
    if(6 > argc)
        return 1;

    /*Determine mode of operation.*/
    if(0 == strncmp("hybrid", argv[1], strlen("hybrid")))
        g_mode = HYBRID;
    else if(0 == strncmp("server", argv[1], strlen("server")))
        g_mode = SERVER;
    else if(0 == strncmp("client", argv[1], strlen("client")))
        g_mode = CLIENT;
    else
    {
        printf("Did not recogize mode %s\n", argv[1]);
        return 1;
    }

    char *endptr;
    long separation = 0;
    long length = strtol(argv[2], &endptr, 10);
    if((MIN_DATA_SIZE > length) || (MAX_DATA_SIZE < length))
    {
        printf("Error: size of data must be between %d and %d bytes.\n", MIN_DATA_SIZE, MAX_DATA_SIZE);
        return 1;
    }
    else if((HYBRID == g_mode) || (SERVER == g_mode))
    {
        g_data = new char[length];
        memset(g_data, 'p', length - 1);
        g_data[length - 1] = '\0';
    }
    
    long reps = strtol(argv[3], &endptr, 10);
    if(MIN_REPS > reps)
    {
        printf("Error: mininum value of reps is %d.\n", MIN_REPS);
        reps = 1;
    }
    //Preallocate.
    g_rpc_start.resize(reps);
    g_rpc_end.resize(reps);
    g_handler_start.resize(reps);
    g_handler_end.resize(reps);
    
    separation = strtol(argv[4], &endptr, 10);
    if(MIN_SEPARATION > separation)
    {
        printf("Error: mininum value of separation is %d ms.\n", MIN_SEPARATION);
        return 1;
    }
    else
        printf("Separation is %ld ms\n", separation);

    if((err = rbuscore_openBrokerConnection2(APPLICATION_NAME, argv[6])) == RBUSCORE_SUCCESS)
    {
        printf("Successfully connected to bus.\n");
    }

    if((HYBRID == g_mode) || (SERVER == g_mode))
    {
        if((err = rbuscore_registerObj(OBJECT_NAME, callback, NULL)) == RBUSCORE_SUCCESS)
        {
            printf("Successfully registered object.\n");
        }

        rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, NULL, handle_get}};
        rbuscore_registerMethodTable(OBJECT_NAME, table, 1); 
    }
    if(0 == strncmp("taint", argv[5], strlen("taint")))
        _rtConnection_TaintMessages(1);
    if((HYBRID == g_mode) || (CLIENT == g_mode))
        run_test(reps, separation);
    else
        pause();

    if((HYBRID == g_mode) || (SERVER == g_mode))
        delete [] g_data;
    
    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
    }

    print_analysis();
    return 0;
}
