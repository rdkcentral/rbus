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
#include "rbus_buffer.h"
#include <rbus.h>

static void testEncodeDecode(rbusFilter_t f1)
{
    rbusFilter_t f2;
    rbusBuffer_t buff;
    FILE *stream;
    char *stream_buf;
    size_t len;

    rbusBuffer_Create(&buff);
    rbusFilter_Encode(f1, buff);
    stream = open_memstream(&stream_buf, &len);

    if(stream)
    {
        fwrite(buff->data, 1, buff->posWrite, stream);
        fclose(stream);

        EXPECT_EQ(rbusFilter_Decode(&f2, buff),0);
        EXPECT_EQ(rbusFilter_Compare(f1, f2),0);
        rbusFilter_Release(f2);

        rbusBuffer_Destroy(buff);
    }
    free(stream_buf);
}

TEST(rbusFilterEncDecTest, testFilterEncDec1)
{
  rbusFilter_t filter = NULL;
  rbusFilter_RelationOperator_t op = RBUS_FILTER_OPERATOR_EQUAL;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);
  rbusFilter_InitRelation(&filter, op, val);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(filter));

  testEncodeDecode(filter);
  rbusValue_Release(val);
  rbusFilter_Release(filter);
}

TEST(rbusFilterEncDecTest, testFilterEncDecLog1)
{
  rbusValue_t v1, v2, v3, v4;
  rbusFilter_t r1, r2, r3, r4, left_filter, right_filter, filter;

  rbusValue_Init(&v1);
  rbusValue_Init(&v2);
  rbusValue_Init(&v3);
  rbusValue_Init(&v4);

  rbusValue_SetInt32(v1, 10);
  rbusValue_SetInt32(v2, -10);
  rbusValue_SetInt32(v3, -5);
  rbusValue_SetInt32(v4, 5);

  EXPECT_EQ(rbusValue_SetFromString(v1,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v2,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v3,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v4,RBUS_STRING,"string"),true);

  rbusFilter_InitRelation(&r1, RBUS_FILTER_OPERATOR_GREATER_THAN, v1);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r1));
  rbusFilter_InitRelation(&r2, RBUS_FILTER_OPERATOR_LESS_THAN, v2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r2));
  rbusFilter_InitLogic(&left_filter, RBUS_FILTER_OPERATOR_OR, r1, r2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(left_filter));

  rbusFilter_InitRelation(&r4, RBUS_FILTER_OPERATOR_GREATER_THAN, v3);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r4));
  rbusFilter_InitRelation(&r3, RBUS_FILTER_OPERATOR_LESS_THAN, v4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r3));
  rbusFilter_InitLogic(&right_filter, RBUS_FILTER_OPERATOR_AND, r3, r4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(right_filter));

  rbusFilter_InitLogic(&filter, RBUS_FILTER_OPERATOR_OR, left_filter, right_filter);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));

  testEncodeDecode(filter);

  rbusValue_Release(v1);
  rbusValue_Release(v2);
  rbusValue_Release(v3);
  rbusValue_Release(v4);
  rbusFilter_Release(r1);
  rbusFilter_Release(r2);
  rbusFilter_Release(r3);
  rbusFilter_Release(r4);
  rbusFilter_Release(left_filter);
  rbusFilter_Release(right_filter);
  rbusFilter_Release(filter);
}

