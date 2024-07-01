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
#include <vector>
extern "C" {
#include "rbuscore.h"

}
#include "gtest_app.h"
#include "rbus_test_util.h"

#define DEFAULT_RESULT_BUFFERSIZE 128
#define MAX_SERVER_NAME 40


static bool OPEN_BROKER_CONNECTION2(char* connection_name)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbuscore_openBrokerConnection(connection_name)) == RBUSCORE_SUCCESS)
    {
         //printf("Successfully connected to bus.\n");
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
static void CREATE_RBUS_SERVER_INSTANCE2(int handle, int obj_count)
{
    char server_name[MAX_SERVER_NAME] = "test_server_";
    char obj_name[20] = "student_info";
    int i = 1;
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;
    static char test_buffer[100][100] = {};

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    memset( test_buffer, 0, sizeof(test_buffer));
    snprintf((server_name + strlen(server_name)), (MAX_SERVER_NAME - strlen(server_name)), "%d", handle);

    printf("*** CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_term2);
    reset_stored_data();

    conn_status = OPEN_BROKER_CONNECTION2(server_name);

    ASSERT_EQ(conn_status, true) << "OPEN_BROKER_CONNECTION2 failed";

    for(i = 1; i < obj_count; i++)
    {
        snprintf(buffer, (sizeof(buffer) - 1), "%s_%s.obj%d", server_name, obj_name, i);
        printf("Registering object %s\n", buffer);
        strncpy(*(test_buffer + i), buffer, 100);

        err = rbuscore_registerObj(buffer, callback, NULL);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_registerObj failed";

        rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, (void *)(test_buffer + i), handle_setStudentInfo}, {METHOD_GETPARAMETERVALUES, (void *)(test_buffer + i), handle_getStudentInfo}};

        err = rbuscore_registerMethodTable(buffer, table, 2);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_registerMethodTable failed";
    }
    return;
}

static std::string integer2string(int num)
{
    char array_num[32] = {0};
    snprintf(array_num, sizeof(array_num), "%d",num);
    return std::string(array_num);
}

static void CREATE_RBUS_SERVER_INSTANCE3(const char * app_prefix, int num)
{
    std::string server_name("");
    std::string num_string = integer2string(num);
    server_name += app_prefix + num_string;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    printf("*** CREATING SERVER : %s \n", server_name.c_str());

    signal(SIGTERM, handle_term2);
    reset_stored_data();

    err = rbuscore_openBrokerConnection(server_name.c_str());
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_openBrokerConnection2() failed";
    
    std::string obj_name = std::string(app_prefix) + num_string + ".obj";
    err = rbuscore_registerObj(obj_name.c_str(), callback, NULL);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_registerObj failed";
    err = rbuscore_addElement(obj_name.c_str(), std::string(app_prefix + num_string + std::string(".element")).c_str());
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_addElement failed";
    rbuscore_addElement(obj_name.c_str(), std::string("common.element." + num_string).c_str());
    return;
}


static void RBUS_PULL_OBJECT2(char* expected_data, char* server_obj, rbusCoreError_t expected_err)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response;
    if((err = rbuscore_pullObj(server_obj, 1000, &response)) == RBUSCORE_SUCCESS)
    {
        const char* buff = NULL;
        rbusMessage_GetString(response, &buff);
        printf("%s: rbus pull returned : %s \n", __FUNCTION__, buff);
        EXPECT_STREQ(buff, expected_data) << "rbuscore_pullObj failed to procure expected string";
        rbusMessage_Release(response);
    }
    else
    {
        printf("Could not pull object %s\n", server_obj);
    }
    EXPECT_EQ(err, expected_err) << "rbuscore_pullObj failed";
    return;
}

static void RBUS_PUSH_OBJECT2(char* data, char* server_obj, rbusCoreError_t expected_err)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setter;
    rbusMessage_Init(&setter);
    rbusMessage_SetString(setter, data);
    err = rbuscore_pushObj(server_obj, setter, 1000);
    EXPECT_EQ(err, expected_err) << "rbuscore_pushObj failed";
    return;
}

class MultipleServerTest : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for MultipleServerTest\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for MultipleServerTest\n");
}

};

