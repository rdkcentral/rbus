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
Test Case : Testing rbus Event Register APIs, Publish Events creation APIs
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

static bool RBUS_OPEN_BROKER_CONNECTION(char* server_name, rbusCoreError_t expected_status)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if((err = rbus_openBrokerConnection(server_name)) == RBUSCORE_SUCCESS)
    {
         printf("Successfully connected to bus.\n");
         result = true;
    }
    EXPECT_EQ(err, expected_status) << "rbus_openBrokerConnection failed";
    return result;
}
static bool RBUS_CLOSE_BROKER_CONNECTION(rbusCoreError_t expected_status)
{
    bool result = false;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    if((err = rbus_closeBrokerConnection()) == RBUSCORE_SUCCESS)
    {
        printf("Successfully disconnected from bus.\n");
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

void CREATE_RBUS_SERVER(int handle)
{

    char server_name[MAX_SERVER_NAME] = "test_server_";
    char buffer[DEFAULT_RESULT_BUFFERSIZE];
    char buff[DEFAULT_RESULT_BUFFERSIZE];
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    bool conn_status = false;
    char count[10] = {};

    memset( buffer, 0, DEFAULT_RESULT_BUFFERSIZE );
    sprintf(count, "%d", handle);
    strcat(server_name, count);

    printf("*********************thamim  CREATING SERVER : %s \n", server_name);

    signal(SIGTERM, handle_signal);

    reset_stored_data();

    conn_status = RBUS_OPEN_BROKER_CONNECTION(server_name, RBUSCORE_SUCCESS);

    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";

    snprintf(buffer, (sizeof(buffer) - 1), "%s.obj1", server_name);

    printf("Registering obj %s \n", buffer);

    err = rbus_registerObj(buffer, callback, NULL);

    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerObj failed";

    memset( buff, 0, DEFAULT_RESULT_BUFFERSIZE );

    snprintf(buff, (sizeof(buff) - 1), "METHOD_%d", 1);

    printf("Registering method %s \n", buff);

    rbus_method_table_entry_t table[2] = {{METHOD_SETPARAMETERVALUES, NULL, handle_set1}, {METHOD_GETPARAMETERVALUES, NULL, handle_get1}};
    err = rbus_registerMethodTable(buffer, table, 2);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerMethodTable failed";

    printf("**********EXITING SERVER : %s ******************** \n", server_name);
    return;
}

class EventServerAPIs : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");
    reset_stored_data();
    printf("Set up done Successfully for EventServerAPIs\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for EventServerAPIs\n");
}

};

TEST_F(EventServerAPIs, sample_test)
{
    EXPECT_EQ(1, 1);
}


TEST_F(EventServerAPIs, rbus_registerEvent_test1)
{
    int counter = 1;
    bool conn_status = false;
    char obj_name[20] = "test_server_1.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char data[] = "data";
    CREATE_RBUS_SERVER(counter);

    printf("Registering Event using %s \n", obj_name);

    //Test with invalid objname passed
        err = rbus_registerEvent(NULL,"event1",sub1_callback,data);
        EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerEvent failed";
    //Test with empty Event name.
       err = rbus_registerEvent(obj_name,NULL,sub1_callback,data);
       EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
     //Test with valid Event Name
      err = rbus_registerEvent(obj_name,"event1",sub1_callback,data);
      EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
    //Test the same Event Name to registered
     err = rbus_registerEvent(obj_name,"event1",sub1_callback,data);
     EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";

    conn_status = RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";
  return;

}

TEST_F(EventServerAPIs, rbus_unregisterEvent_test1)
{
    int counter = 2;
    bool conn_status = false;
    char obj_name[20] = "test_server_2.obj1";
    char obj_name1[20] = "test_server_1.obj2";
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    char data[] = "data";

    CREATE_RBUS_SERVER(counter);

    //Test with  objname to be NULL
    err = rbus_unregisterEvent(obj_name1, "event2");
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_unregisterEvent failed";
    //Test with Event name to be NULL
    err = rbus_unregisterEvent(obj_name, "NULL");
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_unregisterEvent failed";
    //Test with valid Event Name
    err = rbus_registerEvent(obj_name,"event2",sub1_callback,data);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
    err = rbus_unregisterEvent(obj_name, "event2");
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_unregisterEvent failed";
    //Test the same Event Name to unregistered
    err = rbus_unregisterEvent(obj_name, "event2");
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_unregisterEvent failed";
    conn_status = RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";

 return;

}

TEST_F(EventServerAPIs, rbus_publishEvent_test1)
{
    int counter = 3;
    bool conn_status = false;
    char obj_name[20] = "test_server_3.obj1";
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage msg1;
    char data[] = "data";
    CREATE_RBUS_SERVER(counter);

    //Test with valid Event Name
    err = rbus_registerEvent(obj_name,"event3",sub1_callback,data);
    EXPECT_EQ(err, RBUSCORE_SUCCESS) << "rbus_registerEvent failed";
    rbusMessage_Init(&msg1);
    rbusMessage_SetString(msg1, "bar");
    rbus_publishEvent(obj_name, "event1", msg1);
    rbusMessage_Release(msg1);
    conn_status = RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";

 return;

}


TEST_F(EventServerAPIs, rbus_publishEvent_test2)
{
    int counter = 3;
    bool conn_status = false;
    char obj_name[129] = "0";
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage msg1;
    char data[] = "data";

    CREATE_RBUS_SERVER(counter);

    //Boundary Test with MAX_OBJECT_NAME_LENGTH
    memset(obj_name, 't', (sizeof(obj_name)- 1));
    err = rbus_registerEvent(obj_name,"event3",sub1_callback,data);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerEvent failed";
    rbusMessage_Init(&msg1);
    rbusMessage_SetString(msg1, "bar");
    rbus_publishEvent(obj_name, "event1", msg1);
    EXPECT_EQ(err, RBUSCORE_ERROR_INVALID_PARAM) << "rbus_registerEvent failed";
    rbusMessage_Release(msg1);

    conn_status = RBUS_CLOSE_BROKER_CONNECTION(RBUSCORE_SUCCESS);
    ASSERT_EQ(conn_status, true) << "RBUS_OPEN_BROKER_CONNECTION failed";

 return;

}
