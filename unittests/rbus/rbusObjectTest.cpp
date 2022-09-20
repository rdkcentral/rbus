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
TEST(rbusObjectTestName, testName1)
{
  rbusObject_t obj;
  rbusObject_Init(&obj, "gTestObject");
  EXPECT_STREQ(rbusObject_GetName(obj),"gTestObject");

  rbusObject_Release(obj);
}

TEST(rbusObjectTestName, testName2)
{
  rbusObject_t obj;
  rbusObject_Init(&obj, "gTestObject");
  EXPECT_STREQ(rbusObject_GetName(obj),"gTestObject");
  rbusObject_SetName(obj,"gTestObject1");
  EXPECT_STREQ(rbusObject_GetName(obj),"gTestObject1");

  rbusObject_Release(obj);
}

TEST(rbusObjectTestName, testName3)
{
  rbusObject_t obj;
  rbusObject_Init(&obj, "gTestObject");
  EXPECT_STREQ(rbusObject_GetName(obj),"gTestObject");
  rbusObject_SetName(obj,NULL);
  EXPECT_EQ(rbusObject_GetName(obj),nullptr);

  rbusObject_Release(obj);
}

TEST(rbusObjectTestValue, testSetGetValue1)
{
  rbusObject_t obj;
  rbusValue_t val;
  rbusObject_Init(&obj, "gTestObject");
  char *pRet = NULL;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);

  rbusObject_SetValue(obj,"gTestProp", val);
  rbusValue_Release(val);

  val = rbusObject_GetValue(obj,"gTestProp");
  pRet = rbusValue_ToString(val, NULL, 0);

  EXPECT_STREQ(pRet,"string");
  free(pRet);
  rbusObject_Release(obj);
}

TEST(rbusObjectTestValue, testSetGetValue2)
{
  rbusObject_t obj;
  rbusValue_t val;
  rbusObject_Init(&obj, "gTestObject");
  char *pRet = NULL;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);

  rbusObject_SetValue(obj,"gTestProp", val);
  rbusValue_Release(val);

  val = rbusObject_GetValue(obj,NULL);
  pRet = rbusValue_ToString(val, NULL, 0);

  EXPECT_STREQ(pRet,"string");
  free(pRet);
  rbusObject_Release(obj);
}

TEST(rbusObjectTestValue, testSetGetValue3)
{
  rbusObject_t obj;
  rbusValue_t val;
  rbusObject_Init(&obj, "gTestObject");

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);

  rbusObject_SetValue(obj,"gTestProp", val);
  rbusValue_Release(val);

  val = rbusObject_GetValue(obj,"gTestProp1");

  EXPECT_EQ(val,nullptr);
  rbusObject_Release(obj);
}

TEST(rbusObjectTestCompare, testCompare1)
{
  rbusObject_t obj1, obj2;
  rbusObject_t obj_ch1, obj_ch2;
  rbusProperty_t prop1, prop2;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop1, "gTestProp1", val);
  rbusValue_Release(val);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop2, "gTestProp1", val);
  rbusValue_Release(val);

  rbusObject_Init (&obj_ch1, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch1, prop1);

  rbusObject_Init (&obj_ch2, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch2, prop2);

  rbusObject_Init(&obj1, "gTestObject1");
  rbusObject_Init(&obj2, "gTestObject1");

  rbusObject_SetProperty(obj1, prop1);
  rbusObject_SetProperty(obj2, prop2);

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);

  rbusObject_SetChildren(obj1,obj_ch1);
  rbusObject_SetChildren(obj2,obj_ch2);

  EXPECT_EQ(rbusObject_Compare(obj1, obj2, true),0);

  rbusObject_Release(obj_ch1);
  rbusObject_Release(obj_ch2);
  rbusObject_Release(obj1);
  rbusObject_Release(obj2);
}

TEST(rbusObjectTestCompare, testCompare2)
{
  rbusObject_t obj1, obj2;
  rbusObject_t obj_ch1, obj_ch2;
  rbusProperty_t prop1, prop2;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop1, "gTestProp1", val);
  rbusValue_Release(val);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string2"),true);
  rbusProperty_Init (&prop2, "gTestProp1", val);
  rbusValue_Release(val);

  rbusObject_Init (&obj_ch1, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch1, prop1);

  rbusObject_Init (&obj_ch2, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch2, prop2);

  rbusObject_Init(&obj1, "gTestObject1");
  rbusObject_Init(&obj2, "gTestObject1");

  rbusObject_SetProperty(obj1, prop1);
  rbusObject_SetProperty(obj2, prop2);

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);

  rbusObject_SetChildren(obj1,obj_ch1);
  rbusObject_SetChildren(obj2,obj_ch2);

  EXPECT_NE(rbusObject_Compare(obj1, obj2, true),0);

  rbusObject_Release(obj_ch1);
  rbusObject_Release(obj_ch2);
  rbusObject_Release(obj1);
  rbusObject_Release(obj2);
}

