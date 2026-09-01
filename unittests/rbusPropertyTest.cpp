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
#include "gtest/gtest.h"

#include <rbus.h>

TEST(rbusPropertyTest, testName)
{
  rbusProperty_t prop;
  rbusProperty_Init(&prop, NULL, NULL);

  EXPECT_EQ(1, 1);

  rbusProperty_Release(prop);
}

TEST(rbusPropertyTest, testInit)
{
  rbusProperty_t prop1;
  char const* name;

  rbusProperty_Init (&prop1, "Device.rbusPropertyTest", NULL);
  name = rbusProperty_GetName(prop1);
  EXPECT_STREQ("Device.rbusPropertyTest", name) << "rbusProperty_Init failed testInit";

  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testCompare)
{
  rbusProperty_t prop1;
  rbusProperty_t prop2;
  int ret = 0;

  rbusProperty_Init (&prop1, "Device.rbusPropertyTest", NULL);
  rbusProperty_Init (&prop2, "Device.rbusPropertyTest", NULL);

  ret = rbusProperty_Compare(prop1,prop2);
  EXPECT_EQ(0,ret) << "rbusProperty_Compare failed testCompare";

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);
}

TEST(rbusPropertyTest, testSetName)
{
  rbusProperty_t prop1;
  char const* name;

  rbusProperty_Init (&prop1, NULL, NULL);
  rbusProperty_SetName(prop1, "Device.rbusPropertyTest");
  name = rbusProperty_GetName(prop1);
  EXPECT_STREQ("Device.rbusPropertyTest", name) << "rbusProperty_SetName failed testSetName";

  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetValue)
{
  rbusValue_t value;
  rbusProperty_t prop1;
  char const* name;
  int len = 0;

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test");
  rbusProperty_Init (&prop1, "Device.rbusPropertyTest", value);
  rbusValue_Release(value);

  value = rbusProperty_GetValue(prop1);
  name = rbusValue_GetString(value, &len);

  EXPECT_STREQ("test", name) << "rbusProperty_SetName failed testGetValue";
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetNext)
{
  rbusValue_t value=NULL;
  rbusProperty_t prop1=NULL;
  rbusProperty_t prop2;
  rbusProperty_t prop3;
  char const* name;
  char const* rbus_val;
  int len = 0;

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test1");
  rbusProperty_Init (&prop1, "Device.rbusPropertyTest1", value);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test2");
  rbusProperty_Init (&prop2, "Device.rbusPropertyTest2", value);
  rbusProperty_SetNext(prop1, prop2);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test3");
  rbusProperty_Init (&prop3, "Device.rbusPropertyTest3", value);
  rbusProperty_SetNext(prop2, prop3);
  rbusValue_Release(value);

  rbusProperty_t next= prop1;
  while(next) {
      value = rbusProperty_GetValue(next);
      rbus_val = rbusProperty_GetName(next);
      name = rbusValue_GetString(value, &len);
      if(strcmp(rbus_val,"Device.rbusPropertyTest1") == 0) {
	  EXPECT_STREQ("test1", name) << "rbusProperty_GetNext failed testGetNext";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest2") == 0) {
	  EXPECT_STREQ("test2", name) << "rbusProperty_GetNext failed testGetNext";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest3") == 0) {
	  EXPECT_STREQ("test3", name) << "rbusProperty_GetNext failed testGetNext";
      }
      next = rbusProperty_GetNext(next);
  }

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);
  rbusProperty_Release(prop3);
}

TEST(rbusPropertyTest, testPushBack)
{
  rbusValue_t value;
  rbusProperty_t prop1;
  rbusProperty_t prop2;
  rbusProperty_t prop3;
  char const* name;
  char const* rbus_val;
  int len = 0;

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test1");
  rbusProperty_Init (&prop1, "Device.rbusPropertyTest1", value);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test2");
  rbusProperty_Init (&prop2, "Device.rbusPropertyTest2", value);
  rbusProperty_Append(prop1, prop2);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test3");
  rbusProperty_Init (&prop3, "Device.rbusPropertyTest3", value);
  rbusProperty_Append(prop2, prop3);
  rbusValue_Release(value);


  rbusProperty_t next= prop1;
  while(next) {
      value = rbusProperty_GetValue(next);
      rbus_val = rbusProperty_GetName(next);
      name = rbusValue_GetString(value, &len);
      if(strcmp(rbus_val,"Device.rbusPropertyTest1") == 0) {
	  EXPECT_STREQ("test1", name) << "rbusProperty_Append failed testPushBack";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest2") == 0) {
	  EXPECT_STREQ("test2", name) << "rbusProperty_Append failed testPushBack";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest3") == 0) {
	  EXPECT_STREQ("test3", name) << "rbusProperty_Append failed testPushBack";
      }
      next = rbusProperty_GetNext(next);
  }

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);
  rbusProperty_Release(prop3);
}