TEST(rbusFilterEncDecTest, testFilterEncDecLog2)
{
  rbusValue_t v1, v2, v3, v4;
  rbusFilter_t r1, r2, r3, r4, left_filter, right_filter, filter;

  rbusValue_Init(&v1);
  rbusValue_Init(&v2);
  rbusValue_Init(&v3);
  rbusValue_Init(&v4);

  rbusValue_SetInt32(v1, 10);
  rbusValue_SetInt32(v2, -10);
  rbusValue_SetInt32(v3, -5);
  rbusValue_SetInt32(v4, 5);

  EXPECT_EQ(rbusValue_SetFromString(v1,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v2,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v3,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v4,RBUS_STRING,"string"),true);

  rbusFilter_InitRelation(&r1, RBUS_FILTER_OPERATOR_GREATER_THAN, v1);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r1));
  rbusFilter_InitRelation(&r2, RBUS_FILTER_OPERATOR_LESS_THAN, v2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r2));
  rbusFilter_InitLogic(&left_filter, RBUS_FILTER_OPERATOR_OR, r1, r2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(left_filter));

  rbusFilter_InitRelation(&r4, RBUS_FILTER_OPERATOR_GREATER_THAN, v3);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r4));
  rbusFilter_InitRelation(&r3, RBUS_FILTER_OPERATOR_LESS_THAN, v4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r3));
  rbusFilter_InitLogic(&right_filter, RBUS_FILTER_OPERATOR_AND, r3, r4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(right_filter));

  rbusFilter_InitLogic(&filter, RBUS_FILTER_OPERATOR_NOT, left_filter, right_filter);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));

  testEncodeDecode(filter);

  rbusValue_Release(v1);
  rbusValue_Release(v2);
  rbusValue_Release(v3);
  rbusValue_Release(v4);
  rbusFilter_Release(r1);
  rbusFilter_Release(r2);
  rbusFilter_Release(r3);
  rbusFilter_Release(r4);
  rbusFilter_Release(left_filter);
  rbusFilter_Release(right_filter);
  rbusFilter_Release(filter);
}

TEST(rbusFilterEncDecTest, testFilterEncDecLog3)
{
  rbusValue_t v1, v2, v3, v4;
  rbusFilter_t r1, r2, r3, r4, left_filter, right_filter, filter;

  rbusValue_Init(&v1);
  rbusValue_Init(&v2);
  rbusValue_Init(&v3);
  rbusValue_Init(&v4);

  rbusValue_SetInt32(v1, 10);
  rbusValue_SetInt32(v2, 10);
  rbusValue_SetInt32(v3, 10);
  rbusValue_SetInt32(v4, 10);

  EXPECT_EQ(rbusValue_SetFromString(v1,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v2,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v3,RBUS_STRING,"string"),true);
  EXPECT_EQ(rbusValue_SetFromString(v4,RBUS_STRING,"string"),true);

  rbusFilter_InitRelation(&r1, RBUS_FILTER_OPERATOR_EQUAL, v1);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r1));
  rbusFilter_InitRelation(&r2, RBUS_FILTER_OPERATOR_EQUAL, v2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r2));
  rbusFilter_InitLogic(&left_filter, RBUS_FILTER_OPERATOR_OR, r1, r2);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(left_filter));

  rbusFilter_InitRelation(&r3, RBUS_FILTER_OPERATOR_EQUAL, v3);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r3));
  rbusFilter_InitRelation(&r4, RBUS_FILTER_OPERATOR_EQUAL, v4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(r4));
  rbusFilter_InitLogic(&right_filter, RBUS_FILTER_OPERATOR_OR, r3, r4);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(right_filter));

  rbusFilter_InitLogic(&filter, RBUS_FILTER_OPERATOR_OR, left_filter, right_filter);
  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));

  testEncodeDecode(filter);

  rbusValue_Release(v1);
  rbusValue_Release(v2);
  rbusValue_Release(v3);
  rbusValue_Release(v4);
  rbusFilter_Release(r1);
  rbusFilter_Release(r2);
  rbusFilter_Release(r3);
  rbusFilter_Release(r4);
  rbusFilter_Release(left_filter);
  rbusFilter_Release(right_filter);
  rbusFilter_Release(filter);
}

TEST(rbusFilterInitTest, testFilterInit1)
{
  rbusFilter_t filter = NULL;
  rbusFilter_RelationOperator_t op = RBUS_FILTER_OPERATOR_EQUAL;
  rbusValue_t val;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);
  rbusFilter_InitRelation(&filter, op, val);

  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(filter));
  rbusValue_Release(val);
  rbusFilter_Release(filter);
}

TEST(rbusFilterInitTest, testFilterInit2)
{
  rbusFilter_t filter = NULL;
  rbusFilter_LogicOperator_t op = RBUS_FILTER_OPERATOR_AND;
  rbusFilter_InitLogic(&filter, op, NULL,NULL);

  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));
  EXPECT_EQ(nullptr,rbusFilter_GetLogicLeft(filter));
  EXPECT_EQ(nullptr,rbusFilter_GetLogicRight(filter));
  rbusFilter_Release(filter);
}

