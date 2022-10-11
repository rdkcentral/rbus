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
/*****************************************
Test Case : Testing Marshalling APIs
******************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rbuscore_message.h"

extern "C" {

}
#include "gtest_app.h"


#define DEFAULT_RESULT_BUFFERSIZE 128



typedef struct{
    uint32_t element1;
    char element2[100];
}testStruct_t;


static void compareMessage(rbusMessage message, const char* expectedMessage)
{
    char* buff = NULL;
    uint32_t buff_length = 0;

    rbusMessage_ToDebugString(message, &buff, &buff_length);
    //printf("dumpMessage: %.*s\n", buff_length, buff);
    EXPECT_STREQ(buff, expectedMessage) << "Message comparison failed!!";
    free(buff);
}

class TestMarshallingAPIs : public ::testing::Test{

protected:

static void SetUpTestCase()
{
    printf("********************************************************************************************\n");

    printf("Set up done Successfully for TestMarshallingAPIs\n");
}

static void TearDownTestCase()
{
    printf("********************************************************************************************\n");
    printf("Clean up done Successfully for TestMarshallingAPIs\n");
}

};

TEST_F(TestMarshallingAPIs, sample_test)
{
    EXPECT_EQ(1, 1);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test1)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rbusMessage_SetString(testMessage, value);
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test2)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "########!!!!!!TestString123456789000000000000000000000000000";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rbusMessage_SetString(testMessage, value);
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test3)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "########!!!!!!TestString123456789000000000000000000000000000";
    char value2[] = "TestString123456789";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rbusMessage_SetString(testMessage, value1);
    rbusMessage_SetString(testMessage, value2);
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value1) << "rbusMessage_SetString failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value2) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test4)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rbusMessage_SetString(testMessage, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rbusMessage_GetString(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    }

    rbusMessage_Release(testMessage);
}

/*Set and  Get 10000 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test5)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 10000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rbusMessage_SetString(testMessage, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 10000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rbusMessage_GetString(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    }

    rbusMessage_Release(testMessage);
}

/*Set and  Get 100000 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test6)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rbusMessage_SetString(testMessage, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 100000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rbusMessage_GetString(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    }

    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test7)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "string value 1";
    const char* messageString = "\"string value 1\"";
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rbusMessage_SetString(testMessage, value);
    compareMessage(testMessage, messageString);
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test8)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test9)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "########!!!!!!TestString123456789000000000000000000000000000";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test10)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "########!!!!!!TestString123456789000000000000000000000000000";
    char value2[] = "TestString123456789";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value1);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_SetString(testMessage, value2);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value1) << "rbusMessage_SetString failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value2) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test11)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rbusMessage_SetString(testMessage, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    memset(value, 0, 50);
    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rbusMessage_GetString(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed for iteration: " << i ;
    }

    rbusMessage_Release(testMessage);
}

/*Try to set empty string*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test12)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

/*Try to set NULL value for string*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test13)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "";
    const char* resultValue = NULL;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, NULL);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetString(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rbusMessage_SetString failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test14)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value[] = "string value 1";
    const char* messageString = "\"string value 1\"";
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    compareMessage(testMessage, messageString);
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test15)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "string value 1";
    char value2[] = "string value 2";
    const char* messageString = "\"string value 1\" \"string value 2\"";
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value1);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_SetString(testMessage, value2);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    compareMessage(testMessage, messageString);
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetString_test16)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "string value 1";
    char value2[] = "string value 2";
    char value3[] = "string value 3";
    const char* messageString="\"string value 1\" \"string value 2\" \"string value 3\"";
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetString(testMessage, value1);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_SetString(testMessage, value2);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_SetString(testMessage, value3);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    compareMessage(testMessage, messageString);
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test1)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2000;
    int32_t resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetInt32(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetInt32(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test2)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2147483647;
    int32_t resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetInt32(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetInt32(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed";
    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 integers to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test3)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t valueOriginal = 2000;
    int32_t value = 0;
    int32_t resultValue = 0;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_SetInt32(testMessage, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_GetInt32(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed";
    }
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test4)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2000;
    int32_t resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetInt32(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetInt32(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test5)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2147483647;
    int32_t resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetInt32(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetInt32(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed";
    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 integers to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetInt32_test6)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    int32_t valueOriginal = 2000;
    int32_t value = 0;
    int32_t resultValue = 0;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_SetInt32(testMessage, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_GetInt32(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetInt32 failed for iteration: " << i;
    }
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test1)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double value = 999.999;
    double resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetDouble(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetDouble(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test2)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double value = 21474836.67;
    double resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetDouble(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetDouble(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";

    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 double values to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test3)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double valueOriginal = 2000.0002;
    double value = 0;
    double resultValue = 0;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_SetDouble(testMessage, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_GetDouble(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    }
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test4)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double value = 999.999;
    double resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetDouble(testMessage,  value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetDouble(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test5)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double value = 21474836.67;
    double resultValue = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetDouble(testMessage, value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetDouble(testMessage, &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";

    rbusMessage_Release(testMessage);
}

/*Set and  Get 100 double values to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test6)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    double valueOriginal = 2000.0002;
    double value = 0;
    double resultValue = 0;
    int i = 0;
    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_SetDouble(testMessage, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        err = rbusMessage_GetDouble(testMessage, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed for iteration: " << i ;
    }
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetBytes_test1)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    testStruct_t sampleStruct = {10, "String1"};
    const testStruct_t *ptr;
    unsigned int size = 0;

    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetBytes(testMessage, (uint8_t*)&sampleStruct, sizeof(sampleStruct));
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetBytes(testMessage, (const uint8_t**)&ptr, &size);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(size, sizeof(sampleStruct)) << "BinaryData size comparison failed";
    EXPECT_EQ(ptr->element1, sampleStruct.element1) << "BinaryData element1 comparison failed";
    EXPECT_STREQ(ptr->element2, sampleStruct.element2) << "BinaryData element1 comparison failed";
    rbusMessage_Release(testMessage);
}
TEST_F(TestMarshallingAPIs, rbusMessage_SetBytes_test2)
{
    rbusMessage testMessage;
    rtError err = RT_OK;
    testStruct_t sampleStruct = {10, "String1"};
    const testStruct_t *ptr;
    unsigned int size = 0;

    rbusMessage_Init(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rbusMessage_SetBytes(testMessage, (uint8_t*)&sampleStruct, sizeof(sampleStruct));
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rbusMessage_GetBytes(testMessage, (const uint8_t**)&ptr, &size);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(size, sizeof(sampleStruct)) << "BinaryData size comparison failed";
    EXPECT_EQ(ptr->element1, sampleStruct.element1) << "BinaryData element1 comparison failed";
    EXPECT_STREQ(ptr->element2, sampleStruct.element2) << "BinaryData element1 comparison failed";
    rbusMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test1)
{
    rbusMessage childMessage;
    rbusMessage parentMessage;
    rbusMessage procuredMessage;
    char value[] = "TestString1";
    const char* messageString = "\"TestString1\"";

    rbusMessage_Init(&childMessage);
    rbusMessage_SetString(childMessage, value);

    rbusMessage_Init(&parentMessage);
    rbusMessage_SetMessage(parentMessage, childMessage);
    rbusMessage_GetMessage(parentMessage, &procuredMessage);
    compareMessage(procuredMessage, messageString);

    rbusMessage_Release(childMessage);
    rbusMessage_Release(parentMessage);
    rbusMessage_Release(procuredMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test2)
{
    rbusMessage childMessage;
    rbusMessage parentMessage;
    rbusMessage procuredMessage;
    char value[] = "TestString1";
    const char* messageString = "\"TestString1\"";

    rbusMessage_Init(&childMessage);
    rbusMessage_SetString(childMessage, value);

    rbusMessage_Init(&parentMessage);
    rbusMessage_SetMessage(parentMessage, childMessage);

    rbusMessage_GetMessage(parentMessage, &procuredMessage);
    compareMessage(procuredMessage, messageString);

    rbusMessage_Release(procuredMessage);
    rbusMessage_Release(childMessage);
    rbusMessage_Release(parentMessage);
}

TEST_F(TestMarshallingAPIs, DISABLED_rbusMessage_SetMessage_test3)
{
    rbusMessage childMessage1;
    rbusMessage childMessage2;
    rbusMessage parentMessage;
    char value1[] = "TestString1";
    char value2[] = "TestString2";
    const char* messageString = "\"TestString1\" \"TestString2\"";

    rbusMessage_Init(&childMessage1);
    rbusMessage_SetString(childMessage1, value1);
    rbusMessage_Init(&childMessage2);
    rbusMessage_SetString(childMessage2, value2);

    rbusMessage_Init(&parentMessage);
    rbusMessage_SetMessage(parentMessage, childMessage1);
    rbusMessage_SetMessage(parentMessage, childMessage2);

    compareMessage(parentMessage, messageString);

    rbusMessage_Release(childMessage1);
    rbusMessage_Release(childMessage2);
    rbusMessage_Release(parentMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test4)
{
    rbusMessage childMessage1;
    rbusMessage childMessage2;
    rbusMessage parentMessage;
    rbusMessage procuredMessage;
    char value1[] = "TestString1";
    char value2[] = "TestString2";
    const char* messageString1 = "\"TestString1\"";
    const char* messageString2 = "\"TestString2\"";

    rbusMessage_Init(&childMessage1);
    rbusMessage_SetString(childMessage1, value1);
    rbusMessage_Init(&childMessage2);
    rbusMessage_SetString(childMessage2, value2);

    rbusMessage_Init(&parentMessage);
    rbusMessage_SetMessage(parentMessage, childMessage1);
    rbusMessage_SetMessage(parentMessage, childMessage2);

    rbusMessage_GetMessage(parentMessage, &procuredMessage);
    compareMessage(procuredMessage, messageString1);
    rbusMessage_Release(procuredMessage);
    rbusMessage_GetMessage(parentMessage, &procuredMessage);
    compareMessage(procuredMessage, messageString2);
    rbusMessage_Release(procuredMessage);

    rbusMessage_Release(childMessage1);
    rbusMessage_Release(childMessage2);
    rbusMessage_Release(parentMessage);
}
