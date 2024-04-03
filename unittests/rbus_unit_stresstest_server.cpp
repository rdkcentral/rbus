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
#include <signal.h>
extern "C" {
#include "rbuscore.h"

}
#include "gtest_app.h"
#include "rbus_test_util.h"

#define DEFAULT_RESULT_BUFFERSIZE 128
#define BUS_LATENCY_MARGIN 700
#define MIN_WAIT_TIME_DIFFERENCE 750
#define MAX_SERVER_NAME 20
#define MAX_ELEMENT_NAME_LENGTH 25

static test_array_data_t client_data;

static bool OPEN_BROKER_CONNECTION(char* connection_name)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbus_openBrokerConnection(connection_name)) == RBUSCORE_SUCCESS)
    {
         //printf("Successfully connected to bus.\n");
         result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_openBrokerConnection failed";
    return result;
}

static bool CLOSE_BROKER_CONNECTION()
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbus_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        //printf("Successfully disconnected from bus.\n");
        result = true;
    }
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_closeBrokerConnection failed";
    return result;
}

static void handle_term(int sig)
{
    (void) sig;
    CLOSE_BROKER_CONNECTION();
    printf("**********EXITING SERVER ******************** \n");
    kill(getpid(), SIGKILL);
}

static void CREATE_RBUS_SERVER_INSTANCE(int handle)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term);
    reset_stored_data();

    conn_status = OPEN_BROKER_CONNECTION(server_name);

    ASSERT_EQ(conn_status, true) << "OPEN_BROKER_CONNECTION failed";

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", server_name);
    //printf("Registering object %s\n", buffer);

    err = rbus_registerObj(buffer, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1}};

    err = rbus_registerMethodTable(buffer, table, 2);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";

    //pause();
    //printf("**********EXITING SERVER : %s ******************** \n", server_name);
    return;
}

static bool RBUS_PULL_OBJECT(char* expected_data, char* server_obj, rbusCoreError_t expected_err)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    if((err = rbus_pullObj(server_obj, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        rbusMessage_GetString(response, &buff);
        if((NULL != expected_data) && (NULL != buff))
            EXPECT_STREQ(buff, expected_data) << "rbus_pullObj failed to procure expected string";
        else
            EXPECT_EQ(expected_data, buff) << "rbus_pullObj failed";
        rbusMessage_Release(response);
        result = true;
    }
    else
    {
        printf("Could not pull object %s\n", server_obj);
    }
    EXPECT_EQ(err, expected_err) << "rbus_pullObj failed";
    return result;
}

static bool RBUS_PUSH_OBJECT(char* data, char* server_obj, rbusCoreError_t expected_err)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, data);
    err = rbus_pushObj(server_obj, setter, 1000);
    EXPECT_EQ(err, expected_err) << "rbus_pushObj failed";
    return true;
}

static int event_subscribe_callback(char const* object,  char const* eventName, char const* listener, int added, const rbusMessage payload, void* userData)
{
    printf("In event subscribe callback for object %s, event %s.\n", object, eventName);
    return 0;
}

static void _client_disconnect_callback_handler(char const* listener)
{
    printf("******_client_disconnect_callback_handler called******\n");
    fflush(stdout);
}

::testing::AssertionResult doesStringMatch(const char * ipString, const char *resultstring, int stringCount)
{
    if ((NULL != ipString) && (NULL != resultstring) && (0 != stringCount))
    {
        if(!strncmp(ipString, resultstring, stringCount))
        {
            return ::testing::AssertionSuccess();
        }
    }
    return ::testing::AssertionFailure() << "String is not in the list";
}

static void resolveWildcardExpression(const char* expression, int expected_entries, char result_array[][MAX_ELEMENT_NAME_LENGTH])
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char ** destinations = NULL;
    int num_entries = 0;

    err = rbus_discoverWildcardDestinations(expression, &num_entries, &destinations);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_discoverWildcardDestinations failed";

    if(err == RBUSCORE_SUCCESS)
    {
        printf("Query for expression %s was successful.\n No. of entries : %d \n", expression, num_entries);
        EXPECT_EQ(num_entries, expected_entries) << "rbus_discoverWildcardDestinations failed";
        for(int i = 0; i < num_entries; i++)
        {
            printf("Destination %d is %s\n", i, destinations[i]);
            if(i < expected_entries)
                strncpy(*(result_array + i), destinations[i], MAX_ELEMENT_NAME_LENGTH);
            free(destinations[i]);
        }
        if(destinations != NULL)
            free(destinations);
    }
    return;
}

