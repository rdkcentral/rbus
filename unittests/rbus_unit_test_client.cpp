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
Test Case : Testing rbus communications from client end
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
extern "C" {
#include "rbuscore.h"
#include "rbus_session_mgr.h"

}
#include "gtest_app.h"

#define DEFAULT_RESULT_BUFFERSIZE 128

typedef struct
{
   char name[100];
   int age;
   bool work_status;
}test_struct_t;


char server_kill[] = "killall -9 rbus_test_server";
#ifdef BUILD_FOR_DESKTOP
char server_create[] = "./unittests/rbus_test_server alpha > /tmp/ll.txt  2>&1 &";
#else
char server_create[] = "/usr/bin/rbus_test_server alpha > /tmp/ll.txt  2>&1 &";
#endif
static int g_current_session_id = 0;

class TestClient : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    //rtrouted dependency
    //printf("Successfully created rtrouted\n");

    system(server_create);
    sleep(2);
    printf("Set up done Successfully for TestClient\n");
}

static void TearDownTestCase()
{
    system(server_kill);
    printf("Clean up done Successfully for TestClient\n");
}

};

static bool CALL_RBUS_OPEN_BROKER_CONNECTION(char* client_name)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbus_openBrokerConnection(client_name)) == RBUSCORE_SUCCESS)
    {
         //printf("Successfully connected to bus.\n");
         result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed";
    return result;
}

static bool CALL_RBUS_CLOSE_BROKER_CONNECTION()
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbus_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        //printf("Successfully disconnected from bus.\n");
        result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed";
    return result;
}

static bool CALL_RBUS_PULL_OBJECT(char* expected_data, char* server_obj)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    if((err = rbus_pullObj(server_obj, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        rbusMessage_GetString(response, &buff);
        EXPECT_STREQ(buff, expected_data) << "rbus_pullObj failed to procure the server's initial string -init init init- ";
        rbusMessage_Release(response);
        result = true;
    }
    else
    {
        printf("Could not pull object %s\n", server_obj);
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_pullObj failed";
    return result;
}

static bool CALL_RBUS_PUSH_OBJECT(char* data, char* server_obj)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, data);
    err = rbus_pushObj(server_obj, setter, 1000);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_pushObj failed";
    return true;
}

static bool CALL_RBUS_PUSH_OBJECT_NO_ACK(char* data, char* server_obj)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, data);
    err = rbus_pushObjNoAck(server_obj, setter);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_pushObj failed";
    return true;
}

static bool CALL_RBUS_PUSH_OBJECT_DETAILED(char* server_obj, test_struct_t ip_data)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage response;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, ip_data.name);
    //printf("Set name : %s \n",ip_data.name);
    rbusMessage_SetInt32(setter, ip_data.age);
    //printf("Set age : %d  \n", ip_data.age);
    err = rbus_invokeRemoteMethod(server_obj, METHOD_SETPARAMETERATTRIBUTES, setter, 1000, &response);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation failed";

    if(RBUSCORE_SUCCESS == err)
        rbusMessage_Release(response);
    return true;
}

