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
#include "rtMessage.h"

extern "C" {

}
#include "gtest_app.h"


#define DEFAULT_RESULT_BUFFERSIZE 128



typedef struct{
    uint32_t element1;
    char element2[100];
}testStruct_t;


static void compareMessage(rtMessage message, const char* expectedMessage)
{
    const char* buff = NULL;
    rtMessage_GetString(message,"value",&buff);
    EXPECT_STREQ(buff, expectedMessage) << "Message comparison failed!!";
    //free((void*)buff);
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

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test1)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rtMessage call failed";

    rtMessage_SetString(testMessage, "val",value);
    err = rtMessage_GetString(testMessage, "val",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test2)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "########!!!!!!TestString123456789000000000000000000000000000";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "val",value);
    err = rtMessage_GetString(testMessage, "val",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test3)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "########!!!!!!TestString123456789000000000000000000000000000";
    char value2[] = "TestString123456789";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value1",value1);
    rtMessage_SetString(testMessage, "value2",value2);
    err = rtMessage_GetString(testMessage, "value1",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value1) << "rtMessage_SetString failed";
    err = rtMessage_GetString(testMessage, "value2",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value2) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

/*Set and  Get 100 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test4)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rtMessage_SetString(testMessage,value, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rtMessage_GetString(testMessage, value,&resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    }
    memset(value, 0, 50);
    rtMessage_Release(testMessage);
}

/*Set and  Get 10000 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test5)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 10000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rtMessage_SetString(testMessage, value, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 10000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rtMessage_GetString(testMessage, value, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    }

    rtMessage_Release(testMessage);
}

/*Set and  Get 100000 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test6)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rtMessage_SetString(testMessage, value, value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 100000; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rtMessage_GetString(testMessage, value, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    }

    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test7)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "string value 1";
    const char* messageString = "string value 1";
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value",value);
    compareMessage(testMessage, messageString);
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test8)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value",value);
    err = rtMessage_GetString(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test9)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "########!!!!!!TestString123456789000000000000000000000000000";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value",value);
    err = rtMessage_GetString(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test10)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value1[] = "########!!!!!!TestString123456789000000000000000000000000000";
    char value2[] = "TestString123456789";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value1",value1);
    rtMessage_SetString(testMessage, "value2",value2);
    err = rtMessage_GetString(testMessage, "value1",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value1) << "rtMessage_SetString failed";
    err = rtMessage_GetString(testMessage, "value2",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value2) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

/*Set and  Get 100 strings to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test11)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char valueOriginal[50] = "TestString";
    char value[50] = "";
    const char* resultValue = NULL;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        rtMessage_SetString(testMessage, value,value);
    }
    memset(value, 0, 50);
    for(i = 0; i < 100; i++)
    {
        snprintf(value, (sizeof(value) - 1), "%s%d", valueOriginal, i);
        err = rtMessage_GetString(testMessage, value, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed for iteration: " << i ;
    }

    rtMessage_Release(testMessage);
}

/*Try to set empty string*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test12)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value",value);
    err = rtMessage_GetString(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

/*Try to set NULL value for string*/
TEST_F(TestMarshallingAPIs, rtMessage_SetString_test13)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "";
    const char* resultValue = NULL;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value", "");
    err = rtMessage_GetString(testMessage, "value", &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_STREQ(resultValue, value) << "rtMessage_SetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test14)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char value[] = "string value 1";
    const char* messageString = "string value 1";
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value", value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    compareMessage(testMessage, messageString);
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test15)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    const char *value1 = "string value 1";
    const char *value2 = "string value 2";
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value1", value1);
    rtMessage_SetString(testMessage, "value2", value2);
    err = rtMessage_GetString(testMessage, "value1", &value1);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetString failed";
    err = rtMessage_GetString(testMessage,"value2", &value2);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetString_test16)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    const char *value1 = "string value 1";
    const char *value2 = "string value 2";
    const char *value3 = "string value 3";
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    rtMessage_SetString(testMessage, "value1",value1);
    rtMessage_SetString(testMessage, "value2",value2);
    rtMessage_SetString(testMessage, "value3",value3);
    err = rtMessage_GetString(testMessage, "value1", &value1);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetString failed";
    err = rtMessage_GetString(testMessage,"value2", &value2);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetString failed";
    err = rtMessage_GetString(testMessage,"value3", &value3);
    EXPECT_EQ(err, RT_OK) << "rtMessage_GetString failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test1)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2000;
    int32_t resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetInt32(testMessage, "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetInt32(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test2)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2147483647;
    int32_t resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetInt32(testMessage, "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetInt32(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed";
    rtMessage_Release(testMessage);
}

/*Set and  Get 100 integers to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test3)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char string[50] = "TestString";
    char str[50] = "";
    int32_t valueOriginal = 2000;
    int32_t value = 0;
    int32_t resultValue = 0;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_SetInt32(testMessage, str, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
	snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_GetInt32(testMessage, str, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed";
    }
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test4)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2000;
    int32_t resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetInt32(testMessage, "value", value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetInt32(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test5)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    int32_t value = 2147483647;
    int32_t resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetInt32(testMessage, "value", value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetInt32(testMessage, "value", &resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed";
    rtMessage_Release(testMessage);
}

/*Set and  Get 100 integers to an rtMessage*/
TEST_F(TestMarshallingAPIs, rtMessage_SetInt32_test6)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char string[50] = "TestString";
    char str[50] = "";
    int32_t valueOriginal = 2000;
    int32_t value = 0;
    int32_t resultValue = 0;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_SetInt32(testMessage, str, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
        snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_GetInt32(testMessage, str, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rtMessage_SetInt32 failed for iteration: " << i;
    }
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test1)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    double value = 999.999;
    double resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetDouble(testMessage, "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetDouble(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test2)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    double value = 21474836.67;
    double resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetDouble(testMessage, "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetDouble(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";

    rtMessage_Release(testMessage);
}

/*Set and  Get 100 double values to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test3)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char string[50] = "TestString";
    char str[50] = "";
    double valueOriginal = 2000.0002;
    double value = 0;
    double resultValue = 0;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
	snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_SetDouble(testMessage, str,value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
	snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_GetDouble(testMessage, str,&resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    }
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test4)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    double value = 999.999;
    double resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetDouble(testMessage,  "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetDouble(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test5)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    double value = 21474836.67;
    double resultValue = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_SetDouble(testMessage, "value",value);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetDouble(testMessage, "value",&resultValue);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed";

    rtMessage_Release(testMessage);
}

/*Set and  Get 100 double values to an rtMessage*/
TEST_F(TestMarshallingAPIs, rbusMessage_SetDouble_test6)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    char string[50] = "TestString";
    char str[50] = "";
    double valueOriginal = 2000.0002;
    double value = 0;
    double resultValue = 0;
    int i = 0;
    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
	snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_SetDouble(testMessage, str, value);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
    }
    for(i = 0; i < 100; i++)
    {
        value = valueOriginal;
        value = value * i;
	snprintf(str, (sizeof(str) - 1), "%s%d", string, i);
        err = rtMessage_GetDouble(testMessage, str, &resultValue);
        EXPECT_EQ(err, RT_OK) << "rbus call failed";
        EXPECT_EQ(resultValue, value) << "rbusMessage_SetDouble failed for iteration: " << i ;
    }
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rtMessage_AddBinaryData_test1)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    testStruct_t sampleStruct = {10, "String1"};
    const testStruct_t *ptr;
    unsigned int size = 0;

    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_AddBinaryData(testMessage, "value",(uint8_t*)&sampleStruct, sizeof(sampleStruct));
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetBinaryData(testMessage, "value",(void**)&ptr, &size);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(size, sizeof(sampleStruct)) << "BinaryData size comparison failed";
    EXPECT_EQ(ptr->element1, sampleStruct.element1) << "BinaryData element1 comparison failed";
    EXPECT_STREQ(ptr->element2, sampleStruct.element2) << "BinaryData element1 comparison failed";
    rtMessage_Release(testMessage);
}
TEST_F(TestMarshallingAPIs, rtMessage_AddBinaryData_test2)
{
    rtMessage testMessage;
    rtError err = RT_OK;
    testStruct_t sampleStruct = {10, "String1"};
    const testStruct_t *ptr;
    unsigned int size = 0;

    rtMessage_Create(&testMessage);
    EXPECT_EQ(err, RT_OK) << "rbusMessage call failed";

    err = rtMessage_AddBinaryData(testMessage, "value",(uint8_t*)&sampleStruct, sizeof(sampleStruct));
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    err = rtMessage_GetBinaryData(testMessage, "value",(void**)&ptr, &size);
    EXPECT_EQ(err, RT_OK) << "rbus call failed";
    EXPECT_EQ(size, sizeof(sampleStruct)) << "BinaryData size comparison failed";
    EXPECT_EQ(ptr->element1, sampleStruct.element1) << "BinaryData element1 comparison failed";
    EXPECT_STREQ(ptr->element2, sampleStruct.element2) << "BinaryData element1 comparison failed";
    rtMessage_Release(testMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test1)
{
    rtMessage childMessage;
    rtMessage parentMessage;
    rtMessage procuredMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";

    rtMessage_Create(&childMessage);
    rtMessage_SetString(childMessage, "value",value);

    rtMessage_Create(&parentMessage);
    err = rtMessage_SetMessage(parentMessage, "message",childMessage);
    EXPECT_EQ(err,RT_OK);
    err = rtMessage_GetMessage(parentMessage, "message",&procuredMessage);
    EXPECT_EQ(err,RT_OK);

    rtMessage_Release(childMessage);
    rtMessage_Release(parentMessage);
    rtMessage_Release(procuredMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test2)
{
    rtMessage childMessage;
    rtMessage parentMessage;
    rtMessage procuredMessage;
    rtError err = RT_OK;
    char value[] = "TestString1";

    rtMessage_Create(&childMessage);
    rtMessage_SetString(childMessage, "value",value);

    rtMessage_Create(&parentMessage);
    err = rtMessage_SetMessage(parentMessage, "message",childMessage);
    EXPECT_EQ(err,RT_OK);
    err = rtMessage_GetMessage(parentMessage, "message",&procuredMessage);
    EXPECT_EQ(err,RT_OK);

    rtMessage_Release(procuredMessage);
    rtMessage_Release(childMessage);
    rtMessage_Release(parentMessage);
}

TEST_F(TestMarshallingAPIs, DISABLED_rbusMessage_SetMessage_test3)
{
    rtMessage childMessage1;
    rtMessage childMessage2;
    rtMessage parentMessage;
    rtError err = RT_OK;
    char value1[] = "TestString1";
    char value2[] = "TestString2";

    rtMessage_Create(&childMessage1);
    rtMessage_SetString(childMessage1, "value1",value1);
    rtMessage_Create(&childMessage2);
    rtMessage_SetString(childMessage2, "value2",value2);

    rtMessage_Create(&parentMessage);
    err = rtMessage_SetMessage(parentMessage, "message1",childMessage1);
    EXPECT_EQ(err, RT_OK);
    err = rtMessage_SetMessage(parentMessage, "message2",childMessage2);
    EXPECT_EQ(err,RT_OK);

    rtMessage_Release(childMessage1);
    rtMessage_Release(childMessage2);
    rtMessage_Release(parentMessage);
}

TEST_F(TestMarshallingAPIs, rbusMessage_SetMessage_test4)
{
    rtError err = RT_OK;
    rtMessage childMessage1;
    rtMessage childMessage2;
    rtMessage parentMessage;
    rtMessage procuredMessage;
    char value1[] = "TestString1";
    char value2[] = "TestString2";

    rtMessage_Create(&childMessage1);
    rtMessage_SetString(childMessage1, "value1",value1);
    rtMessage_Create(&childMessage2);
    rtMessage_SetString(childMessage2, "value2",value2);

    rtMessage_Create(&parentMessage);
    rtMessage_SetMessage(parentMessage, "message1",childMessage1);
    rtMessage_SetMessage(parentMessage, "message2",childMessage2);

    err = rtMessage_GetMessage(parentMessage, "message1",&procuredMessage);
    EXPECT_EQ(err, RT_OK);
    rtMessage_Release(procuredMessage);
    err = rtMessage_GetMessage(parentMessage, "message2",&procuredMessage);
    EXPECT_EQ(err, RT_OK);
    rtMessage_Release(procuredMessage);

    rtMessage_Release(childMessage1);
    rtMessage_Release(childMessage2);
    rtMessage_Release(parentMessage);
}