TEST(rbusObjectTestCompare, testCompare3)
{
  rbusObject_t obj1, obj2;
  rbusObject_t obj_ch1, obj_ch2;
  rbusProperty_t prop1, prop2;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop1, "gTestProp1", val);
  rbusValue_Release(val);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop2, "gTestProp1", val);
  rbusValue_Release(val);

  rbusObject_Init (&obj_ch1, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch1, prop1);

  rbusObject_Init (&obj_ch2, "gTestObject_ch2");
  rbusObject_SetProperty(obj_ch2, prop2);

  rbusObject_Init(&obj1, "gTestObject1");
  rbusObject_Init(&obj2, "gTestObject1");

  rbusObject_SetProperty(obj1, prop1);
  rbusObject_SetProperty(obj2, prop2);

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);

  rbusObject_SetChildren(obj1,obj_ch1);
  rbusObject_SetChildren(obj2,obj_ch2);

  EXPECT_NE(rbusObject_Compare(obj1, obj2, true),0);

  rbusObject_Release(obj_ch1);
  rbusObject_Release(obj_ch2);
  rbusObject_Release(obj1);
  rbusObject_Release(obj2);
}

TEST(rbusObjectTestCompare, testCompare4)
{
  rbusObject_t obj1, obj2;
  rbusObject_t obj_ch1, obj_ch2;
  rbusProperty_t prop1, prop2;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop1, "gTestProp1", val);
  rbusValue_Release(val);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop2, "gTestProp1", val);
  rbusValue_Release(val);

  rbusObject_Init (&obj_ch1, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch1, prop1);

  rbusObject_Init (&obj_ch2, "gTestObject_ch1");
  rbusObject_SetProperty(obj_ch2, prop2);

  rbusObject_Init(&obj1, "gTestObject1");
  rbusObject_Init(&obj2, "gTestObject1");

  rbusObject_SetProperty(obj1, prop1);
  rbusObject_SetProperty(obj2, prop2);

  rbusProperty_Release(prop1);
  rbusProperty_Release(prop2);

  rbusObject_SetChildren(obj1,obj_ch1);
  rbusObject_SetChildren(obj2,obj_ch2);

  EXPECT_EQ(rbusObject_Compare(obj1, obj2, true),0);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string1"),true);
  rbusProperty_Init (&prop2, "gTestProp2", val);
  rbusValue_Release(val);
  rbusObject_SetProperty(obj_ch2, prop2);
  rbusProperty_Release(prop2);

  EXPECT_NE(rbusObject_Compare(obj1, obj2, true),0);
  rbusObject_Release(obj_ch1);
  rbusObject_Release(obj_ch2);
  rbusObject_Release(obj1);
  rbusObject_Release(obj2);
}

TEST(rbusObjectTest, testFwrite)
{
  FILE *stream;
  char *pRet = NULL;
  char *stream_buf;
  size_t len;
  rbusObject_t obj, obj_ch;
  rbusProperty_t prop, prop_ch;
  rbusValue_t val;

  stream = open_memstream(&stream_buf, &len);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"ch_string"),true);
  rbusProperty_Init (&prop_ch, "ch_gTestProp", val);
  rbusValue_Release(val);

  rbusObject_Init (&obj_ch, "ch_gTestObject");
  rbusObject_SetProperty(obj_ch, prop_ch);

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"ptr_string"),true);
  rbusProperty_Init (&prop, "ptr_gTestProp", val);
  rbusValue_Release(val);

  rbusObject_Init(&obj, "ptr_gTestObject");

  rbusObject_SetProperty(obj, prop);

  rbusProperty_Release(prop);

  rbusObject_SetChildren(obj,obj_ch);

  rbusObject_fwrite(obj, 0, stream);
  rbusObject_Release(obj_ch);
  rbusObject_Release(obj);

  fflush(stream);
  fclose(stream);

  EXPECT_NE(strstr(stream_buf,"ch_string"), nullptr);
  EXPECT_NE(strstr(stream_buf,"ptr_string"), nullptr);
  EXPECT_NE(strstr(stream_buf,"ch_gTestProp"), nullptr);
  EXPECT_NE(strstr(stream_buf,"ptr_gTestProp"), nullptr);
  EXPECT_NE(strstr(stream_buf,"ch_gTestObject"), nullptr);
  EXPECT_NE(strstr(stream_buf,"ptr_gTestObject"), nullptr);
}