static bool CALL_RBUS_PULL_OBJECT_DETAILED(char* server_obj, test_struct_t expected_op)
{
    bool result = false;
    int age = 0;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    if((err = rbus_invokeRemoteMethod(server_obj, METHOD_GETPARAMETERATTRIBUTES, NULL, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        rbusMessage_GetString(response, &buff);
        rbusMessage_GetInt32(response, &age);
        EXPECT_STREQ(buff, expected_op.name) << "METHOD_GETPARAMETERATTRIBUTES failed to procure expected name ";
        EXPECT_EQ(age, expected_op.age) << "METHOD_GETPARAMETERATTRIBUTES failed to procure expected age ";
        rbusMessage_Release(response);
        result = true;
    }
    else
    {
        printf("Could not invoke remote method %s\n", server_obj);
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_invokeRemoteMethod failed";
    return result;
}

rbusCoreError_t CREATE_SESSION()
{
    rbusMessage response = NULL;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    ret = rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_REQUEST_SESSION_ID, NULL, 1000, &response);
    if(RBUSCORE_SUCCESS == ret)
    {
        int result;
        if(RT_OK == rbusMessage_GetInt32(response, &result))
        {
            if(RBUSCORE_SUCCESS != result)
            {
                printf("ERROR Cannot create a new session, Session already exist \n");
                ret = RBUSCORE_ERROR_INVALID_STATE;
                rbusMessage_Release(response);
                return ret;
            }
        }
        if(RT_OK == rbusMessage_GetInt32(response, &g_current_session_id))
        {
            printf("Got new session id %d\n", g_current_session_id);
            ret = RBUSCORE_SUCCESS;
            rbusMessage_Release(response);
            return ret;
        }
        else{
            printf("Malformed response from session manager.\n");
        }
    }
    else{
        printf("RPC with session manager failed.\n");
        ret = RBUSCORE_ERROR_ENTRY_NOT_FOUND;
        rbusMessage_Release(response);
        return ret;
    }
    return ret;
}

rbusCoreError_t PRINT_CURRENT_SESSION_ID()
{
    rbusMessage response = NULL;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    ret = rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID, NULL, 1000, &response);

    if(RBUSCORE_SUCCESS == ret)
    {
        int result;
        if(RT_OK == rbusMessage_GetInt32(response, &result))
        {
            if(RBUSCORE_SUCCESS != result)
            {
                printf("Session manager reports internal error %d.\n", result);
                ret = RBUSCORE_ERROR_INVALID_STATE;
                rbusMessage_Release(response);
                return ret;
            }
        }
        if(RT_OK == rbusMessage_GetInt32(response, &g_current_session_id))
        {
            printf("Current session id %d\n", g_current_session_id);
            ret = RBUSCORE_SUCCESS;
            rbusMessage_Release(response);
            return ret;
        }
        else{
            printf("Malformed response from session manager.\n");
        }
    }
    else{
        printf("RPC with session manager failed.\n");
        ret = RBUSCORE_ERROR_ENTRY_NOT_FOUND;
        rbusMessage_Release(response);
        return ret;
    }
    return ret;
}

rbusCoreError_t END_SESSION(int session)
{
    rbusMessage out;
    rbusMessage response = NULL;

    rbusMessage_Init(&out);
    rbusMessage_SetInt32(out, session);
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    if(RBUSCORE_SUCCESS == rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_END_SESSION, out, 1000, &response))
    {
        int result;
        if(RT_OK == rbusMessage_GetInt32(response, &result))
        {
            if(RBUSCORE_SUCCESS != result)
            {
                printf("ERROR Cannot end session, It doesn't match active session\n");
                ret = RBUSCORE_ERROR_INVALID_STATE;
                rbusMessage_Release(response);
                return ret;
            }
            else{
                printf("Successfully ended session %d.\n", session);
                ret = RBUSCORE_SUCCESS;
                rbusMessage_Release(response);
                return ret;
            }
        }
    }
    else{
        printf("RPC with session manager failed.\n");
        ret = RBUSCORE_ERROR_ENTRY_NOT_FOUND;
        rbusMessage_Release(response);
        return ret;
    }
    return ret;
}

TEST_F(TestClient, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(TestClient, openBrokerConnection_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    if(CALL_RBUS_OPEN_BROKER_CONNECTION(client_name))
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pullObj_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char server_init_test_string[] = "init init init";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PULL_OBJECT(server_init_test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObj_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObj_test2)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_stringggggggggggggggggggggggggggggggggggggggend";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObj_test3)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_stringggggggggggggggggggggggggggggggggggggggendnew";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObjNoAck_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_NO_ACK(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObjNoAck_test2)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_stringggggggggggggggggggggggggggggggggggggggend";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_NO_ACK(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_pushObjNoAck_test3)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_stringggggggggggggggggggggggggggggggggggggggendnew";

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_NO_ACK(test_string, server_obj);

    CALL_RBUS_PULL_OBJECT(test_string, server_obj);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_invokeRemoteMethod_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj1";
    bool conn_status = false;
    test_struct_t test_data = {"TEST_NAME1", 35, true};

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_DETAILED(server_obj, test_data);
    CALL_RBUS_PULL_OBJECT_DETAILED(server_obj, test_data);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_invokeRemoteMethod_test2)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "alpha.obj2";
    bool conn_status = false;
    test_struct_t test_data = {"TEST_NAME2", 45, true};

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_DETAILED(server_obj, test_data);
    CALL_RBUS_PULL_OBJECT_DETAILED(server_obj, test_data);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_test_obj_coexistance_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "alpha.obj1";
    char server_obj2[] = "alpha.obj2";
    bool conn_status = false;
    test_struct_t test_data1 = {"TEST_NAME1", 35, true};
    test_struct_t test_data2 = {"TEST_NAME2", 45, true};

    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);

    CALL_RBUS_PUSH_OBJECT_DETAILED(server_obj1, test_data1);
    CALL_RBUS_PUSH_OBJECT_DETAILED(server_obj2, test_data2);
    CALL_RBUS_PULL_OBJECT_DETAILED(server_obj1, test_data1);
    CALL_RBUS_PULL_OBJECT_DETAILED(server_obj2, test_data2);

    if(conn_status)
        CALL_RBUS_CLOSE_BROKER_CONNECTION();
}

