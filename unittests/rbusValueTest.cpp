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
#include <limits.h>
#include <errno.h>
#include "rbus_buffer.h"
#include <float.h>

TEST(rbusValueTest, validate_types)
{
  rbusValue_t val;
  rbusValue_Init(&val);

  rbusValue_SetBoolean(val, true);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);

  rbusValue_Release(val);
}

TEST(rbusValueTest, validate_bool)
{
  rbusValue_t val;
  rbusValue_Init(&val);

  rbusValue_SetBoolean(val, true);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), true);

  rbusValue_SetBoolean(val, false);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), false);

  rbusValue_SetInt32(val, 10);
  rbusValue_SetBoolean(val, true);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), true);

  rbusValue_SetString(val, "hello world");
  rbusValue_SetBoolean(val, true);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), true);

  rbusValue_SetBoolean(val, 1);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), true);

  rbusValue_SetBoolean(val, 0);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_EQ(rbusValue_GetBoolean(val), false);

  rbusValue_Release(val);
}

TEST(rbusValueTest, validate_char)
{
  rbusValue_t val;
  rbusValue_Init(&val);

  rbusValue_SetChar(val, 'a');
  EXPECT_EQ(rbusValue_GetType(val), RBUS_CHAR);
  EXPECT_EQ(rbusValue_GetChar(val), 'a');

  rbusValue_SetByte(val, 0x0a);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BYTE);
  EXPECT_EQ(rbusValue_GetByte(val), 0x0a);

  rbusValue_SetByte(val, 0xff);
  EXPECT_EQ(rbusValue_GetByte(val), 0xff);

  rbusValue_SetByte(val, static_cast<uint8_t>(0xff+1));
  EXPECT_EQ(rbusValue_GetByte(val), 0x00);

  rbusValue_Release(val);
}

static void exec_validate_test(rbusValueType_t type,char *buffer)
{
  rbusValue_t val;
  char *pRet = NULL;
  FILE *stream;
  char *stream_buf;
  size_t len;
  char type_buf[32]  = {0};
  char val_buf[1024] = {0};

  stream = open_memstream(&stream_buf, &len);
  EXPECT_NE(nullptr,stream);
  if (stream == NULL) {
    printf("Unable to open memstream. Error : %s\n",strerror(errno));
    return;
  }

  rbusValue_Init(&val);
  EXPECT_EQ(rbusValue_SetFromString(val,type,buffer),true);
  EXPECT_EQ(rbusValue_GetType(val), type);
  pRet = rbusValue_ToString(val, NULL, 0);
  if(type == RBUS_BOOLEAN) {
    if(strcmp(buffer,"true") == 0)
      EXPECT_STREQ(pRet,"true");
    if(strcmp(buffer,"false") == 0)
      EXPECT_STREQ(pRet,"false");
  } else if(type == RBUS_BYTE) {
      char tmp[4] = {0};
      sprintf(tmp,"%x",buffer[0]);
      EXPECT_STREQ(pRet,tmp);
  } else {
    if((type == RBUS_DATETIME) && (' ' == buffer[10]))
      buffer[10] = 'T';
    EXPECT_STREQ(pRet,buffer);
  }
  free(pRet);
  pRet = NULL;

  rbusValue_fwrite(val, 0,stream);
  rbusValue_Release(val);

  fflush(stream);
  fclose(stream);
  pRet = strstr(stream_buf,"value:");
  EXPECT_NE(nullptr,pRet);
  if(pRet)
  {
    pRet += strlen("value:");
    len = strlen(pRet);
    if(type == RBUS_BOOLEAN) {
      if(strcmp(buffer,"true") == 0)
        EXPECT_EQ(strncmp(pRet,"true",len-1),0);
      if(strcmp(buffer,"false") == 0)
        EXPECT_EQ(strncmp(pRet,"false",len-1),0);
    } else if(type == RBUS_BYTE) {
      char tmp[4] = {0};
      sprintf(tmp,"%x",buffer[0]);
      EXPECT_EQ(strncmp(pRet,tmp,len-1),0);
    } else {
      EXPECT_EQ(strncmp(pRet,buffer,len-1),0);
    }
  }
  free(stream_buf);
}

