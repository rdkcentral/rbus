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
#define MAX_SERVER_NAME 20
#define OBJECT_NAME_SIZE 25
#define ELEMENT_NAME_SIZE 35
#define DATA_LENGTH 25

char test_buffer[100][100] = {};

static bool OPEN_BROKER_CONNECTION1(char* connection_name)
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

static bool CLOSE_BROKER_CONNECTION1()
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

static void handle_term1(int sig)
{
    (void) sig;
    CLOSE_BROKER_CONNECTION1();
    printf("**********EXITING SERVER ******************** \n");
    kill(getpid(), SIGKILL);
}
static void CREATE_RBUS_SERVER_INSTANCE1(int handle, int obj_count)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char obj_name[20] = "student_info";
    int i = 1;
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    memset( test_buffer, 0, sizeof(test_buffer));
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term1);
    reset_stored_data();

    conn_status = OPEN_BROKER_CONNECTION1(server_name);

    ASSERT_EQ(conn_status, true) << "OPEN_BROKER_CONNECTION1 failed";

    for(i = 1; i < obj_count; i++)
    {
        snprintf(buffer, (sizeof(buffer) - 1), "%s.obj%d", obj_name, i);
        printf("Registering object %s\n", buffer);
        strncpy(*(test_buffer + i), buffer, 100);

        err = rbus_registerObj(buffer, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

        rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)(test_buffer + i), handle_setStudentInfo}, {METHOD_GETPARAMETERVALUES, (void *)(test_buffer + i), handle_getStudentInfo}};

        err = rbus_registerMethodTable(buffer, table, 2);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";
    }
    return;
}

static void CREATE_RBUS_SERVER_ELEMENTS(int handle, int element_count)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char obj_name[20] = "student_info.obj";
    int i = 1;
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    memset( test_buffer, 0, sizeof(test_buffer));
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term1);
    reset_stored_data();

    conn_status = OPEN_BROKER_CONNECTION1(server_name);

    ASSERT_EQ(conn_status, true) << "OPEN_BROKER_CONNECTION1 failed";

    printf("Registering object %s\n", obj_name);

    err = rbus_registerObj(obj_name, callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)(test_buffer), handle_set1}, {METHOD_GETPARAMETERVALUES, (void *)(test_buffer), handle_get1}};

    err = rbus_registerMethodTable(obj_name, table, 2);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";

    for(i = 1; i < element_count; i++)
    {
        memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
        snprintf(buffer, (sizeof(buffer) - 1), "%s.element%d", obj_name, i);
        printf("Adding element %s\n", buffer);
        err = rbus_addElement(obj_name, buffer);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_addElement failed";
    }
    return;
}

static bool RBUS_PULL_OBJECT1(const char* expected_data, char* server_obj, rbusCoreError_t expected_err)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    //printf("pulling data from : %s \n", server_obj);
    if((err = rbus_pullObj(server_obj, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        rbusMessage_GetString(response, &buff);
        printf("%s: rbus pull returned : %s \n", __FUNCTION__, buff);
        EXPECT_STREQ(buff, expected_data) << "rbus_pullObj failed to procure expected string";
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

static bool RBUS_PUSH_OBJECT1(char* data, char* server_obj, rbusCoreError_t expected_err)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, data);
    //printf("pushing data %s to : %s \n", data, server_obj);
    err = rbus_pushObj(server_obj, setter, 1000);
    EXPECT_EQ(err, expected_err) << "rbus_pushObj failed";
    return true;
}

class MultipleObjectsTest : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for MultipleObjectsTest\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for MultipleObjectsTest\n");
}

};

