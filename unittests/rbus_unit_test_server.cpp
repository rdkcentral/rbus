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
Test Case : Testing rbus server creation APIs
*******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
extern "C" {
#include "rbuscore.h"
#include "rtMemory.h"
}
#include "rtList.h"
#include "rtLog.h"
#include "rbus.h"
#include "gtest_app.h"
#include "rbus_test_util.h"

#define DEFAULT_RESULT_BUFFERSIZE 128

#define MAX_SERVER_NAME 20

#define MAX_BROKER_ADDRESS_LEN 256

#define FILEPATH "/etc/rbus_client.conf"

static char g_broker_address[MAX_BROKER_ADDRESS_LEN] = "unix:///tmp/rtrouted";

static bool RBUS_OPEN_BROKER_CONNECTION(char* server_name, rbusCoreError_t expected_status)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbus_openBrokerConnection(server_name)) == RBUSCORE_SUCCESS)
    {
         //printf("Successfully connected to bus.\n");
         result = true;
    }
    EXPECT_EQ(err, expected_status) << "rbus_openBrokerConnection failed";
    return result;
}

static bool RBUS_OPEN_BROKER_CONNECTION2(char* server_name,const char * broker_address,rbusCoreError_t expected_status)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbus_openBrokerConnection2(server_name,broker_address)) == RBUSCORE_SUCCESS)
    {
         printf("Successfully connected to bus.\n");
         result = true;
    }
    EXPECT_EQ(err, expected_status) << "rbus_openBrokerConnection2 failed";
    return result;
}

static bool RBUS_CLOSE_BROKER_CONNECTION(rbusCoreError_t expected_status)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbus_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        //printf("Successfully disconnected from bus.\n");
        result = true;
    }
    EXPECT_EQ(err, expected_status) << "rbus_openBrokerConnection failed";
    return result;
}

/*Signal handler for closing broker connection*/
static void handle_signal(int sig)
{
    (void) sig;
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    kill(getpid(),SIGKILL);
}

static void CREATE_RBUS_SERVER(int handle)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_signal);
    reset_stored_data();

    conn_status = RBUS_OPEN_BROKER_CONNECTION(server_name, RBUSCORE_SUCCESS);

    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", server_name);

    err = rbus_registerObj(buffer, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1}};
    err = rbus_registerMethodTable(buffer, table, 2);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";

//    pause();
    printf("**********EXITING SERVER : %s ******************** \n", server_name);
    return;
}

static void CREATE_RBUS_SERVER_REG_OBJECT(int handle)
{
    char server_name[64] = "";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(server_name, (sizeof(server_name) - 1), "%s%d", "test_server_", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_signal);
    reset_stored_data();
    err = rbus_openBrokerConnection(server_name);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed";

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", server_name);

    err = rbus_registerObj(buffer, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

//  pause();
    printf("**********EXITING SERVER : %s ******************** \n", server_name);
    return;
}

static void get_broker_address(void)
{
#if BUILD_FOR_DESKTOP
    return;
#else
    FILE* fconfig = fopen(FILEPATH, "r");
    if(fconfig)
    {
        size_t len;
        char buff[MAX_BROKER_ADDRESS_LEN] = {0};

        /*locate the first word(block of printable text)*/
        while(fgets(buff, MAX_BROKER_ADDRESS_LEN, fconfig))
        {
            len = strlen(buff);
            if(len > 0)
            {
                size_t idx1 = 0;

                /*move past any leading space*/
                while(idx1 < len && isspace(buff[idx1]))
                {
                    idx1++;
                }

                if(idx1 < len)
                {
                    size_t idx2 = idx1+1;

                    /*move to end of word*/
                    while(idx2 < len && !isspace(buff[idx2]))
                        idx2++;

                    if(idx2-idx1 > 0)
                    {
                        buff[idx2] = 0;
                        strcpy(g_broker_address, &buff[idx1]);
                        break;
                    }
                }
            }
        }
    }
    fclose(fconfig);
#endif
}

class TestServer : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for TestServer\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for TestServer\n");
}

};

