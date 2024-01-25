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
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <benchmark/benchmark.h>
extern "C" {
#include "rbus_core.h"

}
#include "rbus_test_util.h"

#define DEFAULT_RESULT_BUFFERSIZE 128
#define MAX_SERVER_NAME 20

bool CALL_RBUS_OPEN_BROKER_CONNECTION(char* component_name)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbuscore_openBrokerConnection(component_name)) == RBUSCORE_SUCCESS)
    {
         result = true;
    }
    else
    {
         printf("Failed to open connection !!!\n");
    }
    return result;
}

bool CALL_RBUS_CLOSE_BROKER_CONNECTION()
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        result = true;
    }
    else
    {
         printf("Failed to close connection !!!\n");
    }
    return result;
}

void handle_term(int sig)
{
    (void) sig;
    CALL_RBUS_CLOSE_BROKER_CONNECTION();
    //printf("**********EXITING SERVER ******************** \n");
    kill(getpid(), SIGKILL);
}
void CREATE_RBUS_SERVER_INSTANCE(int handle)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    //printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term);
    reset_stored_data();

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(server_name);

    if(true != conn_status)
    {
       printf("rbuscore_openBrokerConnection failed!!");
    }

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", server_name);
    //printf("Registering object %s\n", buffer);

    err = rbuscore_registerObj(buffer, callback, NULL);
    if(RBUSCORE_SUCCESS != err)
    {
       printf("rbuscore_registerObj failed!!");
    }

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1}};

    err = rbuscore_registerMethodTable(buffer, table, 2);
    if(RBUSCORE_SUCCESS != err)
    {
       printf("rbuscore_registerMethodTable failed!!");
    }

    //pause();
    //printf("**********EXITING SERVER : %s ******************** \n", server_name);
    return;
}

static void BM_OpenBrokerConnection(benchmark::State& state) {
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char component_name[] = "component_1";

    for (auto _ : state)
    {
        err = rbuscore_openBrokerConnection(component_name);
    }
    if(RBUSCORE_SUCCESS == err)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}
BENCHMARK(BM_OpenBrokerConnection)->Iterations(1);

static void BM_CloseBrokerConnection(benchmark::State& state) {
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char component_name[] = "component_1";

    CALL_RBUS_OPEN_BROKER_CONNECTION(component_name);

    for (auto _ : state)
    {
        err = rbuscore_closeBrokerConnection();
    }
    if(RBUSCORE_SUCCESS != err)
        printf("rbuscore_closeBrokerConnection failed!!");
}
BENCHMARK(BM_CloseBrokerConnection)->Iterations(1);

static void BM_RegisterObj(benchmark::State& state) {
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char component_name[MAX_SERVER_NAME] = "component_";
    int handle = 1;
    char buffer[DEFAULT_RESULT_BUFFERSIZE];

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf((component_name + strlen(component_name)), (MAX_SERVER_NAME - strlen(component_name)), "%d", handle);

    CALL_RBUS_OPEN_BROKER_CONNECTION(component_name);

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", component_name);

    for (auto _ : state)
    {
        err = rbuscore_registerObj(buffer, callback, NULL);
    }
    if(RBUSCORE_SUCCESS != err)
        printf("rbuscore_registerObj failed!!");

    CALL_RBUS_CLOSE_BROKER_CONNECTION();
}
BENCHMARK(BM_RegisterObj)->Iterations(1);
static void BM_CreateServer(benchmark::State& state) {
    int counter = 2;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        //printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        kill(pid,SIGTERM);
        for (auto _ : state)
        {
            printf("Create server test success-Not benchmarking any call!!\n");
        }
    }
    else
    {
        printf("fork failed.\n");
    }

}
BENCHMARK(BM_CreateServer)->Iterations(1);

static void BM_PushObject(benchmark::State& state) {
    int counter = 2;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        //printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        rbusMessage setter;
        char server_obj[] = "test_server_2.obj1";
        char test_string[] = "rbus benchmark test 1";
        char client_name[] = "TEST_CLIENT_1";
        bool conn_status = false;

       conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

        rbusMessage_Init(&setter);
        rbusMessage_SetString(setter, test_string);
        for (auto _ : state)
        {
            err = rbuscore_pushObj(server_obj, setter, 1000);
        }

        if(RBUSCORE_SUCCESS != err)
            printf("rbuscore_pushObj failed!!");

        if(conn_status)
            CALL_RBUS_CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}
