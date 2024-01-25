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
/******************************************************
Test Case : Testing nested rbus RPC calls
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
extern "C" {
#include "rbuscore.h"

}
#include "gtest_app.h"
#include "rbus_test_util.h"

#define DEFAULT_RESULT_BUFFERSIZE 128
#define MAX_SERVER_NAME 20


static bool OPEN_BROKER_CONNECTION2(const char* connection_name)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbuscore_openBrokerConnection(connection_name)) == RBUSCORE_SUCCESS)
    {
         result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_openBrokerConnection failed";
    return result;
}

static bool CLOSE_BROKER_CONNECTION2()
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbuscore_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        //printf("Successfully disconnected from bus.\n");
        result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_closeBrokerConnection failed";
    return result;
}

static void handle_term2(int sig)
{
    (void) sig;
    CLOSE_BROKER_CONNECTION2();
    printf("**********EXITING SERVER ******************** \n");
    kill(getpid(), SIGKILL);
}

static void CREATE_RBUS_SERVER_INSTANCE(const char * server_name, const char * object_name)
{
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term2);
    reset_stored_data();

    conn_status = OPEN_BROKER_CONNECTION2(server_name);

    ASSERT_EQ(conn_status, true) << "OPEN_BROKER_CONNECTION2 failed";

    printf("Registering object %s\n", object_name);

    err = rbuscore_registerObj(object_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_registerObj failed";

    rbus_method_table_entry_t table[1] = {{METHOD_GETPARAMETERVALUES, NULL, handle_recursive_get}};

    err = rbuscore_registerMethodTable(object_name, table, 1);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_registerMethodTable failed";
    return;
}

static void RBUS_RPC(const char *object, const char* method, const char * payload, rbusCoreError_t expected_err)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rtError er = RT_OK;
    int rpc_result = RBUSCORE_ERROR_GENERAL;
    rbusMessage setter, response;
    rbusMessage_Init(&setter);
    if(nullptr != payload)
        rbusMessage_SetString(setter, payload);
    err = rbuscore_invokeRemoteMethod(object, method, setter, 1000, &response);
    EXPECT_EQ(err, expected_err) << "Nested RPC failed";
   
    if(RBUSCORE_SUCCESS == err)
    {
        er = rbusMessage_GetInt32(response, &rpc_result);
        EXPECT_EQ(RT_OK, er) << "Nested RPC failed";
        EXPECT_EQ(RBUSCORE_SUCCESS, rpc_result) << "Nested RPC failed";
        rbusMessage_Release(response);
    }
    return;
}

class NestedRPCTest : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for NestedRPCTest\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for NestedRPCTest\n");
}

};

TEST_F(NestedRPCTest, rbus_multipleServer_test1)
{
    int i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    int server_count = 2;
    pid_t pid[10];
    bool is_parent = true;
    const char * server_table[2][2] = {{"server_ping", "ping"}, {"server_pong", "pong"}};

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            CREATE_RBUS_SERVER_INSTANCE(server_table[j][0], server_table[j][1]);
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        RBUS_RPC("ping", METHOD_GETPARAMETERVALUES, "pong", RBUSCORE_SUCCESS);
        RBUS_RPC("pong", METHOD_GETPARAMETERVALUES, "ping", RBUSCORE_SUCCESS);
        //RBUS_RPC("ping", METHOD_GETPARAMETERVALUES, "ping", RBUSCORE_SUCCESS);
        //RBUS_RPC("pong", METHOD_GETPARAMETERVALUES, "pong", RBUSCORE_SUCCESS);
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();
        for(i = 0; i < server_count; i++)
            kill(pid[i],SIGTERM);
    }
}