TEST_F(TestServer, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(TestServer, rbus_openBrokerConnection_test1)
{
    char server_name[20] = "test_server_";
    if(RBUS_OPEN_BROKER_CONNECTION(server_name, RBUSCORE_SUCCESS))
        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_openBrokerConnection_test2)
{
    char server_name[20] = "test_server_2";
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if(RBUS_OPEN_BROKER_CONNECTION(server_name, RBUSCORE_SUCCESS))
    {
        err = rbus_openBrokerConnection(server_name);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed to return error on duplicate connection attempt";

        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
        //Neg test calling rbus_closeBrokerConnection again
        err = rbus_closeBrokerConnection();
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbus_closeBrokerConnection failed";
    }
    return;
}

TEST_F(TestServer, rbus_openBrokerConnection_test3)
{
    char server_name1[20] = "test_server_3";
    char server_name2[20] = "test_server_4";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if(RBUS_OPEN_BROKER_CONNECTION(server_name1, RBUSCORE_SUCCESS))
    {
        err = rbus_openBrokerConnection(server_name1);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed to return error on duplicate connection attempt";

        err = rbus_openBrokerConnection(server_name2);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed to return error on duplicate connection attempt";

        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    }
    return;
}

TEST_F(TestServer, rbus_openBrokerConnection_test4)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    // Neg test with component name to be NULL
    err = rbus_openBrokerConnection(NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_openBrokerConnection failed";
    return;
}

TEST_F(TestServer, rbus_openBrokerConnection2_test1)
{
    char server_name[20] = "test_server_";

    get_broker_address();
    if(RBUS_OPEN_BROKER_CONNECTION2(server_name,g_broker_address,RBUSCORE_SUCCESS))
        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_openBrokerConnection2_test2)
{
    const char * broker_address = "unix:///tmp/rtrouted";

    if(RBUS_OPEN_BROKER_CONNECTION2(NULL,broker_address,RBUSCORE_ERROR_INVALID_PARAM))
        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
        return;
}

TEST_F(TestServer, rbus_openBrokerConnection2_test3)
{
    char server_name[20] = "test_server_";
    const char * broker_address = "";

    if(RBUS_OPEN_BROKER_CONNECTION2(server_name,broker_address,RBUSCORE_ERROR_GENERAL))
        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
        return;
}

TEST_F(TestServer, rbus_openBrokerConnection2_test4)
{
    char server_name[20] = "test_server_2";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    get_broker_address();
    if(RBUS_OPEN_BROKER_CONNECTION2(server_name,g_broker_address,RBUSCORE_SUCCESS))
    {

        err = rbus_openBrokerConnection2(server_name,g_broker_address);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed to return error on duplicate connection attempt";

        RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    }
    return;
}

TEST_F(TestServer, rbus_registerObj_test1)
{
    int counter = 1;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObj_test2)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    //Neg test registering object without establishing connection
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbus_registerObj failed";
    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    /*Registering a new object with same name*/
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_unregisterObj_test1)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    /*unregistering object with same name*/
    err = rbus_unregisterObj(obj_name);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_unregisterObj_test2)
{
    int counter = 1;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    /*unregister called with NULL param*/
    err = rbus_unregisterObj(NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_unregisterObj_test3)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj2";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    /*Unregistering an object with different name*/
    err = rbus_unregisterObj(obj_name);
    EXPECT_EQ(err, RBUSCORE_ERROR_GENERAL) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjNameCheck_test1)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj2";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    /*Registering a new object with different name*/
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjNameCheck_test2)
{
    int counter = 1;
    char obj_name1[20] = "test_server_1.obj1";
    char obj_name2[20] = "test_server_2.obj1";
    char obj_name3[20] = "test_server_12.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    err = rbus_registerObj(obj_name1, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name1);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    err = rbus_registerObj(obj_name2, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    err = rbus_registerObj(obj_name2, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    err = rbus_registerObj(obj_name2, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name2);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    err = rbus_registerObj(obj_name3, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name3);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Boundary testing for MAX_OBJECT_NAME_LENGTH - 128*/
TEST_F(TestServer, rbus_registerObjNameCheck_test3)
{
    int counter = 1;
    char obj_name[129] = "0";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    memset(obj_name, 't', (sizeof(obj_name)- 1));
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Boundary testing for MAX_OBJECT_NAME_LENGTH - 128*/
TEST_F(TestServer, rbus_registerObjNameCheck_test4)
{
    int counter = 1;
    char obj_name[128] = "0";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    memset(obj_name, 't', ( sizeof(obj_name) - 1));
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Boundary testing for MAX_OBJECT_NAME_LENGTH - 128*/
TEST_F(TestServer, rbus_registerObjNameCheck_test5)
{
    int counter = 1;
    char obj_name[129] = "0";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    memset(obj_name, 't', ( sizeof(obj_name) - 2));
    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    err = rbus_unregisterObj(obj_name);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjNameCheck_test6)
{
    int counter = 1;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);
    //Neg test passing NULL value as obj_name
    err = rbus_registerObj(NULL, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerObj failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjBoundaryPositive_test1)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_registerObj(buffer, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    }
    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_unregisterObj(buffer);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjBoundaryPositive_test2)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_registerObj(buffer, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    }
    //Unregister the last object to provide space for a new object
    printf("Unregistering object %s \n", buffer);
    err = rbus_unregisterObj(buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, (i - 1));
    printf("Registering object %s \n", buffer);
    err = rbus_registerObj(buffer, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_unregisterObj(buffer);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjBoundaryPositive_test3)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_registerObj(buffer, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    }
    //Unregister the last object to provide space for a new object
    printf("Unregistering object %s \n", buffer);
    err = rbus_unregisterObj(buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, (i-1));
    printf("Registering object %s \n", buffer);
    err = rbus_registerObj(buffer, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    for(i = 2; i <= 63; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
        //printf("Registering object %s \n", buffer);
        err = rbus_unregisterObj(buffer);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerObjBoundaryNegative_test1)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 2; i <= 63; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
       //printf("Registering object %s \n", buffer);
       err = rbus_registerObj(buffer, callback, NULL);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
    }

    for(i = 2; i <= 63; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "%s_%d", obj_name, i);
       //printf("Registering object %s \n", buffer);
       err = rbus_unregisterObj(buffer);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterObj failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Testing MAX_SUPPORTED_METHODS - 32*/
TEST_F(TestServer, rbus_registerMethod_test1)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    }
    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_unregisterMethod(obj_name, buffer);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Testing MAX_SUPPORTED_METHODS - 32*/
TEST_F(TestServer, rbus_registerMethod_test2)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    }
    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
    printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_OUT_OF_RESOURCES) << "rbus_registerMethod failed";

    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_unregisterMethod(obj_name, buffer);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
    }

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Testing MAX_SUPPORTED_METHODS - 32*/
TEST_F(TestServer, rbus_registerMethod_test3)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    }
    printf("Unregistering method %s \n", buffer);
    err = rbus_unregisterMethod(obj_name, buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
    printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    err = rbus_unregisterMethod(obj_name, buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Testing MAX_SUPPORTED_METHODS - 32*/
TEST_F(TestServer, rbus_registerMethod_test4)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    for(i = 1; i <= MAX_SUPPORTED_METHODS; i++)
    {
       memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
       snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
       //printf("Registering method %s \n", buffer);
       err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    }

    printf("Unregistering method %s \n", buffer);
    err = rbus_unregisterMethod(obj_name, buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", (i - 1));
    printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    err = rbus_unregisterMethod(obj_name, buffer);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Setting improper object name for rbus_registerMethod*/
TEST_F(TestServer, rbus_registerMethod_test5)
{
    int counter = 1, i = 1;
    char obj_name[20] = "test_server_1.obj2";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf(buffer, (sizeof(buffer) - 1), "METHOD_%d", i);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerMethod_test6)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE] = "METHOD_SAME";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    //printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";

    //printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1,NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerMethod_test7)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    char buffer[DEFAULT_RESULT_BUFFERSIZE] = "METHOD_SAME";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    //printf("Registering method %s \n", buffer);
    err = rbus_registerMethod(obj_name, buffer, handle_set1, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
    //Neg test passing invalid method name
    err = rbus_unregisterMethod(obj_name, "method_1");
    EXPECT_EQ(err, RBUSCORE_ERROR_GENERAL) << "rbus_unregisterMethod failed";
    //Neg test passing invalid object name
    err = rbus_unregisterMethod("Device.Obj", buffer);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_unregisterMethod failed";
    err = rbus_registerMethod(obj_name, buffer, handle_get1, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Verification of MAX_METHOD_NAME_LENGTH - 64*/
TEST_F(TestServer, rbus_registerMethod_test8)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    char method_name1[62] = "0";
    char method_name2[66] = "0";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    /*Method name - 61 char + null termination*/
    memset(method_name1, 'n', (sizeof(method_name1)- 1));
    printf("Registering method %s strlen : %lu \n", method_name1, strlen(method_name1));

    err = rbus_registerMethod(obj_name, method_name1, handle_set1, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";

    /*Method name - 64char + null termination*/
    memset(method_name2, 'o', (sizeof(method_name2)- 2));
    printf("Registering method %s strlen : %lu\n", method_name2, strlen(method_name2));

    err = rbus_registerMethod(obj_name, method_name2, handle_get1, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethod failed";

    /*Method name - 65char + null termination*/
    memset(method_name2, 'o', (sizeof(method_name2)- 1));
    printf("Registering method %s : strlen : %lu\n", method_name2, strlen(method_name2));

    err = rbus_registerMethod(obj_name, method_name2, handle_get1, NULL);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethod failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_registerMethodTable_test1)
{
    int counter = 2;

    CREATE_RBUS_SERVER(counter);
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Registering 32 methods using rbus_registerMethodTable*/
TEST_F(TestServer, rbus_registerMethodTable_test2)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    rbus_method_table_entry_t table[32] = {{"METHOD_1", NULL, handle_set1}, {"METHOD_2", NULL, handle_get1},
                                           {"METHOD_3", NULL, handle_set1}, {"METHOD_4", NULL, handle_get1},
                                           {"METHOD_5", NULL, handle_set1}, {"METHOD_6", NULL, handle_get1},
                                           {"METHOD_7", NULL, handle_set1}, {"METHOD_8", NULL, handle_get1},
                                           {"METHOD_9", NULL, handle_set1}, {"METHOD_10", NULL, handle_get1},
                                           {"METHOD_11", NULL, handle_set1}, {"METHOD_12", NULL, handle_get1},
                                           {"METHOD_13", NULL, handle_set1}, {"METHOD_14", NULL, handle_get1},
                                           {"METHOD_15", NULL, handle_set1}, {"METHOD_16", NULL, handle_get1},
                                           {"METHOD_17", NULL, handle_set1}, {"METHOD_18", NULL, handle_get1},
                                           {"METHOD_19", NULL, handle_set1}, {"METHOD_20", NULL, handle_get1},
                                           {"METHOD_21", NULL, handle_set1}, {"METHOD_22", NULL, handle_get1},
                                           {"METHOD_23", NULL, handle_set1}, {"METHOD_24", NULL, handle_get1},
                                           {"METHOD_25", NULL, handle_set1}, {"METHOD_26", NULL, handle_get1},
                                           {"METHOD_27", NULL, handle_set1}, {"METHOD_28", NULL, handle_get1},
                                           {"METHOD_29", NULL, handle_set1}, {"METHOD_30", NULL, handle_get1},
                                           {"METHOD_31", NULL, handle_set1}, {"METHOD_32", NULL, handle_get1}};
    err = rbus_registerMethodTable(obj_name, table, 32);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";
    err = rbus_unregisterMethodTable(obj_name, table, 32);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethodTable failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Registering 33 methods using rbus_registerMethodTable*/
TEST_F(TestServer, rbus_registerMethodTable_test3)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    rbus_method_table_entry_t table[33] = {{"METHOD_1", NULL, handle_set1}, {"METHOD_2", NULL, handle_get1},
                                           {"METHOD_3", NULL, handle_set1}, {"METHOD_4", NULL, handle_get1},
                                           {"METHOD_5", NULL, handle_set1}, {"METHOD_6", NULL, handle_get1},
                                           {"METHOD_7", NULL, handle_set1}, {"METHOD_8", NULL, handle_get1},
                                           {"METHOD_9", NULL, handle_set1}, {"METHOD_10", NULL, handle_get1},
                                           {"METHOD_11", NULL, handle_set1}, {"METHOD_12", NULL, handle_get1},
                                           {"METHOD_13", NULL, handle_set1}, {"METHOD_14", NULL, handle_get1},
                                           {"METHOD_15", NULL, handle_set1}, {"METHOD_16", NULL, handle_get1},
                                           {"METHOD_17", NULL, handle_set1}, {"METHOD_18", NULL, handle_get1},
                                           {"METHOD_19", NULL, handle_set1}, {"METHOD_20", NULL, handle_get1},
                                           {"METHOD_21", NULL, handle_set1}, {"METHOD_22", NULL, handle_get1},
                                           {"METHOD_23", NULL, handle_set1}, {"METHOD_24", NULL, handle_get1},
                                           {"METHOD_25", NULL, handle_set1}, {"METHOD_26", NULL, handle_get1},
                                           {"METHOD_27", NULL, handle_set1}, {"METHOD_28", NULL, handle_get1},
                                           {"METHOD_29", NULL, handle_set1}, {"METHOD_30", NULL, handle_get1},
                                           {"METHOD_31", NULL, handle_set1}, {"METHOD_32", NULL, handle_get1},
                                           {"METHOD_33", NULL, handle_set1}};
    err = rbus_registerMethodTable(obj_name, table, 33);
    EXPECT_EQ(err, RBUSCORE_ERROR_OUT_OF_RESOURCES) << "rbus_registerMethodTable failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Registering 34 methods using rbus_registerMethodTable*/
TEST_F(TestServer, rbus_registerMethodTable_test4)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    rbus_method_table_entry_t table[34] = {{"METHOD_1", NULL, handle_set1}, {"METHOD_2", NULL, handle_get1},
                                           {"METHOD_3", NULL, handle_set1}, {"METHOD_4", NULL, handle_get1},
                                           {"METHOD_5", NULL, handle_set1}, {"METHOD_6", NULL, handle_get1},
                                           {"METHOD_7", NULL, handle_set1}, {"METHOD_8", NULL, handle_get1},
                                           {"METHOD_9", NULL, handle_set1}, {"METHOD_10", NULL, handle_get1},
                                           {"METHOD_11", NULL, handle_set1}, {"METHOD_12", NULL, handle_get1},
                                           {"METHOD_13", NULL, handle_set1}, {"METHOD_14", NULL, handle_get1},
                                           {"METHOD_15", NULL, handle_set1}, {"METHOD_16", NULL, handle_get1},
                                           {"METHOD_17", NULL, handle_set1}, {"METHOD_18", NULL, handle_get1},
                                           {"METHOD_19", NULL, handle_set1}, {"METHOD_20", NULL, handle_get1},
                                           {"METHOD_21", NULL, handle_set1}, {"METHOD_22", NULL, handle_get1},
                                           {"METHOD_23", NULL, handle_set1}, {"METHOD_24", NULL, handle_get1},
                                           {"METHOD_25", NULL, handle_set1}, {"METHOD_26", NULL, handle_get1},
                                           {"METHOD_27", NULL, handle_set1}, {"METHOD_28", NULL, handle_get1},
                                           {"METHOD_29", NULL, handle_set1}, {"METHOD_30", NULL, handle_get1},
                                           {"METHOD_31", NULL, handle_set1}, {"METHOD_32", NULL, handle_get1},
                                           {"METHOD_33", NULL, handle_set1}, {"METHOD_34", NULL, handle_get1}};
    err = rbus_registerMethodTable(obj_name, table, 34);
    EXPECT_EQ(err, RBUSCORE_ERROR_OUT_OF_RESOURCES) << "rbus_registerMethodTable failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Registering table array of 34 methods but with num_entries as 32 using rbus_registerMethodTable*/
TEST_F(TestServer, rbus_registerMethodTable_test5)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    rbus_method_table_entry_t table[34] = {{"METHOD_1", NULL, handle_set1}, {"METHOD_2", NULL, handle_get1},
                                           {"METHOD_3", NULL, handle_set1}, {"METHOD_4", NULL, handle_get1},
                                           {"METHOD_5", NULL, handle_set1}, {"METHOD_6", NULL, handle_get1},
                                           {"METHOD_7", NULL, handle_set1}, {"METHOD_8", NULL, handle_get1},
                                           {"METHOD_9", NULL, handle_set1}, {"METHOD_10", NULL, handle_get1},
                                           {"METHOD_11", NULL, handle_set1}, {"METHOD_12", NULL, handle_get1},
                                           {"METHOD_13", NULL, handle_set1}, {"METHOD_14", NULL, handle_get1},
                                           {"METHOD_15", NULL, handle_set1}, {"METHOD_16", NULL, handle_get1},
                                           {"METHOD_17", NULL, handle_set1}, {"METHOD_18", NULL, handle_get1},
                                           {"METHOD_19", NULL, handle_set1}, {"METHOD_20", NULL, handle_get1},
                                           {"METHOD_21", NULL, handle_set1}, {"METHOD_22", NULL, handle_get1},
                                           {"METHOD_23", NULL, handle_set1}, {"METHOD_24", NULL, handle_get1},
                                           {"METHOD_25", NULL, handle_set1}, {"METHOD_26", NULL, handle_get1},
                                           {"METHOD_27", NULL, handle_set1}, {"METHOD_28", NULL, handle_get1},
                                           {"METHOD_29", NULL, handle_set1}, {"METHOD_30", NULL, handle_get1},
                                           {"METHOD_31", NULL, handle_set1}, {"METHOD_32", NULL, handle_get1},
                                           {"METHOD_33", NULL, handle_set1}, {"METHOD_34", NULL, handle_get1}};
    err = rbus_registerMethodTable(obj_name, table, 32);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

/*Registering 32 methods using rbus_registerMethodTable with duplicate method names added in between*/
TEST_F(TestServer, rbus_registerMethodTable_test6)
{
    int counter = 1;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    CREATE_RBUS_SERVER_REG_OBJECT(counter);

    rbus_method_table_entry_t table[32] = {{"METHOD_1", NULL, handle_set1}, {"METHOD_2", NULL, handle_get1},
                                           {"METHOD_3", NULL, handle_set1}, {"METHOD_4", NULL, handle_get1},
                                           {"METHOD_5", NULL, handle_set1}, {"METHOD_6", NULL, handle_get1},
                                           {"METHOD_7", NULL, handle_set1}, {"METHOD_8", NULL, handle_get1},
                                           {"METHOD_9", NULL, handle_set1}, {"METHOD_10", NULL, handle_get1},
                                           {"METHOD_11", NULL, handle_set1}, {"METHOD_12", NULL, handle_get1},
                                           {"METHOD_13", NULL, handle_set1}, {"METHOD_14", NULL, handle_get1},
                                           {"METHOD_SAME", NULL, handle_set1}, {"METHOD_SAME", NULL, handle_get1},
                                           {"METHOD_17", NULL, handle_set1}, {"METHOD_18", NULL, handle_get1},
                                           {"METHOD_19", NULL, handle_set1}, {"METHOD_20", NULL, handle_get1},
                                           {"METHOD_21", NULL, handle_set1}, {"METHOD_22", NULL, handle_get1},
                                           {"METHOD_23", NULL, handle_set1}, {"METHOD_24", NULL, handle_get1},
                                           {"METHOD_25", NULL, handle_set1}, {"METHOD_26", NULL, handle_get1},
                                           {"METHOD_27", NULL, handle_set1}, {"METHOD_28", NULL, handle_get1},
                                           {"METHOD_29", NULL, handle_set1}, {"METHOD_30", NULL, handle_get1},
                                           {"METHOD_31", NULL, handle_set1}, {"METHOD_32", NULL, handle_get1}};
    err = rbus_registerMethodTable(obj_name, table, 32);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerMethodTable failed";

    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rbus_removeElement_test1)
{
    int counter = 1;
    char server_obj[] = "test_server_1.obj1";
    char obj_name[130] = "test_server_1.obj2";
    char server_element[] = "server_element1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    //Neg test adding element before establishing connection
    err = rbus_addElement(server_obj,server_element);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbus_addElement failed";
    //Neg test removing element before establishing connection
    err = rbus_removeElement(server_obj,server_element);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbus_removeElement failed";
    CREATE_RBUS_SERVER(counter);
    err = rbus_addElement(server_obj,server_element);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
    //Neg test passing NULL as object name
    err = rbus_removeElement(NULL,server_element);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_removeElement failed";
    //Neg test with too long object Name
    memset(obj_name, 'o', (sizeof(obj_name)- 1));
    err = rbus_removeElement(obj_name,server_element);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_removeElement failed";
    err = rbus_removeElement(server_obj,server_element);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_removeElement failed";
    RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    return;
}

TEST_F(TestServer, rtmsg_rtConnection_CreateWithConfig_test1)
{
  char const*   router_config ="unix:///tmp/rtrouted";
  rtError       err;
  rtMessage     config;
  rtConnection  connection;
  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "rtsend");
  rtMessage_SetString(config, "uri", router_config);
  rtMessage_SetInt32(config, "start_router", 1);
  err = rtConnection_CreateWithConfig(&connection, config);
  EXPECT_EQ(err, RT_OK) << "rtmsg_rtconnection_CreateWithConfig failed";
  err = rtConnection_Dispatch(connection);
  EXPECT_EQ(err, RT_OK);
  rtMessage_Release(config);
  rtConnection_Destroy(connection);
}

TEST_F(TestServer, rtmsg_rtConnection_CreateWithConfig_test2)
{
  char const*   router_config ="unix:///tmp/rtrouted";
  rtError       err;
  rtMessage     config;
  rtConnection  connection;
  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "rtsend");
  rtMessage_SetString(config, "uri", router_config);
  rtMessage_SetInt32(config, "start_router", 0);
  err = rtConnection_CreateWithConfig(&connection, config);
  EXPECT_EQ(err, RT_OK) << "rtmsg_rtconnection_CreateWithConfig failed";
  _rtConnection_TaintMessages(1);
  rtMessage_Release(config);
  rtConnection_Destroy(connection);
}

TEST_F(TestServer, rtmsg_rtConnection_CreateWithConfig_test3)
{
  char const*   router_config ="tcp://127.0.0.1:10001";
  rtError       err;
  rtMessage     config;
  rtConnection  connection;
  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "rtsend");
  rtMessage_SetString(config, "uri", router_config);
  rtMessage_SetInt32(config, "start_router", 0);
  err = rtConnection_CreateWithConfig(&connection, config);
  EXPECT_EQ(err, RT_NO_CONNECTION) << "rtmsg_rtconnection_CreateWithConfig failed";
  rtMessage_Release(config);
}

TEST_F(TestServer, rtmsg_rtConnection_SendResponse_test1)
{
  char *name = "sample_test";
  rtMessageHeader const* hdr = (const rtMessageHeader*)name;
  char* buff = "TestName";
  rtError err;
  rtMessage res;

  rtConnection  con;
  rtConnection_Create(&con, "PROVIDER1", "unix:///tmp/rtrouted");
  rtMessage_Create(&res);
  rtMessage_SetString(res, "reply", buff);
  err = rtConnection_SendResponse(con, hdr, res, 1000);
  EXPECT_EQ(err, RT_OK);
  rtMessage_Release(res);
  rtConnection_Destroy(con);
}

TEST_F(TestServer, rtmsg_rtMessage_SetBool_test1)
{
  rtError       err;
  rtMessage     config;
  bool val;
  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "rtsend");
  rtMessage_SetString(config, "uri", "router_config");
  err = rtMessage_SetBool(config, "start_router", true);
  EXPECT_EQ(err, RT_OK) << "rtmessage_SetBool failed";
  err = rtMessage_GetBool(config, "start_router", &val);
  EXPECT_EQ(err, RT_OK) << "rtmessage_GetBool failed";
  //Neg test passing invalid name
  err = rtMessage_GetBool(config, "router", &val);
  EXPECT_EQ(err, RT_FAIL) << "rtmessage_GetBool failed";
  rtMessage_Release(config);
}

TEST_F(TestServer, rtmsg_rtMessage_SetDouble_test1)
{
  rtError       err;
  rtMessage     config;
  double val;
  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "rtsend");
  rtMessage_SetString(config, "uri", "router_config");
  err = rtMessage_SetDouble(config, "start_router", 999.999);
  EXPECT_EQ(err, RT_OK) << "rtmessage_SetDouble failed";
  err = rtMessage_GetDouble(config, "start_router", &val);
  EXPECT_EQ(err, RT_OK) << "rtmessage_GetDouble failed";
  //Neg test passing invalid name
  err = rtMessage_GetDouble(config, "router", &val);
  EXPECT_EQ(err, RT_FAIL) << "rtmessage_GetDouble failed";
  rtMessage_Release(config);
}

TEST_F(TestServer, rtmsg_rtMessage_SetMessage_test1)
{
    rtMessage req = NULL, msg = NULL;
    rtMessage item, p;
    char* s = NULL;
    char val;
    uint32_t n = 0;
    uint32_t size = 0;
    rtError err;
    int32_t paramslen, j=1;
    char *topic = "TEST_SAMPLE";
    char getTopic[50] = "";
    void const* ptr = "SAMPLE_TEST";

    rtMessage_Create(&req);
    rtMessage_SetString(req, "method", "rtsend");
    rtMessage_SetString(req, "provider", "router_config");

    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", "ITEM");
    rtMessage_SetString(item, "value", "Book");

    err = rtMessage_SetMessage(req, "params", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_SetMessage failed";
    err = rtMessage_AddMessage(req, "items", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_AddMessage failed";
    rtMessage_GetArrayLength(item, "params", &paramslen);

    //Neg test passing invalid param
    err = rtMessage_GetMessageItem(item, "name", j, &p);
    EXPECT_EQ(err, RT_FAIL) << "rtMessage_GetMessageItem failed";
    //Neg test passing invalid param
    err = rtMessage_GetMessageItem(item, "params", j, &p);
    EXPECT_EQ(err, RT_PROPERTY_NOT_FOUND) << "rtMessage_GetMessageItem failed";
    err = rtMessage_GetStringValue(req, "method", &val, 10);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetStringValue failed";
    //Neg test passing invalid param
    err = rtMessage_SetMessage(NULL, "params", item);
    EXPECT_EQ(err, RT_ERROR_INVALID_ARG) << "rtMessage_SetMessage failed";
    //Neg test passing invalid param
    err = rtMessage_GetMessage(req, "config", &item);
    EXPECT_EQ(err, RT_PROPERTY_NOT_FOUND) << "rtMessage_GetMessage failed";
    //Neg test passing invalid param
    err = rtMessage_ToString(NULL, &s, &n);
    EXPECT_EQ(err, RT_FAIL) << "rtMessage_ToString failed";

    err = rtMessage_AddBinaryData(req, "sample", ptr, sizeof(ptr));
    EXPECT_EQ(err, RT_OK);
    err = rtMessage_GetBinaryData(req, "sample", (void**)&ptr, (uint32_t*)&size);
    EXPECT_EQ(err, RT_OK);
    err = rtMessage_SetSendTopic(req, topic);
    EXPECT_EQ(err, RT_OK);
    err = rtMessage_GetSendTopic(req, getTopic);
    EXPECT_EQ(err, RT_OK);
 
    EXPECT_EQ(strcmp(getTopic,topic), 0);
    rtMessage_Release(req);
    rtMessage_Release(item);
    free(s);
    if(ptr)
    {
       free((void*)ptr);
    }
}

TEST_F(TestServer, rtmsg_rtMessage_SetMessage_test2)
{
    rtMessage req, msg;
    rtMessage item;
    rtError err;

    rtMessage_Create(&req);
    rtMessage_SetString(req, "method", "rtsend");
    rtMessage_SetString(req, "provider", "router_config");
    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", "ITEM");
    rtMessage_SetString(item, "value", "Book");
    err = rtMessage_SetMessage(req, "params", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_SetMessage failed";
    err = rtMessage_AddMessage(req, "items", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_AddMessage failed";
    err = rtMessage_GetMessage(req, "params", &msg);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetMessage failed";
    rtMessage_Release(msg);
    rtMessage_Release(req);
    rtMessage_Release(item);
}

TEST_F(TestServer, rtmsg_rtMessage_SetMessage_test3)
{
    rtMessage req, msg;
    rtMessage item, p;
    rtError err;
    int32_t paramslen, j=1;

    rtMessage_Create(&req);
    rtMessage_SetString(req, "method", "rtsend");
    rtMessage_SetString(req, "provider", "router_config");
    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", "ITEM");
    rtMessage_SetString(item, "value", "Book");
    err = rtMessage_SetMessage(req, "params", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_SetMessage failed";
    err = rtMessage_AddMessage(req, "items", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_AddMessage failed";
    err = rtMessage_GetArrayLength(item, "params", &paramslen);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetArrayLength failed";
    err = rtMessage_GetMessageItem(req, "params", j, &p);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetMessage failed";
    rtMessage_Release(p);
    rtMessage_Release(req);
    rtMessage_Release(item);
}

TEST_F(TestServer, rtmsg_rtMessage_Retain_test1)
{
  rtError err;
  rtMessage msg;

  rtMessage_Create(&msg);
  err = rtMessage_Retain(msg);
  EXPECT_EQ(err, RT_OK);
  rtMessage_Release(msg);
  if(msg)
     rtMessage_Release(msg);
}

TEST_F(TestServer, rtmsg_rtMessage_Clone_test1)
{
  rtError err;
  rtMessage msg, cpy;

  rtMessage_Create(&msg);
  err = rtMessage_Clone(msg, &cpy);
  EXPECT_EQ(err, RT_OK);
  rtMessage_Release(msg);
  rtMessage_Release(cpy);
}

TEST_F(TestServer, rtmsg_rtMessage_toByteArray_test1)
{
    rtError err;
    rtMessage req, item;
    uint8_t* buffer = NULL;
    uint32_t size;
    rtMessage_Create(&req);
    rtMessage_SetString(req, "method", "rtsend");
    rtMessage_SetString(req, "provider", "router_config");
    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", "ITEM");
    rtMessage_SetString(item, "value", "Book");
    err = rtMessage_SetMessage(req, "params", item);
    EXPECT_EQ(err, RT_OK) << "rtMessage_SetMessage failed";
    err = rtMessage_ToByteArray(req, &buffer, &size);
    EXPECT_EQ(err,RT_OK) << "rtMessage_ToByteArray failed";
    free(buffer);
    rtMessage_Release(req);
    rtMessage_Release(item);
}

TEST_F(TestServer, rtmsg_rtError_test1)
{
  rtError err = RT_OK;
  const char *s = NULL;
  rtErrorSetLastError(err);
  err = rtErrorGetLastError();
  EXPECT_EQ(err, RT_OK);
  rtStrError(err);
  err = RT_FAIL;
  rtStrError(err);
  err = RT_ERROR_NOT_ENOUGH_ARGS;
  rtStrError(err);
  err = RT_PROP_NOT_FOUND;
  rtStrError(err);
  err = RT_OBJECT_NOT_INITIALIZED;
  rtStrError(err);
  err = RT_PROPERTY_NOT_FOUND;
  rtStrError(err);
  err = RT_RESOURCE_NOT_FOUND;
  rtStrError(err);
  err = RT_NO_CONNECTION;
  rtStrError(err);
  err = RT_ERROR_NOT_IMPLEMENTED;
  rtStrError(err);
  err = RT_ERROR_TYPE_MISMATCH;
  rtStrError(err);
  err = RT_ERROR_TIMEOUT;
  rtStrError(err);
  err = RT_ERROR_DUPLICATE_ENTRY;
  rtStrError(err);
  err = RT_ERROR_OBJECT_NOT_FOUND;
  rtStrError(err);
  err = RT_ERROR_PROTOCOL_ERROR;
  rtStrError(err);
  err = RT_ERROR_INVALID_OPERATION;
  rtStrError(err);
  err = RT_ERROR_IN_PROGRESS;
  rtStrError(err);
  err = RT_ERROR_QUEUE_EMPTY;
  rtStrError(err);
  err = RT_ERROR_STREAM_CLOSED;
  rtStrError(err);
}

TEST_F(TestServer, rtmsg_rtList_test1)
{
  rtList list;
  rtListItem items[6];
  rtListItem item, prev;
  rtError err;
  void* data;
  void* ptr;
  data = rt_malloc(100);

  rtList_Create(&list);
  err = rtList_PushFront(list, (void*)2, &items[2]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushFront(list, (void*)1, &items[1]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushFront(list, (void*)0, &items[0]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushBack(list, (void*)3, &items[3]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushBack(list, (void*)4, &items[4]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushBack(list, (void*)5, &items[5]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_RemoveItem(list, items[0], NULL);
  EXPECT_EQ(err, RT_OK);
  err = rtList_InsertBefore(list, (void*)0, items[1], &items[0]);
  EXPECT_EQ(err, RT_OK);
  err = rtList_InsertAfter(list, (void*)0, items[2], &items[3]);
  EXPECT_EQ(err, RT_OK);
  err = rtListItem_SetData(items[4], data);
  EXPECT_EQ(err, RT_OK);
  err = rtListItem_GetData(items[4], &data);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushFront(list, (void*)-1, &item);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushFront(list, (void*)-2, &item);
  EXPECT_EQ(err, RT_OK);
  err = rtList_PushFront(list, (void*)-3, &item);
  EXPECT_EQ(err, RT_OK);
  err = rtListItem_GetPrev(item, &prev);
  EXPECT_EQ(err, RT_OK);
  err = rtList_GetBack(list, &item);
  EXPECT_EQ(err, RT_OK);
  err = rtList_RemoveAllItems(list, NULL);
  EXPECT_EQ(err, RT_OK);
  rtList_Destroy(list, NULL);
  rt_free(data);
}

TEST_F(TestServer, rtmsg_rtLog_test1)
{
   rtLogLevel level;
   rtLoggerSelection opt;
   rtLogHandler sloghandler, handler = NULL;

   rtLog_SetLevel(RT_LOG_INFO);
   level = rtLog_GetLevel();
   EXPECT_EQ(level, RT_LOG_INFO) << "rtLog_SetLevel failed";
   rtLog_SetLevel(rtLogLevelFromString("debug"));
   rtLog_SetLevel(rtLogLevelFromString("warn"));
   rtLog_SetLevel(rtLogLevelFromString("info"));
   rtLog_SetLevel(rtLogLevelFromString("error"));
   rtLog_SetLevel(rtLogLevelFromString("fatal"));
   rtLog_SetOption(RT_USE_RDKLOGGER);
   opt = rtLog_GetOption();
   EXPECT_EQ(opt, RT_USE_RDKLOGGER) << "rtLog_GetOption failed";
   rtLogSetLogHandler(NULL);
   sloghandler = rtLogGetLogHandler();
   EXPECT_EQ(sloghandler, handler);
}
