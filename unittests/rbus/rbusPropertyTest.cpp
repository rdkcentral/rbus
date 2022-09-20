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
  rbusProperty_SetNext(prop1, prop2);
  rbusValue_Release(value);

  rbusValue_Init(&value);
  rbusValue_SetString(value, "test3");
  rbusProperty_Init (&prop3, "Device.rbusPropertyTest3", value);
  rbusProperty_SetNext(prop2, prop3);
  rbusValue_Release(value);

  while(prop1) {
      value = rbusProperty_GetValue(prop1);
      rbus_val = rbusProperty_GetName(prop1);
      name = rbusValue_GetString(value, &len);
      if(strcmp(rbus_val,"Device.rbusPropertyTest1") == 0) {
	  EXPECT_STREQ("test1", name) << "rbusProperty_GetNext failed testGetNext";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest2") == 0) {
	  EXPECT_STREQ("test2", name) << "rbusProperty_GetNext failed testGetNext";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest3") == 0) {
	  EXPECT_STREQ("test3", name) << "rbusProperty_GetNext failed testGetNext";
      }
      prop1 = rbusProperty_GetNext(prop1);
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

  while(prop1) {
      value = rbusProperty_GetValue(prop1);
      rbus_val = rbusProperty_GetName(prop1);
      name = rbusValue_GetString(value, &len);
      if(strcmp(rbus_val,"Device.rbusPropertyTest1") == 0) {
	  EXPECT_STREQ("test1", name) << "rbusProperty_Append failed testPushBack";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest2") == 0) {
	  EXPECT_STREQ("test2", name) << "rbusProperty_Append failed testPushBack";
      } else if(strcmp(rbus_val,"Device.rbusPropertyTest3") == 0) {
	  EXPECT_STREQ("test3", name) << "rbusProperty_Append failed testPushBack";
      }
      prop1 = rbusProperty_GetNext(prop1);
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
}