static void increment_val(char *buffer)
{
  char inc = 0;
  inc = buffer[strlen(buffer) - 1];
  buffer[strlen(buffer) - 1] = ++inc;
}

static void exec_validate_neg_test(rbusValueType_t type,char *buffer)
{
  rbusValue_t val;
  char *pRet = NULL;

  rbusValue_Init(&val);
  if(type == RBUS_SINGLE || type == RBUS_DOUBLE || type == RBUS_STRING ||
      type == RBUS_BYTE || type == RBUS_CHAR) {
    EXPECT_EQ(rbusValue_SetFromString(val,type,buffer),true);
  } else {
    EXPECT_NE(rbusValue_SetFromString(val,type,buffer),true);
    EXPECT_NE(rbusValue_GetType(val), type);
  }
  pRet = rbusValue_ToString(val, NULL, 0);
  if(type == RBUS_STRING || type == RBUS_CHAR) {
    EXPECT_STREQ(pRet,buffer);
  } else {
    EXPECT_STRNE(pRet,buffer);
  }
  rbusValue_Release(val);
  free(pRet);
}

static void exec_copy_compare_test(rbusValueType_t type,void *buffer)
{
  rbusValue_t val1, val2;
  int ret = -1;

  rbusValue_Init(&val1);
  rbusValue_Init(&val2);
  EXPECT_EQ(rbusValue_SetFromString(val1,type,(char *)buffer),true);
  if(RBUS_STRING == type)
    EXPECT_EQ(rbusValue_SetFromString(val2,type,"a"),true);
  if(RBUS_BYTES == type) {
    uint8_t *ptr = (uint8_t *)buffer;
    rbusValue_SetBytes(val1,ptr, 32);
    ptr[0] = rand() % 8;
    rbusValue_SetBytes(val2,ptr, 32);
  }
  rbusValue_Copy(val2, val1);

  EXPECT_EQ(rbusValue_Compare( val1, val2),0);

  rbusValue_Releases(2, val1, val2);
}

static void exec_neg_copy_compare_test(rbusValueType_t type,void *buffer)
{
  rbusValue_t val1;

  rbusValue_Init(&val1);
  EXPECT_EQ(rbusValue_SetFromString(val1,type,(char *)buffer),true);
  rbusValue_Copy(val1, val1);

  EXPECT_EQ(rbusValue_Compare( val1, val1),0);

  rbusValue_Release(val1);
}

static void value_setptr_test(rbusValueType_t type,void *buffer)
{
  rbusValue_t val1, val2;
  int ret = -1;

  rbusValue_Init(&val1);
  rbusValue_Init(&val2);
  EXPECT_EQ(rbusValue_SetFromString(val1,type,(char *)buffer),true);
  rbusValue_SetPointer(&val1, val2);

  EXPECT_EQ(rbusValue_Compare(val1, val2),0);

  rbusValue_Release(val1);
  rbusValue_Release(val2);
}

static void value_swap_test(rbusValueType_t type,void *buffer)
{
  rbusValue_t val1, val2, val3, val4;

  rbusValue_Init(&val1);
  rbusValue_Init(&val2);
  EXPECT_EQ(rbusValue_SetFromString(val1,type,(char *)buffer),true);
  EXPECT_EQ(rbusValue_SetFromString(val2,type,(char *)buffer),true);
  if(RBUS_STRING == type){
    EXPECT_EQ(rbusValue_SetFromString(val2,type,"a"),true);
  }
  val3=val1;
  val4=val2;
  rbusValue_Swap(&val1, &val2);

  EXPECT_EQ(rbusValue_Compare(val1, val4),0);
  EXPECT_EQ(rbusValue_Compare(val2, val3),0);

  rbusValue_Release(val1);
  rbusValue_Release(val2);
}