TEST(rbusPropertyTest, testCount)
{
  rbusValue_t value;
  rbusProperty_t prop1;
  rbusProperty_t prop2;
  rbusProperty_t prop3;
  char const* name;
  char const* rbus_val;
  int len = 0;

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test1");
  rbusProperty_Init (&prop1, "Device.rbusPropertyTest1", value);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test2");
  rbusProperty_Init (&prop2, "Device.rbusPropertyTest2", value);
  rbusProperty_Append(prop1, prop2);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test3");
  rbusProperty_Init (&prop3, "Device.rbusPropertyTest3", value);
  rbusProperty_Append(prop2, prop3);
  rbusValue_Release(value);

  EXPECT_EQ(rbusProperty_Count(prop1),3);

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);
  rbusProperty_Release(prop3);
}

TEST(rbusPropertyTest, testFwrite)
{
  rbusValue_t value;
  rbusProperty_t prop;
  FILE *stream;
  char *pRet = NULL;
  char *stream_buf;
  size_t len;
  char type_buf[32]  = {0};
  char val_buf[1024] = {0};

  stream = open_memstream(&stream_buf, &len);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test1");
  rbusProperty_Init (&prop, "Device.rbusPropertyTest1", value);
  rbusValue_Release(value);

  rbusProperty_fwrite(prop, 0, stream);
  rbusProperty_Release(prop);

  fflush(stream);
  fclose(stream);

  pRet = strstr(stream_buf,"name=");
  pRet += strlen("name=");

  EXPECT_EQ(strncmp(pRet,"Device.rbusPropertyTest1",strlen("Device.rbusPropertyTest1")),0);

  pRet = strstr(stream_buf,"value:");
  pRet += strlen("value:");
  EXPECT_EQ(strncmp(pRet,"test1",strlen("test1")),0);
  free(stream_buf);
}

TEST(rbusPropertyTest, testGetString)
{
  char const teststring[] = "Hello World";
  char const string[] = "Good Morning";
  char const *name;
  rbusProperty_t prop1 = rbusProperty_InitString("prop1",teststring);
  rbusProperty_SetString(prop1, string);
  EXPECT_STREQ((char const*)rbusProperty_GetString(prop1, NULL), string) << "rbusProperty_GetString failed testGetValue";
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testAppendString)
{
  char const teststring[] = "Hello World";
  char const *name;
  rbusProperty_t prop;
  rbusProperty_t prop1 = rbusProperty_InitString("prop1","Device.PropertyTest");
  rbusProperty_SetString(prop1, teststring);
  name = rbusProperty_GetString(prop1, NULL);
  EXPECT_STREQ(teststring, name) << "rbusProperty_GetString failed testGetValue";
  rbusProperty_AppendString(prop1, "Device.rbusPropertyTest1", "Good Morning");
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, NegtestGetString)
{
  char const *name;
  //Negative test passing NULL value for property
  name = rbusProperty_GetString(NULL, NULL);
  EXPECT_EQ(NULL, NULL) << "rbusProperty_GetString failed testGetValue";
}