TEST_F(MultipleObjectsTest, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(MultipleObjectsTest, rbus_multipleObjects_test1)
{
    int counter = 3, i = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[OBJECT_NAME_SIZE];
    char data_buf[DATA_LENGTH];

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE1(counter, 10);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        for(i = 1;i < 10;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            printf("Registering object %s\n", name_buf);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
        //printf("Stoping server instance from createServer test\n");
    }
    else
    {
        printf("fork failed.\n");
    }
}


TEST_F(MultipleObjectsTest, rbus_multipleObjects_test2)
{
    int counter = 3, i = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[OBJECT_NAME_SIZE];
    char data_buf[DATA_LENGTH];

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE1(counter, 10);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        for(i = 1;i < 10;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }
        for(i = 1;i < 10;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleObjects_test3)
{
    int counter = 3, i = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[OBJECT_NAME_SIZE];
    char data_buf[DATA_LENGTH];

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE1(counter, 13);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        for(i = 1;i < 13;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }
        for(i = 1;i < 13;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleObjects_test4)
{
    int counter = 3, i = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[OBJECT_NAME_SIZE];
    char data_buf[DATA_LENGTH];

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_INSTANCE1(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        for(i = 1;i < 15;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }
        for(i = 1;i < 15;i++)
        {
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleElement_test1)
{
    int counter = 1, i = 1;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        for(i = 1;i < 15;i++)
        {
            memset(name_buf, 0, ELEMENT_NAME_SIZE);
            memset(data_buf, 0, DATA_LENGTH);
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d", i);
            //printf("pushing data : %s to element %s\n", data_buf, name_buf);

            RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }
        for(i = 1;i < 15;i++)
        {
            memset(name_buf, 0, ELEMENT_NAME_SIZE);
            memset(data_buf, 0, DATA_LENGTH);
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element%d", i);
            snprintf(data_buf, (sizeof(data_buf) - 1), "student_14");
            //printf("pulling data from element %s\n", name_buf);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleElement_test2)
{
    int counter = 2, i = 1;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        /*Data pushed with object name*/
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_object_data");

        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);

        for(i = 1;i < 15;i++)
        {
            memset(name_buf, 0, ELEMENT_NAME_SIZE);
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleElement_test3)
{
    int counter = 3, i = 1;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        /*Data pushed with element1 name*/
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element1");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_element1_data");

        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);

        for(i = 1;i < 15;i++)
        {
            memset(name_buf, 0, ELEMENT_NAME_SIZE);
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, rbus_multipleElement_test4)
{
    int counter = 4, i = 1;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        /*Data pushed with element1 name*/
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element1");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_element1_data");

        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);

        memset(name_buf, 0, ELEMENT_NAME_SIZE);
        memset(data_buf, 0, DATA_LENGTH);
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element2");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_element2_data");

        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);

        for(i = 1;i < 15;i++)
        {
            memset(name_buf, 0, ELEMENT_NAME_SIZE);
            snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element%d", i);

            RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_SUCCESS);
        }

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

/*Test cases 5 and 6 will fail in default configuration of rtrouted.
  Because of some internal optimizations in rtrtouted, it doesn't match the *complete* object name in order to decide where to send data.
  What this means is that if you've registered an object "obj", and added new elements "obj.element.1", "obj.element.2" to it, rtrouted
  will send any traffic matching destination wildcard pattern obj.element.* to that object. So you can set/get something entirely fictional,
  like "obj.element.nonexistent", and those bus messages will get delivered to object "obj"'s handlers.

  There is a way to disable this optimization (at runtime) and make rtrouted stricter, (you can run `./rtm_diag_probe rStrategyNormal`)
  Current decision is to leave the optimizations enabled by default. Hence disabling rbus_multipleElement_test5 and rbus_multipleElement_test6*/

TEST_F(MultipleObjectsTest, DISABLED_rbus_multipleElement_test5)
{
    int counter = 5;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 5);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        /*Only 5 elements are created. Trying to push data to an undefined element*/
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element10");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_element10_data");

//        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_ERROR_ENTRY_NOT_FOUND);

        /*Trying to pull data from an undefined element*/
        RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_ERROR_ENTRY_NOT_FOUND);

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

TEST_F(MultipleObjectsTest, DISABLED_rbus_multipleElement_test6)
{
    int counter = 6;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};
    char data_buf[DATA_LENGTH] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 5);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        /*Only 5 elements are created. Trying to push data to an undefined element*/
        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.elementNone");
        snprintf(data_buf, (sizeof(data_buf) - 1), "student_data_none");

        RBUS_PUSH_OBJECT1(data_buf, name_buf, RBUSCORE_ERROR_ENTRY_NOT_FOUND);

        /*Trying to pull data from an undefined element*/
        RBUS_PULL_OBJECT1(data_buf, name_buf, RBUSCORE_ERROR_ENTRY_NOT_FOUND);

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}

/*Trying to pull data without a data push to the object*/
TEST_F(MultipleObjectsTest, rbus_multipleElement_test7)
{
    int counter = 7;
    char client_name[] = "TEST_CLIENT_4";
    bool conn_status = false;
    char name_buf[ELEMENT_NAME_SIZE] = {0};

    pid_t pid = fork();

    if(pid == 0)
    {
        CREATE_RBUS_SERVER_ELEMENTS(counter, 15);
        printf("********** SERVER ENTERING PAUSED STATE******************** \n");
        pause();
    }
    else if (pid > 0)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION1(client_name);

        snprintf(name_buf, (sizeof(name_buf) - 1), "student_info.obj.element1");

        RBUS_PULL_OBJECT1("init init init", name_buf, RBUSCORE_SUCCESS);

        if(conn_status)
            CLOSE_BROKER_CONNECTION1();

        kill(pid,SIGTERM);
    }
    else
    {
        printf("fork failed.\n");
    }
}