static void exec_encode_decode_tlv_test(rbusValueType_t type,void *buf)
{
  rbusValue_t valIn, valOut;
  rbusBuffer_t buff;
  bool ret = false;

  rbusValue_Init(&valIn);

  ret = rbusValue_SetFromString(valIn,type,(char *)buf);
  EXPECT_EQ(ret,true);

  if(false == ret)
    return;

  if(RBUS_BYTES == type)
    rbusValue_SetBytes(valIn, (uint8_t *)buf, 32);

  rbusBuffer_Create(&buff);
  rbusValue_Encode(valIn, buff);
  rbusValue_Decode(&valOut, buff);
  rbusBuffer_Destroy(buff);

  EXPECT_EQ(rbusValue_GetType(valIn),rbusValue_GetType(valOut));
  EXPECT_EQ(rbusValue_Compare( valIn, valOut),0);

  rbusValue_SetTLV(valOut, rbusValue_GetType(valIn),rbusValue_GetL(valIn),rbusValue_GetV(valIn));
  EXPECT_EQ(rbusValue_Compare( valIn, valOut),0);

  rbusValue_Release(valOut);
  rbusValue_Release(valIn);
}

TEST(rbusValueTest, validate_datetime1)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%TZ",timeinfo);

  exec_validate_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueTest, validate_datetime2)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%T+01:00",timeinfo);

  exec_validate_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueTest, validate_datetime3)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%T-01:00",timeinfo);

  exec_validate_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueTest, validate_datetime4)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%F %TZ",timeinfo);

  exec_validate_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueTest, validate_bool_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","true");
  exec_validate_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueTest, validate_bool_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","false");
  exec_validate_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueTest, validate_char_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","C");
  exec_validate_test(RBUS_CHAR,buffer);
}

TEST(rbusValueTest, validate_string_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","test");
  exec_validate_test(RBUS_STRING,buffer);
}

TEST(rbusValueTest, validate_byte_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","c");
  exec_validate_test(RBUS_BYTE,buffer);
}

TEST(rbusValueTest, validate_int8_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",INT8_MIN);
  exec_validate_test(RBUS_INT8,buffer);
}

TEST(rbusValueTest, validate_int8_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",INT8_MAX);
  exec_validate_test(RBUS_INT8,buffer);
}

TEST(rbusValueTest, validate_int16_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",SHRT_MIN);
  exec_validate_test(RBUS_INT16,buffer);
}

TEST(rbusValueTest, validate_int16_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",SHRT_MAX);
  exec_validate_test(RBUS_INT16,buffer);
}

TEST(rbusValueTest, validate_int32_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MIN);
  exec_validate_test(RBUS_INT32,buffer);
}

TEST(rbusValueTest, validate_int32_2)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MAX);
  exec_validate_test(RBUS_INT32,buffer);
}

TEST(rbusValueTest, validate_int64_1)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lld",(long long int)INT64_MIN);
  exec_validate_test(RBUS_INT64,buffer);
}

TEST(rbusValueTest, validate_int64_2)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lld",(long long int)INT64_MAX);
  exec_validate_test(RBUS_INT64,buffer);
}

TEST(rbusValueTest, validate_single_1)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",FLT_MAX);
  exec_validate_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueTest, validate_single_2)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",-FLT_MAX);
  exec_validate_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueTest, validate_double_1)
{
  char buffer[512] = {0};
  int len = 0;

  memset(buffer,'0',sizeof(buffer));
  sprintf(buffer,"%f",(1.0*UINT64_MAX));
  len=strlen(buffer);
  buffer[len] = '0';
  buffer[len+9] = 0;
  exec_validate_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueTest, validate_double_2)
{
  char buffer[512] = {0};
  int len = 0;

  memset(buffer,'0',sizeof(buffer));
  sprintf(buffer,"%f",(-1.0*UINT64_MAX));
  len=strlen(buffer);
  buffer[len] = '0';
  buffer[len+9] = 0;
  exec_validate_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueTest, validate_uint8_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",UCHAR_MAX);
  exec_validate_test(RBUS_UINT8,buffer);
}