class StressTestServer : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for StressTestServer\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for StressTestServer\n");
}

};

TEST_F(StressTestServer, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(StressTestServer, createServer_test1)
{
    int counter = 1;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(5);
        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, createServer_test2)
{
    int counter = 2;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, dataPushPull_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        RBUS_PUSH_OBJECT(test_string, server_obj, RBUSCORE_SUCCESS);
        RBUS_PULL_OBJECT(test_string, server_obj, RBUSCORE_SUCCESS);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

/*Continuous push-pull calls to server object*/
TEST_F(StressTestServer, dataPushPull_test2)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_4.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        for(i = 0; i < 100; i++)
        {
            RBUS_PUSH_OBJECT(test_string, server_obj, RBUSCORE_SUCCESS);
            RBUS_PULL_OBJECT(test_string, server_obj, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

/*Push-Pull calls to invalid server object*/
TEST_F(StressTestServer, dataPushPull_test3)
{
    int counter = 4;
    char client_name[] = "TEST_CLIENT_1";
    char test_server_obj[] = "test_server_invalid.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        RBUS_PUSH_OBJECT(test_string, test_server_obj, RBUSCORE_ERROR_ENTRY_NOT_FOUND);
        RBUS_PULL_OBJECT(test_string, test_server_obj, RBUSCORE_ERROR_ENTRY_NOT_FOUND);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_resolveWildcardDestination_test1)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "Test_Obj1";
    char server_obj2[] = "Test_Obj2";
    char server_obj3[] = "Test_Obj3";
    int obj1_element_count = 9;
    int obj2_element_count = 9;
    int obj3_element_count = 3;
    char obj1_elements[][MAX_ELEMENT_NAME_LENGTH] = { "test.dac1.box1",
                                                      "test.dac1.box2",
                                                      "test.dac1.box3",
                                                      "test.dac2.box1",
                                                      "test.dac2.box2",
                                                      "test.dac2.box3",
                                                      "test.dac3.box1",
                                                      "test.dac3.box2",
                                                      "test.dac3.box3"};
    char obj2_elements[][MAX_ELEMENT_NAME_LENGTH] = { "test.dac1.cam1",
                                                      "test.dac1.cam2",
                                                      "test.dac1.cam3",
                                                      "test.dac2.cam1",
                                                      "test.dac2.cam2",
                                                      "test.dac2.cam3",
                                                      "test.dac3.cam1",
                                                      "test.dac3.cam2",
                                                      "test.dac3.cam3"};
    char obj3_elements[][MAX_ELEMENT_NAME_LENGTH] = { "test.dac1.gw1",
                                                      "test.dac2.gw1",
                                                      "test.dac3.gw1"};
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);

        err = rbus_registerObj(server_obj1, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj1_element_count; i++)
        {
            err = rbus_addElement(server_obj1, *(obj1_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj2, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj2_element_count; i++)
        {
            err = rbus_addElement(server_obj2, *(obj2_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj3, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj3_element_count; i++)
        {
            err = rbus_addElement(server_obj3, *(obj3_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        char result_array [3][MAX_ELEMENT_NAME_LENGTH] = {0};
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        resolveWildcardExpression("test.", 3, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 2), server_obj3, strlen(server_obj3)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac1.", 3, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 2), server_obj3, strlen(server_obj3)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac2.", 3, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 2), server_obj3, strlen(server_obj3)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac3.", 3, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 2), server_obj3, strlen(server_obj3)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac1.box1", 0, result_array);

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac1.gw1", 0, result_array);

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac3.cam3", 0, result_array);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_resolveWildcardDestination_test2)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "Test_Obj1";
    char server_obj2[] = "Test_Obj2";
    int obj1_element_count = 3;
    int obj2_element_count = 2;
    char obj1_elements[][MAX_ELEMENT_NAME_LENGTH] = { "test.dac1.box1",
                                                      "test.dac2.box1",
                                                      "test.dac3.box3"};
    char obj2_elements[][MAX_ELEMENT_NAME_LENGTH] = { "test.dac1.cam1",
                                                      "test.dac3.cam3"};
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);

        err = rbus_registerObj(server_obj1, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj1_element_count; i++)
        {
            err = rbus_addElement(server_obj1, *(obj1_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj2, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj2_element_count; i++)
        {
            err = rbus_addElement(server_obj2, *(obj2_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        char result_array [3][MAX_ELEMENT_NAME_LENGTH] = {0};
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        resolveWildcardExpression("test.", 2, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_resolveWildcardDestination_test3)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "Test_Obj1";
    char server_obj2[] = "Test_Obj2";
    int obj1_element_count = 4;
    int obj2_element_count = 2;
    char obj1_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj1.foo.1",
                                                      "global.obj1.foo.2",
                                                      "global.obj1.bar.1",
                                                      "global.testelement"};
    char obj2_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj2.foo.1",
                                                      "global.obj2.bar.1"};
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);

        err = rbus_registerObj(server_obj1, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj1_element_count; i++)
        {
            err = rbus_addElement(server_obj1, *(obj1_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj2, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj2_element_count; i++)
        {
            err = rbus_addElement(server_obj2, *(obj2_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        char result_array [3][MAX_ELEMENT_NAME_LENGTH] = {0};
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        resolveWildcardExpression("global.", 2, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_resolveWildcardDestination_test4)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "Test_Obj1";
    char server_obj2[] = "Test_Obj2";
    int obj1_element_count = 3;
    int obj2_element_count = 2;
    char obj1_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj1.foo.1",
                                                      "global.obj1.foo.2",
                                                      "global.obj1.bar.1"};
    char obj2_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj2.foo.1",
                                                      "global.obj2.bar.1"};
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);

        err = rbus_registerObj(server_obj1, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj1_element_count; i++)
        {
            err = rbus_addElement(server_obj1, *(obj1_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj2, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj2_element_count; i++)
        {
            err = rbus_addElement(server_obj2, *(obj2_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        char result_array [3][MAX_ELEMENT_NAME_LENGTH] = {0};
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        resolveWildcardExpression("global.", 2, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_resolveWildcardDestination_test5)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj1[] = "Test_Obj1";
    char server_obj2[] = "Test_Obj2";
    int obj1_element_count = 4;
    int obj2_element_count = 2;
    char obj1_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj1.foo.1",
                                                      "global.obj1.foo.2",
                                                      "test.dac2.box1",
                                                      "global.obj1.bar.1"};
    char obj2_elements[][MAX_ELEMENT_NAME_LENGTH] = { "global.obj2.foo.1",
                                                      "global.obj2.bar.1"};
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);

        err = rbus_registerObj(server_obj1, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj1_element_count; i++)
        {
            err = rbus_addElement(server_obj1, *(obj1_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }

        err = rbus_registerObj(server_obj2, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        for (i = 0; i < obj2_element_count; i++)
        {
            err = rbus_addElement(server_obj2, *(obj2_elements + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        char result_array [3][MAX_ELEMENT_NAME_LENGTH] = {0};
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        resolveWildcardExpression("global.", 2, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));
        EXPECT_TRUE(doesStringMatch(*(result_array + 1), server_obj2, strlen(server_obj2)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.", 1, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("test.dac2.", 1, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj1, strlen(server_obj1)));

        memset(result_array, 0, sizeof(result_array));
        resolveWildcardExpression("global.obj2.", 1, result_array);
        EXPECT_TRUE(doesStringMatch(*(result_array), server_obj2, strlen(server_obj2)));

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_addElement_test1)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_4.obj1";
    char server_element[] = "server_element4.x";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_addElement(server_obj,server_element);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        for(i = 0; i < 100; i++)
        {
            RBUS_PUSH_OBJECT(test_string, server_element, RBUSCORE_SUCCESS);
            RBUS_PULL_OBJECT(test_string, server_element, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbusMessage_GetElementsAddedByObject_test1)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_5.obj1";
    char server_element[] = "test.1.box1";
    bool conn_status = false;
    rtError err = RT_OK;
    int num_objects;
    char** objects;
    int i;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_addElement(server_obj,server_element);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        //Neg test discovering Object before connection
        err = rbus_discoverObjectElements("test.", NULL, &objects);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbusMessage_discoverObjectElements failed";
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        //Neg test passing NULL as count
        err = rbus_discoverObjectElements("test.", NULL, &objects);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbusMessage_discoverObjectElements failed";
        err = rbus_discoverObjectElements("test.", &num_objects, &objects);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbusMessage_discoverObjectElements failed";

        if(objects != NULL){
            for(i = 0; i < num_objects; ++i)
                free(objects[i]);
            free(objects);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_discoverElementObjects_test1)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_5.obj1";
    char server_element[] = "test.1.box1";
    bool conn_status = false;
    rtError err = RT_OK;
    int num_objects;
    char** objects;
    int i;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_addElement(server_obj,server_element);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        err = rbus_discoverElementObjects(server_element, &num_objects, &objects);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbusMessage_discoverElementObjects failed";

        for(i = 0; i < num_objects; ++i)
            free(objects[i]);
        free(objects);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_discoverElementObjects_test2)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rtError err = RT_OK;
    int num_objects;
    char** objects;
    int i;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        //Testing with element to be NULL
        err = rbus_discoverElementObjects(NULL, &num_objects, &objects);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbusMessage_discoverElementObjects failed";

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_discoverElementsObjects_test1)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_5.obj1";
    const char *server_element[] = {"test.1.box1","test.1.box2","test.1.box3"};
    bool conn_status = false;
    rtError err = RT_OK;
    int num_elements = 3;
    int num_objects;
    char** objects;
    int i;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        for(i=0; i < num_elements; i++)
        {
            err = rbus_addElement(server_obj,*(server_element + i));
            EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        }
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0){
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        err = rbus_discoverElementsObjects(num_elements, server_element, &num_objects, &objects);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbusMessage_discoverElementsObjects failed";

        for(i = 0; i < num_objects; ++i)
            free(objects[i]);
        free(objects);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_discoverElementsObjects_test2)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rtError err = RT_OK;
    int num_elements = 3;
    int num_objects;
    char** objects;
    int i;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0){
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        //Testing with elements to be NULL
        err = rbus_discoverElementsObjects(num_elements, NULL, &num_objects, &objects);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbusMessage_discoverElementsObjects failed";

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_addElementEvent_test1)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_4.obj1";
    char server_event[] = "test_server.Event!";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    char data[] = "data";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("******Registering Event %s with object %s****** \n", server_event, server_obj);
        err = rbus_registerEvent(server_obj,server_event,sub1_callback,data);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
        printf("*******Adding Event %s as an Element using rbus_addElementEvent******\n", server_event);
        err = rbus_addElementEvent(server_obj,server_event);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElementEvent failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        for(i = 0; i < 100; i++)
        {
            RBUS_PUSH_OBJECT(test_string, server_event, RBUSCORE_SUCCESS);
            RBUS_PULL_OBJECT(test_string, server_event, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_addElementEvent_test2)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_4.obj1";
    char server_event[] = "test_server.Event!";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    char data[] = "data";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("******Registering Event %s with object %s****** \n", server_event, server_obj);
        err = rbus_registerEvent(server_obj,server_event,sub1_callback,data);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
        printf("*******Adding Event %s as an Element using rbus_addElementEvent******\n", server_event);
        err = rbus_addElementEvent(server_obj,server_event);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElementEvent failed";
        //Testing Duplicate Entry
        err = rbus_addElementEvent(server_obj,server_event);
        EXPECT_EQ(err, RBUSCORE_ERROR_DUPLICATE_ENTRY) << "rbus_addElementEvent failed";
        err = rbus_addElementEvent(server_obj,server_event);
        EXPECT_EQ(err, RBUSCORE_ERROR_DUPLICATE_ENTRY) << "rbus_addElementEvent failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        for(i = 0; i < 100; i++)
        {
            RBUS_PUSH_OBJECT(test_string, server_event, RBUSCORE_SUCCESS);
            RBUS_PULL_OBJECT(test_string, server_event, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_registerClientDisconnectHandler_test1)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        rbus_registerClientDisconnectHandler(_client_disconnect_callback_handler);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerClientDisconnectHandler failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        if(conn_status)
            CLOSE_BROKER_CONNECTION();
        sleep(1);
        kill(pid,SIGTERM);
        printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
    rbus_unregisterClientDisconnectHandler();
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterClientDisconnectHandler failed";
}

TEST_F(StressTestServer, rbus_removeElement_test1)
{
    int counter = 4, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_4.obj1";
    char server_element[] = "server_element4.x";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_addElement(server_obj,server_element);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
        err = rbus_removeElement(server_obj,server_element);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_removeElement failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        for(i = 0; i < 100; i++)
        {
            RBUS_PUSH_OBJECT(test_string, server_element, RBUSCORE_ERROR_ENTRY_NOT_FOUND);
            RBUS_PULL_OBJECT(test_string, server_element, RBUSCORE_ERROR_ENTRY_NOT_FOUND);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_unregisterMethod_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    char server_init_test_string[] = "init init init";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        reset_stored_data();
        err = rbus_unregisterMethod(server_obj,METHOD_SETPARAMETERVALUES);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        /*As set parameter value is unregistered, pull object will retrive only initializing string*/
        RBUS_PUSH_OBJECT(test_string, server_obj, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
        RBUS_PULL_OBJECT(server_init_test_string, server_obj, RBUSCORE_SUCCESS);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_unregisterMethodTable_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    char server_init_test_string[] = "init init init";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        reset_stored_data();

        rbus_method_table_entry_t table[1] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}};
        err = rbus_unregisterMethodTable(server_obj, table, 1);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethodTable failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        /*As set parameter rpc is unregistered, pull object will retrive only initializing string*/
        RBUS_PUSH_OBJECT(test_string, server_obj, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
        RBUS_PULL_OBJECT(server_init_test_string, server_obj, RBUSCORE_SUCCESS);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_unregisterMethodTable_test2)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    char test_string[] = "rbus_client_test_string";
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        reset_stored_data();
        rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1}};
        err = rbus_unregisterMethodTable(server_obj, table, 2);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethodTable failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        RBUS_PUSH_OBJECT(test_string, server_obj, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
        RBUS_PULL_OBJECT(NULL, server_obj, RBUSCORE_ERROR_UNSUPPORTED_METHOD);

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_registerMethod_test1)
{
    int counter = 3, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_GET_BINARY_RPC,handle_getBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        err = rbus_registerMethod(server_obj,METHOD_SET_BINARY_RPC,handle_setBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);


        client_data.count = 50;
        for(i = 0; i < client_data.count; i++)
            client_data.numerals[i] = i;

        rbusMessage setter;
        rbusMessage response = NULL;
        rbusMessage_Init(&setter);
        rbusMessage_SetBytes(setter, (uint8_t*)&client_data, sizeof(client_data));

        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_BINARY_RPC, setter, 1000, &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation failed";
        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if((err = rbus_invokeRemoteMethod(server_obj, METHOD_GET_BINARY_RPC, NULL, 1000, &response)) == RBUSCORE_SUCCESS)
        {
            unsigned int size = 0;
            const test_array_data_t * data_procurred = NULL;
            int result;
            rbusMessage_GetInt32(response, &result);
            rbusMessage_GetBytes(response, (const uint8_t **)&data_procurred, &size);
            if(NULL != data_procurred)
            {
                for(i = 0; i < data_procurred->count;i++)
                {
                    EXPECT_EQ(i, *(data_procurred->numerals + i)) << "Corrupted Data";
                }
            }
            rbusMessage_Release(response);
        }
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation to get data failed";
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_unregisterMethod_test2)
{
    int counter = 3, i = 0;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_GET_BINARY_RPC,handle_getBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        err = rbus_registerMethod(server_obj,METHOD_SET_BINARY_RPC,handle_setBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        err = rbus_unregisterMethod(server_obj,METHOD_SET_BINARY_RPC);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);


        client_data.count = 50;
        for(i = 0; i < client_data.count; i++)
            client_data.numerals[i] = i;

        rbusMessage setter;
        rbusMessage response;
        rbusMessage_Init(&setter);
        rbusMessage_SetBytes(setter, (uint8_t *)&client_data, sizeof(client_data));

        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_BINARY_RPC, setter, 1000, &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation failed";
        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if((err = rbus_invokeRemoteMethod(server_obj, METHOD_GET_BINARY_RPC, NULL, 1000, &response)) == RBUSCORE_SUCCESS)
        {
            unsigned int size = 0;
            const test_array_data_t * data_procurred = NULL;
            int result;
            rbusMessage_GetInt32(response, &result);
            rbusMessage_GetBytes(response, (const uint8_t **)&data_procurred, &size);
            if(NULL != data_procurred)
            {
                for(i = 0; i < data_procurred->count;i++)
                {
                    EXPECT_EQ(i, *(data_procurred->numerals + i)) << "Corrupted Data";
                }
            }
            rbusMessage_Release(response);
        }
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation to get data failed";
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(StressTestServer, rbus_invokeMethodWithTimeout_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_TIMEOUT_RPC,handle_timeout,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        rbusCoreError_t err = RBUSCORE_SUCCESS;
        rbusMessage setter;
        rbusMessage response;
        rbusMessage_Init(&setter);
        int timeout = 2;
        rbusMessage_SetString(setter, "rbus_invokeMethodWithTimeout_test1");
        //printf("Set test_name : %s \n","rbus_invokeMethodWithTimeout_test1");
        rbusMessage_SetInt32(setter, timeout);
        //printf("Set time out : %d  \n", timeout);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_TIMEOUT_RPC, setter, ((timeout * 1000) + BUS_LATENCY_MARGIN), &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation failed";

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}

TEST_F(StressTestServer, rbus_invokeMethodWithTimeout_test2)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_TIMEOUT_RPC,handle_timeout,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        rbusCoreError_t err = RBUSCORE_SUCCESS;
        rbusMessage setter;
        rbusMessage response;
        rbusMessage_Init(&setter);
        int timeout = 10;
        rbusMessage_SetString(setter, "rbus_invokeMethodWithTimeout_test2");
        rbusMessage_SetInt32(setter, timeout);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_TIMEOUT_RPC, setter,
                                      ((timeout * 1000) + BUS_LATENCY_MARGIN), &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation failed";

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}

TEST_F(StressTestServer, rbus_invokeMethodWithTimeout_test3)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_TIMEOUT_RPC,handle_timeout,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        rbusCoreError_t err = RBUSCORE_SUCCESS;
        rbusMessage setter;
        rbusMessage response;
        rbusMessage_Init(&setter);
        int timeout = 5;  // In seconds
        int waitTime = timeout - 2;
        rbusMessage_SetString(setter, "rbus_invokeMethodWithTimeout_test3");
        rbusMessage_SetInt32(setter, timeout);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_TIMEOUT_RPC, setter,
                                      ((waitTime * 1000) + BUS_LATENCY_MARGIN), &response);
        EXPECT_EQ(err, RBUSCORE_ERROR_REMOTE_TIMED_OUT) << "Expected time out error. But we got some thing different";
        //EXPECT_EQ(err, RBUSCORE_ERROR_GENERAL) << "Expected time out error. But we got some thing different";

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}
TEST_F(StressTestServer, rbus_invokeMethodWithTimeout_test4)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_TIMEOUT_RPC,handle_timeout,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);

        rbusCoreError_t err = RBUSCORE_SUCCESS;
        rbusMessage setter;
        rbusMessage response;
        rbusMessage_Init(&setter);
        int timeout = 5;  // In seconds
        rbusMessage_SetString(setter, "rbus_invokeMethodWithTimeout_test4");
        rbusMessage_SetInt32(setter, timeout);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_TIMEOUT_RPC, setter,
                                      ((timeout * 1000) - MIN_WAIT_TIME_DIFFERENCE + BUS_LATENCY_MARGIN), &response);
        EXPECT_EQ(err, RBUSCORE_ERROR_REMOTE_TIMED_OUT) << "Expected time out error. But we got some thing different";
        //EXPECT_EQ(err, RBUSCORE_ERROR_GENERAL) << "Expected time out error. But we got some thing different";

        if(NULL != response)
        {
            rbusMessage_Release(response);
            response = NULL;
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}

TEST_F(StressTestServer, rbus_invokeMethodMsgSize_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_BINARY_DATA_SIZE_RPC,handle_setBinaryDataSize,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
        err = rbus_registerMethod(server_obj,METHOD_GET_LARGE_BINARY_RPC,handle_getLargeBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        rbusMessage response;
        rbusMessage setter;
        int data_chunk_size = 4;
        char chunk_name[10] = {0};
        int i = 0;

        rbusMessage_Init(&setter);
        rbusMessage_SetInt32(setter, data_chunk_size);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_BINARY_DATA_SIZE_RPC, setter, 1000, &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC - handle_setBinaryDataSize invocation failed";
        rbusMessage_Release(response);

        if((err = rbus_invokeRemoteMethod(server_obj, METHOD_GET_LARGE_BINARY_RPC, NULL, 1000, &response)) == RBUSCORE_SUCCESS)
        {
            unsigned int size = 0;
            for(i = 1; i <= data_chunk_size; i++)
            {
                const unsigned char * data_procurred = NULL;
                snprintf(chunk_name, 10, "chunk%d", i);
                rbusMessage_GetBytes(response, (const uint8_t **)&data_procurred, &size);
                if(NULL != data_procurred)
                {
                    EXPECT_EQ(size, 1024) << "Data chunk size error";
                    EXPECT_EQ(*(data_procurred), (i + '0')) << "Data error";
                }
            }
            rbusMessage_Release(response);
        }
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation to get data failed";

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}

TEST_F(StressTestServer, rbus_invokeMethodMsgSize_test2)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_3.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        err = rbus_registerMethod(server_obj,METHOD_SET_BINARY_DATA_SIZE_RPC,handle_setBinaryDataSize,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
        err = rbus_registerMethod(server_obj,METHOD_GET_LARGE_BINARY_RPC,handle_getLargeBinaryData,NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethod failed";
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        rbusMessage response;
        rbusMessage setter;
        int data_chunk_size = 8;
        char chunk_name[10] = {0};
        int i = 0;

        rbusMessage_Init(&setter);
        rbusMessage_SetInt32(setter, data_chunk_size);
        err = rbus_invokeRemoteMethod(server_obj, METHOD_SET_BINARY_DATA_SIZE_RPC, setter, 1000, &response);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC - handle_setBinaryDataSize invocation failed";
        rbusMessage_Release(response);

        if((err = rbus_invokeRemoteMethod(server_obj, METHOD_GET_LARGE_BINARY_RPC, NULL, 1000, &response)) == RBUSCORE_SUCCESS)
        {
            unsigned int size = 0;
            const unsigned char * data_procurred = NULL;
            for(i = 1; i <= data_chunk_size; i++)
            {
                snprintf(chunk_name, 10, "chunk%d", i);
                rbusMessage_GetBytes(response, (const uint8_t **)&data_procurred, &size);
                if(NULL != data_procurred)
                {
                    EXPECT_EQ(size, 1024) << "Data chunk size error";
                    EXPECT_EQ(*(data_procurred), (i + '0')) << "Data error";
                }
            }
            rbusMessage_Release(response);
        }
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "RPC invocation to get data failed";

        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else
    {
        printf("fork failed.\n");
        return;
    }
}

TEST_F(StressTestServer, rbus_registerSubscribeHandler_test1)
{
    int counter = 3;
    char client_name[] = "TEST_CLIENT_1";
    char server_obj[] = "test_server_5.obj1";
    bool conn_status = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE(counter);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(4);
        conn_status = OPEN_BROKER_CONNECTION(client_name);
        err = rbus_registerObj(server_obj, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";
        //Neg test passing object name as NULL
        err = rbus_registerSubscribeHandler(NULL, event_subscribe_callback, NULL);
        EXPECT_EQ(err,RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerSubscribeHandler failed";
        //Neg test passing invalid object name
        err = rbus_registerSubscribeHandler("obj_name", event_subscribe_callback, NULL);
        EXPECT_EQ(err,RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerSubscribeHandler failed";
        err = rbus_registerSubscribeHandler(server_obj, event_subscribe_callback, NULL);
        EXPECT_EQ(err,RBUSCORE_SUCCESS) << "rbus_registerSubscribeHandler failed";
        if(conn_status)
            CLOSE_BROKER_CONNECTION();

        kill(pid,SIGTERM);
        return;
    }
    else{
        printf("fork failed \n");
    }
}
