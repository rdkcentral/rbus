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

  rbusObject_Releases(4, obj_ch1, obj_ch2, obj1, obj2);
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
  rbusProperty_Release(prop_ch);

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
  free(stream_buf);
}

TEST(rbusObjectTest, testGetPropertyString)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  rbusValueError_t ret;
  char const* name = "hello";

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);

  rbusObject_SetPropertyString(obj, "string", name);
  ret = rbusObject_GetPropertyString(obj, "string", &name, NULL);
  EXPECT_EQ(ret,RBUS_VALUE_ERROR_SUCCESS) << "rbusObjectTest failed testGetPropertyBytes";
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetParent)
{
  rbusObject_t obj1, obj2;
  rbusProperty_t prop1, prop2;
  rbusObject_Init(&obj1, "gTestObject1");
  rbusObject_Init(&obj2, "gTestObject1");
  rbusProperty_Init(&prop1, "prop1", NULL);
  rbusProperty_Init(&prop2, "prop2", NULL);
  rbusObject_SetProperty(obj1, prop1);
  rbusObject_SetProperty(obj2, prop2);

  rbusObject_SetParent(obj1, obj2);
  EXPECT_EQ(rbusObject_GetParent(obj1),obj2);
  rbusProperty_Releases(2, prop1,prop2);
  rbusObject_Releases(2, obj1, obj2);
}

TEST(rbusObjectTest, testGetBoolean)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  bool val;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);

  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyBoolean(obj, "bool", true);

  EXPECT_EQ(rbusObject_GetPropertyBoolean(obj, "notexist", &val),RBUS_VALUE_ERROR_NOT_FOUND);
  EXPECT_EQ(rbusObject_GetPropertyBoolean(obj, "nulval", &val),RBUS_VALUE_ERROR_NULL);
  EXPECT_EQ(rbusObject_GetPropertyBoolean(obj, "bool", &val),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetInt8)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  int8_t i8;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyInt8(obj, "int8", -123);

  EXPECT_EQ(rbusObject_GetPropertyInt8(obj, "int8", &i8),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetUInt8)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  uint8_t i8;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyUInt8(obj, "uint8", 123);

  EXPECT_EQ(rbusObject_GetPropertyUInt8(obj, "uint8", &i8),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetInt16)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  int16_t i16;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyInt16(obj, "int16", -1234);

  EXPECT_EQ(rbusObject_GetPropertyInt16(obj, "int16", &i16),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetUInt16)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  uint16_t i16;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyUInt16(obj, "uint16", 1234);

  EXPECT_EQ(rbusObject_GetPropertyUInt16(obj, "uint16", &i16),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetInt32)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  int32_t i32;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyInt32(obj, "int32", -123456);

  EXPECT_EQ(rbusObject_GetPropertyInt32(obj, "int32", &i32),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetUInt32)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  uint32_t i32;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyUInt32(obj, "uint32", 123456);

  EXPECT_EQ(rbusObject_GetPropertyUInt32(obj, "uint32", &i32),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetInt64)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  int64_t i64;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyInt64(obj, "int64", -123456789012);

  EXPECT_EQ(rbusObject_GetPropertyInt64(obj, "int64", &i64),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetUInt64)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  uint64_t i64;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyUInt64(obj, "int64", -123456789012);

  EXPECT_EQ(rbusObject_GetPropertyUInt64(obj, "int64", &i64),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetSingle)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  float val;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertySingle(obj, "float", 123.456);

  EXPECT_EQ(rbusObject_GetPropertySingle(obj, "float", &val),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetDouble)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  double val;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyDouble(obj, "double", 123.456789);

  EXPECT_EQ(rbusObject_GetPropertyDouble(obj, "double", &val),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetChar)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  char val;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyChar(obj, "char", -123);

  EXPECT_EQ(rbusObject_GetPropertyChar(obj, "char", &val),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}

TEST(rbusObjectTest, testGetByte)
{
  rbusObject_t obj;
  rbusProperty_t prop1;
  unsigned char val;

  rbusObject_Init(&obj, NULL);
  rbusProperty_Init(&prop1, "nulval", NULL);
  rbusObject_SetProperty(obj, prop1);
  rbusObject_SetPropertyByte(obj, "uchar", 123);

  EXPECT_EQ(rbusObject_GetPropertyByte(obj, "uchar", &val),RBUS_VALUE_ERROR_SUCCESS);
  rbusObject_Release(obj);
  rbusProperty_Release(prop1);
}