TEST(rbusValueTest, validate_uint16_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",USHRT_MAX);
  exec_validate_test(RBUS_UINT16,buffer);
}

TEST(rbusValueTest, validate_uint32_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%u",UINT_MAX);
  exec_validate_test(RBUS_UINT32,buffer);
}

TEST(rbusValueTest, validate_uint64_1)
{
  char buffer[32] = {0};

  sprintf(buffer,"%llu",(long long unsigned int)UINT64_MAX);
  exec_validate_test(RBUS_UINT64,buffer);
}

TEST(rbusValueTest, validate_bytes)
{
  int i = 0, len = 32, len_cmp = 0;
  uint8_t bytes[32] = {0};
  uint8_t const* bytes_cmp;
  rbusValue_t val;

  rbusValue_Init(&val);

  for(i=0; i<len; i++)
  {
    bytes[i] = rand() % 8;
  }

  rbusValue_SetBytes(val, bytes, len);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BYTES);
  bytes_cmp = rbusValue_GetBytes(val, &len_cmp);
  EXPECT_EQ(len, len_cmp);
  EXPECT_EQ(memcmp(bytes, bytes_cmp, len), 0);
  rbusValue_Release(val);
}

//negative cases
TEST(rbusValueTestNeg, validate_bytes)
{
  int i = 0, len = 32, len_cmp = 0;
  uint8_t bytes[32] = {0};
  uint8_t const* bytes_cmp;
  rbusValue_t val;

  rbusValue_Init(&val);

  for(i=0; i<len; i++)
  {
    bytes[i] = rand() % 8;
  }

  rbusValue_SetBytes(val, bytes, len);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BYTES);
  bytes_cmp = rbusValue_GetBytes(val, &len_cmp);
  EXPECT_EQ(len, len_cmp);
  bytes[0] += 1;
  EXPECT_NE(memcmp(bytes, bytes_cmp, len), 0);
  rbusValue_Release(val);
}

TEST(rbusValueTestNeg, validate_char)
{
  rbusValue_t val;
  rbusValue_Init(&val);

  rbusValue_SetChar(val, '*');
  EXPECT_EQ(rbusValue_GetType(val), RBUS_CHAR);
  EXPECT_NE(rbusValue_GetChar(val), 'a');

  rbusValue_Release(val);
}

TEST(rbusValueTestNeg, validate_char_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","C");
  increment_val(buffer);
  exec_validate_neg_test(RBUS_CHAR,buffer);
}

TEST(rbusValueTestNeg, validate_bool)
{
  rbusValue_t val;
  rbusValue_Init(&val);

  rbusValue_SetBoolean(val, 0);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_NE(rbusValue_GetBoolean(val), true);

  rbusValue_SetBoolean(val, 1);
  EXPECT_EQ(rbusValue_GetType(val), RBUS_BOOLEAN);
  EXPECT_NE(rbusValue_GetBoolean(val), false);

  rbusValue_Release(val);
}

TEST(rbusValueTestNeg, validate_bool_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","truee");
  exec_validate_neg_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueTestNeg, validate_bool_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","ffalse");
  exec_validate_neg_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueTestNeg, validate_string_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","test");
  exec_validate_neg_test(RBUS_STRING,buffer);
}

TEST(rbusValueTestNeg, validate_datetime1)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%RZ",timeinfo);
  exec_validate_neg_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueTestNeg, validate_byte_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","c");
  increment_val(buffer);
  exec_validate_neg_test(RBUS_BYTE,buffer);
}

TEST(rbusValueTestNeg, validate_int8_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",INT8_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT8,buffer);
}