TEST(rbusPropertyTest, testGetBytes)
{
  char const teststring[] = "Hello World";
  char const string[] = "test1";
  char const *value;
  rbusProperty_t prop1 = rbusProperty_InitBytes("prop1", (uint8_t const*)teststring, strlen(teststring)+1);
  rbusProperty_SetBytes(prop1, (uint8_t const*)string, strlen(string)+1);
  EXPECT_STREQ((char const*)rbusProperty_GetBytes(prop1, NULL), string) << "rbusProperty_GetBytes failed testGetBytes";

  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetBoolean)
{
  rbusProperty_t prop1 = rbusProperty_InitBoolean("prop1",true);
  rbusProperty_SetBoolean(prop1, false);
  EXPECT_EQ(rbusProperty_GetBoolean(prop1), false);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetInt8)
{
  rbusProperty_t prop1 = rbusProperty_InitInt8("prop1",-100);
  rbusProperty_SetInt8(prop1, -123);
  EXPECT_EQ(rbusProperty_GetInt8(prop1), -123);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetUInt8)
{
  rbusProperty_t prop1 = rbusProperty_InitUInt8("prop1",100);
  rbusProperty_SetUInt8(prop1, 123);
  EXPECT_EQ(rbusProperty_GetUInt8(prop1), 123);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetInt16)
{
  rbusProperty_t prop1 = rbusProperty_InitInt16("prop1",-1234);
  rbusProperty_SetInt16(prop1, -4567);
  EXPECT_EQ(rbusProperty_GetInt16(prop1), -4567);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetUInt16)
{
  rbusProperty_t prop1 = rbusProperty_InitUInt16("prop1",1234);
  rbusProperty_SetUInt16(prop1, 4567);
  EXPECT_EQ(rbusProperty_GetUInt16(prop1), 4567);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetInt32)
{
  rbusProperty_t prop1 = rbusProperty_InitInt32("prop1",-123456);
  rbusProperty_SetInt32(prop1, -456789);
  EXPECT_EQ(rbusProperty_GetInt32(prop1), -456789);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetUInt32)
{
  rbusProperty_t prop1 = rbusProperty_InitUInt32("prop1",123456);
  rbusProperty_SetUInt32(prop1, 456789);
  EXPECT_EQ(rbusProperty_GetUInt32(prop1), 456789);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetUInt64)
{
  rbusProperty_t prop1 = rbusProperty_InitUInt64("prop1",123456789012);
  rbusProperty_SetUInt64(prop1, 456789012345);
  EXPECT_EQ(rbusProperty_GetUInt64(prop1), 456789012345);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetInt64)
{
  rbusProperty_t prop1 = rbusProperty_InitInt64("prop1",-123456789012);
  rbusProperty_SetUInt64(prop1, -456789012345);
  EXPECT_EQ(rbusProperty_GetUInt64(prop1), -456789012345);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetSingle)
{
  rbusProperty_t prop1 = rbusProperty_InitSingle("prop1",354.678f);
  rbusProperty_SetSingle(prop1, -354.678f);
  EXPECT_EQ(rbusProperty_GetSingle(prop1), -354.678f);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetDouble)
{
  rbusProperty_t prop1 = rbusProperty_InitDouble("prop1",-789.4738291023);
  rbusProperty_SetDouble(prop1, -789.4738291023);
  EXPECT_EQ(rbusProperty_GetDouble(prop1), -789.4738291023);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetProperty)
{
  rbusProperty_t prop;
  rbusProperty_Init(&prop, "MyProp", NULL);
  rbusProperty_t prop1;
  rbusProperty_Init(&prop1, "MyProp1", NULL);
  rbusProperty_t prop2 = rbusProperty_InitProperty("MyProp2",prop);
  rbusProperty_SetProperty(prop2, prop1);
  EXPECT_EQ(rbusProperty_GetProperty(prop2), prop1);
  rbusProperty_Releases(3, prop,prop1,prop2);
}

TEST(rbusPropertyTest, testGetObject)
{
  rbusObject_t obj;
  rbusObject_Init(&obj, "MyObj");
  rbusObject_t obj1;
  rbusObject_Init(&obj1, "MyObj1");
  rbusProperty_t obj2 = rbusProperty_InitObject("MyProp2",obj);
  rbusProperty_SetObject(obj2, obj1);
  EXPECT_STREQ(rbusObject_GetName(rbusProperty_GetObject(obj2)), "MyObj1");
  rbusObject_Releases(2, obj,obj1);
  rbusProperty_Release(obj2);
}

TEST(rbusPropertyTest, testGetChar)
{
  rbusProperty_t prop1 = rbusProperty_InitChar("prop1", -123);
  rbusProperty_SetChar(prop1, 123);
  EXPECT_EQ(rbusProperty_GetChar(prop1), 123);
  rbusProperty_Release(prop1);
}

TEST(rbusPropertyTest, testGetByte)
{
  rbusProperty_t prop1 = rbusProperty_InitByte("prop1", 250);
  rbusProperty_SetChar(prop1, 130);
  unsigned char s = rbusProperty_GetChar(prop1);
  EXPECT_EQ(s, 130);
  rbusProperty_Release(prop1);
}