TEST_F(TestClient, rbus_session_manager_test1)
{
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*********************  CREATING CLIENT : %s \n", client_name);
    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);
    if(conn_status)
        printf("Successfully connected to bus.\n");
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "CREATE_SESSION failed";
    err = PRINT_CURRENT_SESSION_ID();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "PRINT_CURRENT_SESSION_ID failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "END_SESSION failed";
    conn_status = CALL_RBUS_CLOSE_BROKER_CONNECTION();
    if(conn_status)
        printf("Successfully disconnected from bus.\n");
}

TEST_F(TestClient, rbus_session_manager_test2)
{
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*********************  CREATING CLIENT : %s \n", client_name);
    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);
    if(conn_status)
        printf("Successfully connected to bus.\n");
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "CREATE_SESSION failed";
    //Neg Test Calling CREATE_SESSION again
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "CREATE_SESSION failed";
    err = PRINT_CURRENT_SESSION_ID();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "PRINT_CURRENT_SESSION_ID failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "END_SESSION failed";
    conn_status = CALL_RBUS_CLOSE_BROKER_CONNECTION();
    if(conn_status)
        printf("Successfully disconnected from bus.\n");
}

TEST_F(TestClient, rbus_session_manager_test3)
{
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*********************  CREATING CLIENT : %s \n", client_name);
    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);
    if(conn_status)
        printf("Successfully connected to bus.\n");
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "CREATE_SESSION failed";
    err = PRINT_CURRENT_SESSION_ID();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "PRINT_CURRENT_SESSION_ID failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "END_SESSION failed";
    //Neg Test Calling END_SESSION again
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "END_SESSION failed";
    conn_status = CALL_RBUS_CLOSE_BROKER_CONNECTION();
    if(conn_status)
        printf("Successfully disconnected from bus.\n");
}

TEST_F(TestClient, rbus_session_manager_test4)
{
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*********************  CREATING CLIENT : %s \n", client_name);
    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);
    if(conn_status)
        printf("Successfully connected to bus.\n");
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "CREATE_SESSION failed";
    //Neg Test Calling CREATE_SESSION multiple times
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "CREATE_SESSION failed";
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "CREATE_SESSION failed";
    err = PRINT_CURRENT_SESSION_ID();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "PRINT_CURRENT_SESSION_ID failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "END_SESSION failed";
    conn_status = CALL_RBUS_CLOSE_BROKER_CONNECTION();
    if(conn_status)
        printf("Successfully disconnected from bus.\n");
}

TEST_F(TestClient, rbus_session_manager_test5)
{
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*********************  CREATING CLIENT : %s \n", client_name);
    conn_status = CALL_RBUS_OPEN_BROKER_CONNECTION(client_name);
    if(conn_status)
        printf("Successfully connected to bus.\n");
    err = CREATE_SESSION();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "CREATE_SESSION failed";
    err = PRINT_CURRENT_SESSION_ID();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "PRINT_CURRENT_SESSION_ID failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "END_SESSION failed";
    //Neg Test Calling END_SESSION multiple times
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "END_SESSION failed";
    err = END_SESSION(g_current_session_id);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "END_SESSION failed";
    conn_status = CALL_RBUS_CLOSE_BROKER_CONNECTION();
    if(conn_status)
        printf("Successfully disconnected from bus.\n");
}