TEST_F(MultipleServerTest, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(MultipleServerTest, rbus_multipleServer_test1)
{
    int counter = 1, i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[100];
    char data_buf[100];
    int server_count = 3;
    int object_count = 3;
    int reg_object_count = 0;
    pid_t pid[10];
    bool is_parent = true;
    const char *pFound = NULL;

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            counter = j + 1;
            CREATE_RBUS_SERVER_INSTANCE2(counter, (object_count + 1));
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(2);
        int num_comps = 0;
        char** components = NULL;
        rbusCoreError_t err;
        //Neg test discover Registered components without connection
        err = rbuscore_discoverRegisteredComponents(&num_comps, &components);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_STATE) << "rbuscore_discoverRegisteredComponents failed";
        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        err = rbuscore_discoverRegisteredComponents(&num_comps, &components);
        EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbuscore_discoverRegisteredComponents failed";
        std::vector <std::string> object_list;
        if(RBUSCORE_SUCCESS == err)
        {
            for(int i = 0; i < num_comps; i++)
            {
                pFound=strstr(components[i],"test_server_");
                if(pFound) {
                    if(strstr(pFound,"_student_info.obj"))
                        reg_object_count++;
                }
                object_list.emplace_back(std::string(components[i]));
                free(components[i]);
            }
            free(components);
            EXPECT_EQ(reg_object_count, (server_count * object_count)) << "rbuscore_discoverRegisteredComponents returned wrong size";
        }
        
        for(j = 1; j <= server_count; j++)
        {
            counter = j;
            for(i = 1;i <= object_count; i++)
            {
                snprintf(name_buf, (sizeof(name_buf) - 1), "test_server_%d_student_info.obj%d", counter, i);
                printf("Registering object %s\n", name_buf);
                snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d_of_server-%d", i, counter);

                RBUS_PUSH_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
                RBUS_PULL_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);

                int listed_in_discovery = 0;
                for(unsigned int k = 0; k < object_list.size(); k++)
                {
                    if(object_list[k] == name_buf)
                    {
                        listed_in_discovery = 1;
                        break;
                    }
                }
                EXPECT_EQ(listed_in_discovery, 1) << "Could not locate object in rbus_registeredComponents() result.";
            }
        }
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();

        for(i = 0; i < server_count; i++)
            kill(pid[i],SIGTERM);
    }
}

TEST_F(MultipleServerTest, rbus_multipleServer_test2)
{
    int counter = 1, i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[100];
    char data_buf[100];
    int server_count = 3;
    int object_count = 10;
    pid_t pid[10];
    bool is_parent = true;

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            counter = j + 1;
            CREATE_RBUS_SERVER_INSTANCE2(counter, (object_count + 1));
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(2);
        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        for(j = 1; j <= server_count; j++)
        {
            counter = j;
            for(i = 1;i <= object_count; i++)
            {
                snprintf(name_buf, (sizeof(name_buf) - 1), "test_server_%d_student_info.obj%d", counter, i);
                printf("Registering object %s\n", name_buf);
                snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d_of_server-%d", i, counter);

                RBUS_PUSH_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
                RBUS_PULL_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
            }
        }
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();

        for(i = 0; i < server_count; i++)
            kill(pid[i],SIGTERM);
    }
}

TEST_F(MultipleServerTest, rbus_multipleServer_test3)
{
    int counter = 1, i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[100];
    char data_buf[100];
    int server_count = 10;
    int object_count = 3;
    pid_t pid[10];
    bool is_parent = true;

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            counter = j + 1;
            CREATE_RBUS_SERVER_INSTANCE2(counter, (object_count + 1));
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(4);

        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        for(j = 1; j <= server_count; j++)
        {
            counter = j;
            for(i = 1;i <= object_count; i++)
            {
                snprintf(name_buf, (sizeof(name_buf) - 1), "test_server_%d_student_info.obj%d", counter, i);
                printf("Registering object %s\n", name_buf);
                snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d_of_server-%d", i, counter);

                RBUS_PUSH_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
                RBUS_PULL_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
            }
        }
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();

        for(i = 0; i < server_count; i++)
            kill(pid[i],SIGTERM);
    }
}