TEST(rbusFilterInitTest, testFilterInit3)
{
  rbusFilter_t filter,filter_left,filter_right;
  rbusFilter_LogicOperator_t op = RBUS_FILTER_OPERATOR_AND;
  rbusFilter_InitLogic(&filter_left, op, NULL,NULL);
  rbusFilter_InitLogic(&filter_right, op, NULL,NULL);
  rbusFilter_InitLogic(&filter, op, filter_left,filter_right);

  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));
  EXPECT_EQ(filter_left,rbusFilter_GetLogicLeft(filter));
  EXPECT_EQ(filter_right,rbusFilter_GetLogicRight(filter));
  rbusFilter_Release(filter);
  rbusFilter_Release(filter_left);
  rbusFilter_Release(filter_right);
}

static void execRbusFilterLogicOperatorTest(rbusFilter_LogicOperator_t op)
{
  rbusFilter_t filter;
  rbusFilter_InitLogic(&filter, op, NULL,NULL);

  EXPECT_EQ(RBUS_FILTER_EXPRESSION_LOGIC,rbusFilter_GetType(filter));
  EXPECT_EQ(op,rbusFilter_GetLogicOperator(filter));
  rbusFilter_Release(filter);
}

static void execRbusFilterRelOperatorTest(rbusFilter_RelationOperator_t op)
{
  rbusFilter_t filter = NULL;
  rbusValue_t val, val_cmp;

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,RBUS_STRING,"string"),true);
  rbusFilter_InitRelation(&filter, op, val);

  EXPECT_EQ(RBUS_FILTER_EXPRESSION_RELATION,rbusFilter_GetType(filter));
  EXPECT_EQ(op,rbusFilter_GetRelationOperator(filter));
  val_cmp = rbusFilter_GetRelationValue(filter);
  EXPECT_EQ(rbusValue_Compare( val, val_cmp),0);
  rbusValue_Release(val);
  rbusFilter_Release(filter);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator1)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_EQUAL);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator2)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_NOT_EQUAL);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator3)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator4)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_LESS_THAN);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator5)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_GREATER_THAN);
}

TEST(rbusFilterRelOperatorTest, testFilterRelOperator6)
{
  execRbusFilterRelOperatorTest(RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL);
}

TEST(rbusFilterLogicOperatorTest, testFilterLogicOperator1)
{
  execRbusFilterLogicOperatorTest(RBUS_FILTER_OPERATOR_AND);
}

TEST(rbusFilterLogicOperatorTest, testFilterLogicOperator2)
{
  execRbusFilterLogicOperatorTest(RBUS_FILTER_OPERATOR_OR);
}

TEST(rbusFilterLogicOperatorTest, testFilterLogicOperator3)
{
  execRbusFilterLogicOperatorTest(RBUS_FILTER_OPERATOR_NOT);
}