TEST(rbusValueTestNeg, validate_int8_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",INT8_MIN);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT8,buffer);
}

TEST(rbusValueTestNeg, validate_int16_1)
{
  rbusValue_t val;
  char *pRet = NULL;
  char buffer[8] = {0};

  sprintf(buffer,"%d",SHRT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT16,buffer);
}

TEST(rbusValueTestNeg, validate_int16_2)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",SHRT_MIN);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT16,buffer);
}

TEST(rbusValueTestNeg, validate_int32_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT32,buffer);
}

TEST(rbusValueTestNeg, validate_int32_2)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MIN);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT32,buffer);
}

TEST(rbusValueTestNeg, validate_int64_1)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lld",(long long int)INT64_MIN);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT64,buffer);
}

TEST(rbusValueTestNeg, validate_int64_2)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lld",(long long int)INT64_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_INT64,buffer);
}

TEST(rbusValueTestNeg, validate_single_1)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",FLT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueTestNeg, validate_single_2)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",-FLT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueTestNeg, validate_double_1)
{
  char buffer[512] = {0};
  int len = 0;

  memset(buffer,'0',sizeof(buffer));
  sprintf(buffer,"%f",(1.0*UINT64_MAX));
  len=strlen(buffer);
  buffer[len] = '0';
  buffer[len+9] = 0;
  increment_val(buffer);
  exec_validate_neg_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueTestNeg, validate_double_2)
{
  char buffer[512] = {0};
  int len = 0;

  memset(buffer,'0',sizeof(buffer));
  sprintf(buffer,"%f",(-1.0*UINT64_MAX));
  len=strlen(buffer);
  buffer[len] = '0';
  buffer[len+9] = 0;
  increment_val(buffer);
  exec_validate_neg_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueTestNeg, validate_uint8_1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",UCHAR_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_UINT8,buffer);
}

TEST(rbusValueTestNeg, validate_uint16_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",USHRT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_UINT16,buffer);
}

TEST(rbusValueTestNeg, validate_uint32_1)
{
  char buffer[16] = {0};

  sprintf(buffer,"%u",UINT_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_UINT32,buffer);
}