TEST_F(MultipleServerTest, rbus_multipleServer_test4)
{
    int counter = 1, i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[100];
    char data_buf[100];
    int server_count = 8;
    int object_count = 3;
    pid_t pid[server_count];
    bool is_parent = true;

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            counter = j + 1;
            CREATE_RBUS_SERVER_INSTANCE2(counter, (object_count + 1));
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(5);

        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        for(j = 1; j <= server_count; j++)
        {
            counter = j;
            for(i = 1;i <= object_count; i++)
            {
                snprintf(name_buf, (sizeof(name_buf) - 1), "test_server_%d_student_info.obj%d", counter, i);
                printf("Registering object %s\n", name_buf);
                snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d_of_server-%d", i, counter);

                RBUS_PUSH_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
                RBUS_PULL_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
            }
        }
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();

        for(i = 0; i < server_count; i++)
        {
            kill(pid[i],SIGTERM);
        }
    }
}
TEST_F(MultipleServerTest, rbus_multipleServer_test5)
{
    int counter = 1, i = 1, j = 1;
    char client_name[] = "TEST_CLIENT_1";
    bool conn_status = false;
    char name_buf[100];
    char data_buf[100];
    int server_count = 14;
    int object_count = 3;
    pid_t pid[server_count];
    bool is_parent = true;

    for(j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if(pid[j] == 0)
        {
            counter = j + 1;
            CREATE_RBUS_SERVER_INSTANCE2(counter, (object_count + 1));
            is_parent = false;
            printf("********** SERVER ENTERING PAUSED STATE******************** \n");
            pause();
        }
    }
    if(is_parent)
    {
        sleep(10);

        conn_status = OPEN_BROKER_CONNECTION2(client_name);
        for(j = 1; j <= server_count; j++)
        {
            counter = j;
            for(i = 1;i <= object_count; i++)
            {
                snprintf(name_buf, (sizeof(name_buf) - 1), "test_server_%d_student_info.obj%d", counter, i);
                printf("Registering object %s\n", name_buf);
                snprintf(data_buf, (sizeof(data_buf) - 1), "student_%d_of_server-%d", i, counter);

                RBUS_PUSH_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
                RBUS_PULL_OBJECT2(data_buf, name_buf, RBUSCORE_SUCCESS);
            }
        }
        if(conn_status)
            CLOSE_BROKER_CONNECTION2();

        for(i = 0; i < server_count; i++)
        {
            kill(pid[i],SIGTERM);
        }
    }
}
/* Has to be enhanced to get array of output count */
#if 0
TEST_F(MultipleServerTest, rbus_multipleServer_test6)
{
    int j = 0;
    int server_count = 2;
    pid_t pid[server_count];
    bool is_parent = true;

    for (j = 0; j < server_count; j++)
    {
        pid[j] = fork();

        if (pid[j] == 0)
        {
            is_parent = false;
            CREATE_RBUS_SERVER_INSTANCE3("lookup_test", j);
            pause();
        }
    }
    if (is_parent)
    {
        rbusCoreError_t err;
	rbuscore_openBrokerConnection("lookup_client");
        const char *inputs[] = {"lookup_test0.obj", "lookup_test0.element", "lookup_test1.obj", "lookup_test1.element1", "lookup_test0.", "lookup_test1.", "abcd", "common.", "common.element.0", "common.element.1"};
        constexpr int in_length = sizeof(inputs) / sizeof(char *);
        const char *expected_output[] = {"lookup_test0.obj", "lookup_test0.obj", "lookup_test1.obj", "lookup_test1.obj", "lookup_test0.obj", "lookup_test1.obj", "", "", "lookup_test0.obj", "lookup_test1.obj"};
        char **output = nullptr;
        sleep(3);//Allow servers to set up.
        err = rbuscore_discoverElementObjects(inputs, in_length, &output);
        EXPECT_EQ(RBUSCORE_SUCCESS, err) << "rbuscore_discoverElementObjects failed.";
        if(RBUSCORE_SUCCESS == err)
        {
            printf("Multi-lookup returned success. Printing mapping information...\n");
            for (int i = 0; i < in_length; i++)
            {
                //printf("%s mapped to %s\n", inputs[i], output[i]);
                EXPECT_EQ(0, strcmp(expected_output[i], output[i])) << "rbuscore_discoverElementObjects returned wrong data";
                free(output[i]);
            }
            free(output);
        }
        rbuscore_closeBrokerConnection();

        for(int i = 0; i < server_count; i++)
            kill(pid[i], SIGTERM);
    }
}
#endif