BENCHMARK(BM_PushObject)->Iterations(1);

static void BM_PullObject(benchmark::State& state) {
    int counter = 2;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        //printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        rbusMessage setter;
        rbusMessage response;
        char server_obj[] = "test_server_2.obj1";
        char test_string[] = "rbus benchmark test 1";
        char client_name[] = "TEST_CLIENT_1";
        bool conn_status = false;

       conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

        rbusMessage_Init(&setter);
        rbusMessage_SetString(setter, test_string);
        err = rbuscore_pushObj(server_obj, setter, 1000);
        if(RBUSCORE_SUCCESS != err)
            printf("rbuscore_pushObj failed!!");

        for (auto _ : state)
        {
            err = rbuscore_pullObj(server_obj, 1000, &response);
        }

        if(err == RBUSCORE_SUCCESS)
        {
            const char* buff = NULL;
            rbusMessage_GetString(response, &buff);
            if(NULL != buff)
            {
                if (strncmp (buff,test_string,strlen(test_string)) == 0)
                {
                    //printf ("Same string!! %s\n",buff);
                }
                else
                {
                    printf ("Error!! Pull failed to fetch the same string!! %s\n",buff);
                }
            }
        }
        else
            printf("rbuscore_pullObj failed!!");

        rbusMessage_Release(response);

        if(conn_status)
            CALL_RBUS_CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}
BENCHMARK(BM_PullObject)->Iterations(1);

static void BM_InvokeRemoteMethodSet (benchmark::State& state) {
    int counter = 2;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        //printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        rbusMessage setter;
        char server_obj[] = "test_server_2.obj1";
        char test_string[] = "rbus benchmark test 1";
        char client_name[] = "TEST_CLIENT_1";
        bool conn_status = false;
        rbusMessage response;

       conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

        rbusMessage_Init(&setter);
        rbusMessage_SetString(setter, test_string);
        for (auto _ : state)
        {
            err = rbuscore_invokeRemoteMethod(server_obj, METHOD_SETPARAMETERVALUES, setter, 1000, &response);
        }
        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if(RBUSCORE_SUCCESS != err)
            printf("rbuscore_invokeRemoteMethod failed!!");

        if(conn_status)
            CALL_RBUS_CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}
BENCHMARK(BM_InvokeRemoteMethodSet)->Iterations(1);

static void BM_InvokeRemoteMethodGet (benchmark::State& state) {
    int counter = 2;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        //printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        rbusMessage setter;
        char server_obj[] = "test_server_2.obj1";
        char test_string[] = "rbus benchmark test 1";
        char client_name[] = "TEST_CLIENT_1";
        bool conn_status = false;
        rbusMessage response;

       conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

        rbusMessage_Init(&setter);
        rbusMessage_SetString(setter, test_string);
        err = rbuscore_invokeRemoteMethod(server_obj, METHOD_SETPARAMETERVALUES, setter, 1000, &response);
        if(RBUSCORE_SUCCESS != err)
            printf("rbuscore_invokeRemoteMethod failed!!");

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        for (auto _ : state)
        {
            err = rbuscore_invokeRemoteMethod(server_obj, METHOD_GETPARAMETERVALUES, NULL,  1000, &response);
        }

        if(err == RBUSCORE_SUCCESS)
        {
            const char* buff = NULL;
            rbusMessage_GetString(response, &buff);
            if(NULL != buff)
            {
                if (strncmp (buff,test_string,strlen(test_string)) == 0)
                {
                    //printf ("Same string!! %s\n",buff);
                }
            }
        }
        else
            printf("rbuscore_invokeRemoteMethod failed!!");

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }
        if(conn_status)
            CALL_RBUS_CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}
BENCHMARK(BM_InvokeRemoteMethodGet)->Iterations(1);

BENCHMARK_MAIN();