static void execRbusFilterApplyTest(char *buffer1, char *buffer2, char *filter_buf,rbusFilter_LogicOperator_t op_logic,rbusFilter_RelationOperator_t op_rel)
{
  rbusValue_t val1, val2, val3;
  rbusFilter_t filter_rel1, filter_rel2, filter;
  bool ret;
  FILE *stream;
  char *stream_buf;
  size_t len;

  stream = open_memstream(&stream_buf, &len);

  rbusValue_Init(&val1);
  EXPECT_EQ(rbusValue_SetFromString(val1,RBUS_STRING,buffer1),true);
  rbusFilter_InitRelation(&filter_rel1, op_rel, val1);

  rbusValue_Init(&val2);
  EXPECT_EQ(rbusValue_SetFromString(val2,RBUS_STRING,((buffer2) ? (buffer2):(""))),true);
  rbusFilter_InitRelation(&filter_rel2, op_rel, val2);

  rbusValue_Init(&val3);
  EXPECT_EQ(rbusValue_SetFromString(val3,RBUS_STRING,filter_buf),true);

  rbusFilter_InitLogic(&filter, op_logic, filter_rel1,filter_rel2);

  rbusFilter_fwrite(filter,stream,val3);

  EXPECT_EQ(rbusFilter_Apply(filter,val3),true);

  rbusFilter_Release(filter);
  rbusFilter_Release(filter_rel1);
  rbusFilter_Release(filter_rel2);
  rbusValue_Release(val1);
  rbusValue_Release(val2);
  rbusValue_Release(val3);
  fflush(stream);
  fclose(stream);

  switch(op_logic)
  {
    case RBUS_FILTER_OPERATOR_AND:
      EXPECT_NE(nullptr,strstr(stream_buf,"&&"));
    break;
    case RBUS_FILTER_OPERATOR_OR:
      EXPECT_NE(nullptr,strstr(stream_buf,"||"));
    break;
    case RBUS_FILTER_OPERATOR_NOT:
      EXPECT_NE(nullptr,strstr(stream_buf,"!("));
    break;
  }
  switch(op_rel)
  {
    case RBUS_FILTER_OPERATOR_EQUAL:
      EXPECT_NE(nullptr,strstr(stream_buf,"=="));
    break;
    case RBUS_FILTER_OPERATOR_NOT_EQUAL:
      EXPECT_NE(nullptr,strstr(stream_buf,"!="));
    break;
    case RBUS_FILTER_OPERATOR_GREATER_THAN:
      EXPECT_NE(nullptr,strstr(stream_buf,">"));
    break;
    case RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL:
      EXPECT_NE(nullptr,strstr(stream_buf,">="));
    break;
    case RBUS_FILTER_OPERATOR_LESS_THAN:
      EXPECT_NE(nullptr,strstr(stream_buf,"<"));
    break;
    case RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL:
      EXPECT_NE(nullptr,strstr(stream_buf,"<="));
    break;
  }
  free(stream_buf);
}

TEST(rbusFilterApplyTest, testFilterApply1)
{
  char buffer1[]="test string";
  char buffer2[]="test string";
  char filter_buf[]="test string";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply2)
{
  char buffer1[]="test string";
  char buffer2[]="test string";
  char filter_buf[]="test strin1";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_NOT_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply3)
{
  char buffer1[]="test string";
  char buffer2[]="test string";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_GREATER_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply4)
{
  char buffer1[]="test string";
  char buffer2[]="test string";
  char filter_buf[]="test stri";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_LESS_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply5)
{
  char buffer1[]="test string";
  char buffer2[]="test string12";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply6)
{
  char buffer1[]="test string";
  char buffer2[]="test stri";
  char filter_buf[]="test stri";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_AND, RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply7)
{
  char buffer1[]="test string";
  char buffer2[]="test strin1";
  char filter_buf[]="test string";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply8)
{
  char buffer1[]="test striqg";
  char buffer2[]="test string";
  char filter_buf[]="test strin1";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_NOT_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply9)
{
  char buffer1[]="test string";
  char buffer2[]="test string";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_GREATER_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply10)
{
  char buffer1[]="test str";
  char buffer2[]="test string";
  char filter_buf[]="test stri";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_LESS_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply11)
{
  char buffer1[]="test string";
  char buffer2[]="test string12";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply12)
{
  char buffer1[]="test string";
  char buffer2[]="test stri";
  char filter_buf[]="test stri";

  execRbusFilterApplyTest(buffer1, buffer2, filter_buf, RBUS_FILTER_OPERATOR_OR, RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply13)
{
  char buffer[]="test string1";
  char filter_buf[]="test string2";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply14)
{
  char buffer[]="test string1";
  char filter_buf[]="test string1";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_NOT_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply15)
{
  char buffer[]="test string12";
  char filter_buf[]="test string1";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_GREATER_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply16)
{
  char buffer[]="test string1";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_LESS_THAN);
}

TEST(rbusFilterApplyTest, testFilterApply17)
{
  char buffer[]="test string12";
  char filter_buf[]="test string1";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL);
}

TEST(rbusFilterApplyTest, testFilterApply18)
{
  char buffer[]="test string1";
  char filter_buf[]="test string12";

  execRbusFilterApplyTest(buffer, NULL, filter_buf, RBUS_FILTER_OPERATOR_NOT, RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL);
}