TEST(rbusValueTestNeg, validate_uint64_1)
{
  char buffer[32] = {0};

  sprintf(buffer,"%llu",(long long unsigned int)UINT64_MAX);
  increment_val(buffer);
  exec_validate_neg_test(RBUS_UINT64,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_uint8)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",UCHAR_MAX);
  exec_neg_copy_compare_test(RBUS_UINT8,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_bool)
{
  char buffer[8] = "true";

  exec_copy_compare_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_char)
{
  char buffer[8] = "a";

  exec_copy_compare_test(RBUS_CHAR,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_str)
{
  char buffer[8] = "test";

  exec_copy_compare_test(RBUS_STRING,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_byte)
{
  char buffer[8] = "a";

  exec_copy_compare_test(RBUS_BYTE,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_bytes)
{
  int i = 0, len = 32;
  uint8_t buffer[32] = {0};

  for(i=0; i<len; i++)
  {
    buffer[i] = rand() % 8;
  }

  exec_copy_compare_test(RBUS_BYTES,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_int8)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",CHAR_MIN);
  exec_copy_compare_test(RBUS_INT8,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_int16)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",SHRT_MIN);
  exec_copy_compare_test(RBUS_INT16,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_int32)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MIN);
  exec_copy_compare_test(RBUS_INT32,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_int64)
{
  char buffer[32] = {0};

  sprintf(buffer,"%ld",LONG_MAX);
  exec_copy_compare_test(RBUS_INT64,buffer);
}

TEST(rbusValueCopyCompare, neg_copy_compare_uint8)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",UCHAR_MAX);
  exec_copy_compare_test(RBUS_UINT8,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_uint16)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",USHRT_MAX);
  exec_copy_compare_test(RBUS_UINT16,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_uint32)
{
  char buffer[32] = {0};

  sprintf(buffer,"%u",UINT_MAX);
  exec_copy_compare_test(RBUS_UINT32,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_uint64)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lu",ULONG_MAX);
  exec_copy_compare_test(RBUS_UINT64,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_single)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",FLT_MAX);
  exec_copy_compare_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_double)
{
  char buffer[512] = {0};
  int len = 0;

  memset(buffer,'0',sizeof(buffer));

  sprintf(buffer,"%f",(1.0*UINT64_MAX));
  len=strlen(buffer);
  buffer[len] = '0';
  buffer[len+9] = 0;

  exec_copy_compare_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueCopyCompare, copy_compare_datetime)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[32] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%T-01:00",timeinfo);

  exec_copy_compare_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueSwap, swap_value1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",CHAR_MIN);
  value_swap_test(RBUS_INT8,buffer);
}

TEST(rbusValueSwap, swap_string1)
{
  char buffer[8] = "test";

  value_swap_test(RBUS_STRING,buffer);
}

TEST(rbusValueSetptr, setptr_value1)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",CHAR_MIN);
  value_setptr_test(RBUS_INT8,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_datetime)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%FT%TZ",timeinfo);

  exec_encode_decode_tlv_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_datetime1)
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80] = {0};

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer,80,"%F %TZ",timeinfo);

  exec_encode_decode_tlv_test(RBUS_DATETIME,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_bool)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","true");
  exec_encode_decode_tlv_test(RBUS_BOOLEAN,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_byte)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","c");
  exec_encode_decode_tlv_test(RBUS_BYTE,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_bytes)
{
  int i = 0, len = 32;
  uint8_t buffer[32] = {0};

  for(i=0; i<len; i++)
  {
    buffer[i] = rand() % 8;
  }
  exec_encode_decode_tlv_test(RBUS_BYTES,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_char)
{
  char buffer[8] = {0};

  sprintf(buffer,"%s","C");
  exec_encode_decode_tlv_test(RBUS_CHAR,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_int8)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",CHAR_MIN);
  exec_encode_decode_tlv_test(RBUS_INT8,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_int16)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",SHRT_MIN);
  exec_encode_decode_tlv_test(RBUS_INT16,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_int32)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",INT_MIN);
  exec_encode_decode_tlv_test(RBUS_INT32,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_int64)
{
  char buffer[32] = {0};

  sprintf(buffer,"%ld",LONG_MAX);
  exec_encode_decode_tlv_test(RBUS_INT64,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_single)
{
  char buffer[64] = {0};

  sprintf(buffer,"%f",(1.0*INT_MAX));
  exec_encode_decode_tlv_test(RBUS_SINGLE,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_double)
{
  char buffer[512] = {0};

  memset(buffer,0,sizeof(buffer));
  sprintf(buffer,"%f",(1.0*LONG_MAX));
  exec_encode_decode_tlv_test(RBUS_DOUBLE,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_uint8)
{
  char buffer[8] = {0};

  sprintf(buffer,"%d",UCHAR_MAX);
  exec_encode_decode_tlv_test(RBUS_UINT8,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_uint16)
{
  char buffer[16] = {0};

  sprintf(buffer,"%d",USHRT_MAX);
  exec_encode_decode_tlv_test(RBUS_UINT16,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_uint32)
{
  char buffer[16] = {0};

  sprintf(buffer,"%u",UINT_MAX);
  exec_encode_decode_tlv_test(RBUS_UINT32,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_uint64)
{
  char buffer[32] = {0};

  sprintf(buffer,"%lu",ULONG_MAX);
  exec_encode_decode_tlv_test(RBUS_UINT64,buffer);
}

TEST(rbusValueEncDecTlv, enc_dec_tlv_string)
{
  char buffer[16] = {0};

  sprintf(buffer,"%s","test string");
  exec_encode_decode_tlv_test(RBUS_STRING,buffer);
}
