/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <endian.h>
#include <float.h>
#include <unistd.h>
#include <rbus.h>
#include "rbus_buffer.h"
#include <math.h>
#include "../common/test_macros.h"
#include <rtMemory.h>
#include "rbus_value.h"

int getDurationValueAPI()
{
    return 1;
}

void testValue_Type()
{
    rbusValue_t val;
    rbusDateTime_t tv;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&val);

    rbusValue_SetBoolean(val,true);
    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);

    rbusValue_SetChar(val,0);
    TEST(rbusValue_GetType(val)==RBUS_CHAR);

    rbusValue_SetByte(val,0);
    TEST(rbusValue_GetType(val)==RBUS_BYTE);
    /*
    rbusValue_SetInt8(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT8);

    rbusValue_SetUInt8(val,0);
    TEST(rbusValue_GetType(val)==RBUS_UINT8);
    */
    rbusValue_SetInt16(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT16);

    rbusValue_SetUInt16(val,0);
    TEST(rbusValue_GetType(val)==RBUS_UINT16);

    rbusValue_SetInt32(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT32);

    rbusValue_SetUInt32(val,0);
    TEST(rbusValue_GetType(val)==RBUS_UINT32);

    rbusValue_SetInt64(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT64);

    rbusValue_SetUInt64(val,0);
    TEST(rbusValue_GetType(val)==RBUS_UINT64);

    rbusValue_SetSingle(val,0);
    TEST(rbusValue_GetType(val)==RBUS_SINGLE);

    rbusValue_SetDouble(val,0);
    TEST(rbusValue_GetType(val)==RBUS_DOUBLE);

    rbusValue_SetTime(val,&tv);
    TEST(rbusValue_GetType(val)==RBUS_DATETIME);

    rbusValue_SetString(val,"");
    TEST(rbusValue_GetType(val)==RBUS_STRING);

    rbusValue_SetBytes(val,(uint8_t*)"", 0);
    TEST(rbusValue_GetType(val)==RBUS_BYTES);

    /*set from buffer back type to integer*/
    rbusValue_SetInt32(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT32);

    /*test NULL buffers and swithing to and from integer*/
    rbusValue_SetString(val,NULL);
    TEST(rbusValue_GetType(val)==RBUS_STRING);

    rbusValue_SetBytes(val,NULL,0);
    TEST(rbusValue_GetType(val)==RBUS_BYTES);

    rbusValue_SetInt32(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT32);

    rbusValue_SetBytes(val,NULL,0);
    TEST(rbusValue_GetType(val)==RBUS_BYTES);

    rbusValue_SetString(val,NULL);
    TEST(rbusValue_GetType(val)==RBUS_STRING);

    rbusValue_SetString(val,NULL);
    TEST(rbusValue_GetType(val)==RBUS_STRING);

    rbusValue_SetInt32(val,0);
    TEST(rbusValue_GetType(val)==RBUS_INT32);

    rbusValue_Release(val);
}

void testValue_Bool()
{
    rbusValue_t val;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&val);

    rbusValue_SetBoolean(val, true);

    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);

    TEST(rbusValue_GetBoolean(val) == true);

    rbusValue_SetBoolean(val, false);
    TEST(rbusValue_GetBoolean(val) == false);

    rbusValue_SetBoolean(val, true);
    TEST(rbusValue_GetBoolean(val) == true);

    rbusValue_SetBoolean(val, true);
    TEST(rbusValue_GetBoolean(val) == true);

    rbusValue_SetBoolean(val, false);
    TEST(rbusValue_GetBoolean(val) == false);

    rbusValue_SetBoolean(val, false);
    TEST(rbusValue_GetBoolean(val) == false);

    /*set it to another type then set it back*/
    rbusValue_SetInt32(val, 1);
    TEST(rbusValue_GetType(val)!=RBUS_BOOLEAN);
    rbusValue_SetBoolean(val, false);
    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(val) == false);

    rbusValue_SetInt32(val, 1);
    TEST(rbusValue_GetType(val)!=RBUS_BOOLEAN);
    rbusValue_SetBoolean(val, true);
    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(val) == true);

    rbusValue_SetString(val, "Test");
    TEST(rbusValue_GetType(val)!=RBUS_BOOLEAN);
    rbusValue_SetBoolean(val, true);
    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(val) == true);

    rbusValue_SetString(val, "Test");
    TEST(rbusValue_GetType(val)!=RBUS_BOOLEAN);
    rbusValue_SetBoolean(val, false);
    TEST(rbusValue_GetType(val)==RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(val) == false);

    rbusValue_Release(val);
}

void testValue_Char()
{
    rbusValue_t val;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&val);

    rbusValue_SetChar(val, 'a');
    TEST(rbusValue_GetType(val)==RBUS_CHAR);
    TEST(rbusValue_GetChar(val)=='a');

    rbusValue_SetByte(val, 0x0a);
    TEST(rbusValue_GetType(val)==RBUS_BYTE);
    TEST(rbusValue_GetByte(val)==0x0a);

    rbusValue_SetByte(val, 0xff);
    TEST(rbusValue_GetByte(val)==0xff);

    rbusValue_SetByte(val, 0xff+1);
    TEST(rbusValue_GetByte(val)==0x00);

    rbusValue_Release(val);
}

void testValue_Ints()
{
    rbusValue_t val, val1, val2;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&val);

    /*
     * Test min/max and GetType
     */
#if 0
    /*INT8*/
    rbusValue_SetInt8(val, INT8_MIN);
    TEST(rbusValue_GetInt8(val) == INT8_MIN);

    rbusValue_SetInt8(val, INT8_MAX);
    TEST(rbusValue_GetInt8(val) == INT8_MAX);

    rbusValue_SetInt8(val, INT8_MIN-1);
    TEST(rbusValue_GetInt8(val) == INT8_MAX);

    rbusValue_SetInt8(val, INT8_MAX+1);
    TEST(rbusValue_GetInt8(val) == INT8_MIN);

    /*UINT8*/
    rbusValue_SetUInt8(val, 0);
    TEST(rbusValue_GetUInt8(val) == 0);

    rbusValue_SetUInt8(val, UINT8_MAX);
    TEST(rbusValue_GetUInt8(val) == UINT8_MAX);

    rbusValue_SetUInt8(val, -1);
    TEST(rbusValue_GetUInt8(val) == UINT8_MAX);

    rbusValue_SetUInt8(val, UINT8_MAX+1);
    TEST(rbusValue_GetUInt8(val) == 0);
#endif 
    /*INT16*/
    rbusValue_SetInt16(val, INT16_MIN);
    TEST(rbusValue_GetInt16(val) == INT16_MIN);

    rbusValue_SetInt16(val, INT16_MAX);
    TEST(rbusValue_GetInt16(val) == INT16_MAX);

    rbusValue_SetInt16(val, INT16_MIN-1);
    TEST(rbusValue_GetInt16(val) == INT16_MAX);

    rbusValue_SetInt16(val, INT16_MAX+1);
    TEST(rbusValue_GetInt16(val) == INT16_MIN);

    /*UINT16*/
    rbusValue_SetUInt16(val, 0);
    TEST(rbusValue_GetUInt16(val) == 0);

    rbusValue_SetUInt16(val, UINT16_MAX);
    TEST(rbusValue_GetUInt16(val) == UINT16_MAX);

    rbusValue_SetUInt16(val, -1);
    TEST(rbusValue_GetUInt16(val) == UINT16_MAX);

    rbusValue_SetUInt16(val, UINT16_MAX+1);
    TEST(rbusValue_GetUInt16(val) == 0);

    /*INT32*/
    rbusValue_SetInt32(val, INT32_MIN);
    TEST(rbusValue_GetInt32(val) == INT32_MIN);

    rbusValue_SetInt32(val, INT32_MAX);
    TEST(rbusValue_GetInt32(val) == INT32_MAX);

    rbusValue_SetInt32(val, INT32_MIN-1);
    TEST(rbusValue_GetInt32(val) == INT32_MAX);

    rbusValue_SetInt32(val, INT32_MAX+1);
    TEST(rbusValue_GetInt32(val) == INT32_MIN);

    /*UINT16*/
    rbusValue_SetUInt32(val, 0);
    TEST(rbusValue_GetUInt32(val) == 0);

    rbusValue_SetUInt32(val, UINT32_MAX);
    TEST(rbusValue_GetUInt32(val) == UINT32_MAX);

    rbusValue_SetUInt32(val, -1);
    TEST(rbusValue_GetUInt32(val) == UINT32_MAX);

    rbusValue_SetUInt32(val, UINT32_MAX+1);
    TEST(rbusValue_GetUInt32(val) == 0);

    /*INT64*/
    rbusValue_SetInt64(val, INT64_MIN);
    TEST(rbusValue_GetInt64(val) == INT64_MIN);

    rbusValue_SetInt64(val, INT64_MAX);
    TEST(rbusValue_GetInt64(val) == INT64_MAX);

    rbusValue_SetInt64(val, INT64_MIN-1);
    TEST(rbusValue_GetInt64(val) == INT64_MAX);

    rbusValue_SetInt64(val, INT64_MAX+1);
    TEST(rbusValue_GetInt64(val) == INT64_MIN);

    /*UINT64*/
    rbusValue_SetUInt64(val, 0);
    TEST(rbusValue_GetUInt64(val) == 0);

    rbusValue_SetUInt64(val, UINT64_MAX);
    TEST(rbusValue_GetUInt64(val) == UINT64_MAX);

    rbusValue_SetUInt64(val, -1);
    TEST(rbusValue_GetUInt64(val) == UINT64_MAX);

    rbusValue_SetUInt64(val, UINT64_MAX+1);
    TEST(rbusValue_GetUInt64(val) == 0);

    rbusValue_Release(val);

    /*
     * Compare values between types of different bits
     */
    rbusValue_Init(&val1);
    rbusValue_Init(&val2);

    rbusValue_SetInt16(val1, 25000);
    rbusValue_SetInt32(val2, 25000);
    TEST(rbusValue_GetInt16(val1) == 25000);
    TEST(rbusValue_GetInt32(val2) == 25000);
    TEST(rbusValue_GetInt16(val1) == rbusValue_GetInt32(val2));

    rbusValue_SetInt32(val1, 1500250);
    rbusValue_SetInt64(val2, 1500250);
    TEST(rbusValue_GetInt32(val1) == 1500250);
    TEST(rbusValue_GetInt64(val2) == 1500250);
    TEST(rbusValue_GetInt32(val1) == rbusValue_GetInt64(val2));

    rbusValue_SetInt16(val1, -9022);
    rbusValue_SetInt64(val2, -9022);
    TEST(rbusValue_GetInt16(val1) == -9022);
    TEST(rbusValue_GetInt64(val2) == -9022);
    TEST(rbusValue_GetInt16(val1) == rbusValue_GetInt64(val2));

    rbusValue_SetUInt16(val1, 60247);
    rbusValue_SetUInt32(val2, 60247);
    TEST(rbusValue_GetUInt16(val1) == 60247);
    TEST(rbusValue_GetUInt32(val2) == 60247);
    TEST(rbusValue_GetUInt16(val1) == rbusValue_GetUInt32(val2));

    rbusValue_SetUInt32(val1, 4100200300);
    rbusValue_SetUInt64(val2, 4100200300);
    TEST(rbusValue_GetUInt32(val1) == 4100200300);
    TEST(rbusValue_GetUInt64(val2) == 4100200300);
    TEST(rbusValue_GetUInt32(val1) == rbusValue_GetUInt64(val2));

    rbusValue_SetUInt16(val1, 12);
    rbusValue_SetUInt64(val2, 12);
    TEST(rbusValue_GetUInt16(val1) == 12);
    TEST(rbusValue_GetUInt64(val2) == 12);
    TEST(rbusValue_GetUInt16(val1) == rbusValue_GetUInt64(val2));

    rbusValue_Release(val1);
    rbusValue_Release(val2);
}

void testValue_Floats()
{
    int i;
    rbusValue_t val;

    rbusValue_Init(&val);

    printf("%s\n",__FUNCTION__);
    /*some very very basic tests*/

    float singleValues[4] = { 
        -0.000001f,
         0.000001f,
         12345.6789f,
        -9876.54321f 
    };
    for(i=0; i < 4; ++i)
    {
        rbusValue_SetSingle(val, singleValues[i]);
        TEST(rbusValue_GetType(val)==RBUS_SINGLE);
        //printf("test %.*f = %.*f\n", FLT_DIG, rbusValue_GetSingle(val), FLT_DIG, singleValues[i]);
        TEST(rbusValue_GetSingle(val) == singleValues[i]);
    }

    float doubleValues[4] = { 
        -0.000000000000001,
         0.000000000000001,
         999999999999999.1,
        -999999999999999.1 
    };
    for(i=0; i < 4; ++i)
    {
        rbusValue_SetDouble(val, doubleValues[i]);
        TEST(rbusValue_GetType(val)==RBUS_DOUBLE);
        //printf("test %.*f = %.*f\n", FLT_DIG, rbusValue_GetDouble(val), FLT_DIG, doubleValues[i]);
        TEST(rbusValue_GetDouble(val) == doubleValues[i]);
    }

    rbusValue_Release(val);
}

void testValue_Time()
{
    rbusDateTime_t tv1 = {{0},{0}};
    rbusDateTime_t const* tv2;
    time_t nowtime = 0;

    rbusValue_t val;

    rbusValue_Init(&val);

    printf("%s\n",__FUNCTION__);

    rbusValue_MarshallTMtoRBUS(&tv1, localtime(&nowtime));
    //memcpy(&(tv1.m_time), localtime(&nowtime),sizeof(struct tm));
    rbusValue_SetTime(val, &tv1);
    TEST(rbusValue_GetType(val)==RBUS_DATETIME);
    tv2 = rbusValue_GetTime(val);
    TEST(memcmp(&tv1, tv2, sizeof(rbusDateTime_t)) == 0);

    sleep(1);
    /*test replacing time works*/
    //memcpy(&(tv1.m_time), localtime(&nowtime),sizeof(struct tm));
    rbusValue_MarshallTMtoRBUS(&tv1, localtime(&nowtime));
    rbusValue_SetTime(val, &tv1);
    tv2 = rbusValue_GetTime(val);
    TEST(memcmp(&tv1, tv2, sizeof(rbusDateTime_t)) == 0);

    rbusValue_Release(val);
}

void testValue_String()
{
    int i;
    rbusValue_t val;

    rbusValue_Init(&val);

    printf("%s\n",__FUNCTION__);

    /*running through strings of various lengths*/
    char const* strTest;
    int strTestLen;
    rbusValue_SetString(val, NULL);
    strTest = rbusValue_GetString(val, &strTestLen);
    TEST(rbusValue_GetType(val) == RBUS_STRING);
    TEST(strTestLen == 0);
    TEST(strTest == NULL);

    rbusValue_SetString(val, "");
    strTest = rbusValue_GetString(val, &strTestLen);
    TEST(strTestLen == 0);
    TEST(strcmp(strTest,"")==0);

    int minChar=32;
    int maxChar=126;
    int charRange=maxChar-minChar+1;
    int numStrings = 1000;
    //printf("testing strings of size 1 to %d\n", numStrings);
    for(i=1; i<=numStrings; ++i)
    {
        int len;
        char const* str;
        int j;
        char* data;

        /*in the final loop, do a huge string*/
        if(i == numStrings)
        {
            i = 1000000;
            //printf("testing huge string of size %d\n", i);
        }

        data = rt_malloc(i+1);

        for(j=0; j<i; ++j)
        {
            data[j] = minChar + rand() % charRange;
        }
        data[i] = 0;

        rbusValue_SetString(val, data);
        TEST(rbusValue_GetType(val)==RBUS_STRING);
        str = rbusValue_GetString(val, &len);
        TEST(i == len);
        TEST(strcmp(data, str)==0);
        free(data);
    }

    rbusValue_Release(val);
}

void testValue_Bytes()
{
    int i;
    uint8_t const* byteTest;
    int byteTestLen;
    /*test rbusValue_SetBytes works correctly for various buffer sizes, especially around the 64 byte buffer block size*/
    int byteLength[18] = {0, 1, 62, 63, 64, 65, 127, 128, 129, 1023, 1024, 1025, 1026, 10000, 65534, 65535, 65536, 65537};
    rbusValue_t val;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&val);

    rbusValue_SetBytes(val, NULL, 0);
    byteTest = rbusValue_GetBytes(val, &byteTestLen);
    TEST(rbusValue_GetType(val) == RBUS_BYTES);
    TEST(byteTestLen == 0);
    TEST(byteTest == NULL);

    rbusValue_SetBytes(val, (uint8_t*)"", 0);
    byteTest = rbusValue_GetBytes(val, &byteTestLen);
    TEST(byteTestLen == 0);

    for(i=0; i < 18; ++i)
    {
        int j;
        int len = byteLength[i];
        int len2;
        uint8_t* bytes = rt_malloc(len);
        uint8_t const* bytes2;
        rbusValue_t bval;

        //printf("test bytes of length %d\n", len);

        rbusValue_Init(&bval);

        for(j=0; j<len; ++j)
        {
            bytes[j] = rand() % 8;
        }
        
        rbusValue_SetBytes(bval, bytes, len);
        TEST(rbusValue_GetType(bval)==RBUS_BYTES);
        bytes2 = rbusValue_GetBytes(bval, &len2);
        TEST(len == len2);
        if(len > 0)
        {
            TEST(memcmp(bytes, bytes2, len) == 0);
        }

        /*test repeated call to rbusValue_SetBytes replaces value correctly */
        len = 100;
        uint8_t overwrite[100];
        for(j=0; j<len; ++j)
        {
            overwrite[j] = rand() % 256;
        }
        rbusValue_SetBytes(bval, overwrite, len);
        bytes2 = rbusValue_GetBytes(bval, &len2);
        TEST(len == len2);
        TEST(memcmp(overwrite, bytes2, len) == 0);

        rbusValue_Release(bval);
        free(bytes);
    }

    rbusValue_Release(val);
}

#define TEST_VALUE_COERCE(F1,V1,F2,V2,RES)\
{\
    rbusValue_t v1;\
    rbusValue_t v2;\
    rbusValue_Init(&v1);\
    rbusValue_Init(&v2);\
    rbusValue_Set##F1(v1, V1);\
    rbusValue_Set##F2(v2, V2);\
    TEST(rbusValue_Compare(v1, v2)==RES);\
    rbusValue_Release(v1);\
    rbusValue_Release(v2);\
}


void testValue_Compare()
{
    rbusValue_t v1;
    rbusValue_t v2;
    uint8_t bytes[1000];
    int i;
    time_t tnow;
    struct tm* tlocal;
    rbusDateTime_t rbus_time = {{0},{0}};

    printf("%s\n",__FUNCTION__);

    time(&tnow);
    tlocal = localtime(&tnow);
    rbusValue_MarshallTMtoRBUS(&rbus_time, tlocal);
    rbus_time.m_tz.m_tzhour = 5;
    rbus_time.m_tz.m_tzmin = 0;
    rbus_time.m_tz.m_isWest = true;

    for(i=0; i < 1000; ++i)
        bytes[i] = rand() % 256;

    rbusValue_Init(&v1);
    rbusValue_Init(&v2);

    TEST(rbusValue_Compare(v1, v2)==0);

    rbusValue_SetString(v1, "s1");
    TEST(rbusValue_Compare(v1, v2)!=0);

    rbusValue_SetString(v2, "s1");
    TEST(rbusValue_Compare(v1, v2)==0);

    rbusValue_SetInt32(v1, 1000);
    TEST(rbusValue_Compare(v1, v2)!=0);

    rbusValue_SetInt32(v2, 1000);
    TEST(rbusValue_Compare(v1, v2)==0);

    rbusValue_SetTime(v1, &rbus_time);
    TEST(rbusValue_Compare(v1, v2)!=0);

    rbusValue_SetTime(v2, &rbus_time);
    TEST(rbusValue_Compare(v1, v2)==0);

    rbusValue_SetBytes(v1, bytes, 1000);
    TEST(rbusValue_Compare(v1, v2)!=0);

    rbusValue_SetBytes(v2, bytes, 1000);
    TEST(rbusValue_Compare(v1, v2)==0);

    bytes[900] = bytes[900]+1;
    rbusValue_SetBytes(v2, bytes, 1000);
    TEST(rbusValue_Compare(v1, v2)!=0);


    TEST_VALUE_COERCE(Int32,0,UInt32,0,0);
    TEST_VALUE_COERCE(UInt32,0,Int32,0,0);
    TEST_VALUE_COERCE(Int32,-1,UInt32,0,-1);
    TEST_VALUE_COERCE(UInt32,0,Int32,-1,1);

    TEST_VALUE_COERCE(Int32,1,UInt32,2,-1);
    TEST_VALUE_COERCE(Int32,1,UInt32,1,0);
    TEST_VALUE_COERCE(Int32,1,UInt32,0,1);
    TEST_VALUE_COERCE(UInt32,100,Int32,101,-1);
    TEST_VALUE_COERCE(UInt32,100,Int32,100,0);
    TEST_VALUE_COERCE(UInt32,100,Int32,99,1);

    TEST_VALUE_COERCE(Boolean,true,Int32,-1,1);
    TEST_VALUE_COERCE(Boolean,true,Int32,0,1);
    TEST_VALUE_COERCE(Boolean,true,Int32,1,0);
    TEST_VALUE_COERCE(Boolean,true,Int32,2,-1);
    TEST_VALUE_COERCE(Boolean,false,Int32,-1,1);
    TEST_VALUE_COERCE(Boolean,false,Int32,0,0);
    TEST_VALUE_COERCE(Boolean,false,Int32,1,-1);
    TEST_VALUE_COERCE(Boolean,false,Int32,2,-1);
    TEST_VALUE_COERCE(Int32,-1,Boolean,true,-1);
    TEST_VALUE_COERCE(Int32,0,Boolean,true,-1);
    TEST_VALUE_COERCE(Int32,1,Boolean,true,0);
    TEST_VALUE_COERCE(Int32,2,Boolean,true,1);
    TEST_VALUE_COERCE(Int32,-1,Boolean,false,-1);
    TEST_VALUE_COERCE(Int32,0,Boolean,false,0);
    TEST_VALUE_COERCE(Int32,1,Boolean,false,1);
    TEST_VALUE_COERCE(Int32,2,Boolean,false,1);
    TEST_VALUE_COERCE(Int32,1,Single,0.999f,1);
    TEST_VALUE_COERCE(Int32,1,Single,1.0f,0);
    TEST_VALUE_COERCE(Int32,1,Single,1.001f,-1);
    TEST_VALUE_COERCE(Double,1000.0001,UInt32,999,1);
    TEST_VALUE_COERCE(Double,1000.0001,UInt32,1000,1);
    TEST_VALUE_COERCE(Double,1000.0001,UInt32,1001,-1);
    TEST_VALUE_COERCE(Double,1000.0001,UInt32,1000,1);
    TEST_VALUE_COERCE(Double,1000.0001,UInt32,1001,-1);

    rbusValue_Release(v1);
    rbusValue_Release(v2);
}
#if 0
void testValue_Copy()
{
    rbusValue_t src;
    rbusValue_t dest;

    rbusValue_Init(&src);
    rbusValue_Init(&dest);

    rbusValue_SetString(src, "s1");
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest) == RBUS_STRING);
    TEST(strcmp(rbusValue_GetString(dest,NULL), "s1")==0);
    TEST(rbusValue_Compare(dest, src)==0);

    rbusValue_SetInt32(src, -1234567);
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest)  == RBUS_INT32);
    TEST(rbusValue_GetInt32(dest)  == -1234567);
    TEST(rbusValue_Compare(dest, src)==0);

    rbusValue_SetBoolean(src, false);
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest) == RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(dest) == false);
    TEST(rbusValue_Compare(dest, src)==0);

    rbusValue_Release(src);
    rbusValue_Release(dest);
}
#endif

void testValue_Copy()
{
    rbusValue_t src;
    rbusValue_t dest;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&src);
    rbusValue_Init(&dest);

    rbusValue_SetString(src, "s1");
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest) == RBUS_STRING);
    TEST(strcmp(rbusValue_GetString(dest,NULL), "s1")==0);
    TEST(rbusValue_GetType(src) == RBUS_STRING);

    rbusValue_SetInt32(src, -1234567);
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest) == RBUS_INT32);
    TEST(rbusValue_GetInt32(dest) == -1234567);
    TEST(rbusValue_GetType(src) == RBUS_INT32);

    rbusValue_SetBoolean(src, false);
    rbusValue_Copy(dest, src);
    TEST(rbusValue_GetType(dest) == RBUS_BOOLEAN);
    TEST(rbusValue_GetBoolean(dest) == false);

    rbusValue_Release(src);
    rbusValue_Release(dest);
}

int compareDateTime(const rbusDateTime_t* l, const rbusDateTime_t* r)
{
#if 0
    printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", 
        l->m_time.tm_sec, 
        l->m_time.tm_min, 
        l->m_time.tm_hour, 
        l->m_time.tm_mday, 
        l->m_time.tm_mon, 
        l->m_time.tm_year, 
        l->m_time.tm_wday, 
        l->m_time.tm_yday, 
        l->m_time.tm_isdst,
        l->m_tz.m_tzhour,
        l->m_tz.m_tzmin,
        l->m_tz.m_isWest);

    printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", 
        r->m_time.tm_sec, 
        r->m_time.tm_min, 
        r->m_time.tm_hour, 
        r->m_time.tm_mday, 
        r->m_time.tm_mon, 
        r->m_time.tm_year, 
        r->m_time.tm_wday, 
        r->m_time.tm_yday, 
        r->m_time.tm_isdst,
        r->m_tz.m_tzhour,
        r->m_tz.m_tzmin,
        r->m_tz.m_isWest);
#endif
    if( l->m_time.tm_sec == r->m_time.tm_sec || 
        l->m_time.tm_min == r->m_time.tm_min ||
        l->m_time.tm_hour == r->m_time.tm_hour ||
        l->m_time.tm_mday == r->m_time.tm_mday ||
        l->m_time.tm_mon == r->m_time.tm_mon ||
        l->m_time.tm_year == r->m_time.tm_year ||
        l->m_time.tm_wday == r->m_time.tm_wday ||
        l->m_time.tm_yday == r->m_time.tm_yday ||
        l->m_time.tm_isdst == r->m_time.tm_isdst ||
        l->m_tz.m_tzhour == r->m_tz.m_tzhour ||
        l->m_tz.m_tzmin == r->m_tz.m_tzmin ||
        l->m_tz.m_isWest == r->m_tz.m_isWest)
        return 0;
    else
        return -1;
}

void testValue_Buffer()
{
    int i;
    int len;
    uint8_t bytes[10000];
    rbusDateTime_t rbus_time = {{0},{0}};

    printf("%s\n",__FUNCTION__);

    for(i=0; i < 10000; ++i)
        bytes[i] = rand() % 256;

    for(i=RBUS_BOOLEAN; i<RBUS_NONE; ++i)
    {
        rbusValue_t valIn;
        rbusValue_t valOut;
        rbusBuffer_t buff;
        bool set = true;

        rbusValue_Init(&valIn);

        switch(i)
        {
            case RBUS_BOOLEAN: rbusValue_SetBoolean(valIn, true); break;
            case RBUS_CHAR: rbusValue_SetChar(valIn, 'a'); break;
            case RBUS_BYTE: rbusValue_SetByte(valIn, 0xa); break;
            //case RBUS_INT8: rbusValue_SetInt8(valIn, -127); break;
            //case RBUS_UINT8: rbusValue_SetUInt8(valIn, 255); break;
            case RBUS_INT16: rbusValue_SetInt16(valIn, 1232); break;
            case RBUS_UINT16: rbusValue_SetUInt16(valIn, 54325); break;
            case RBUS_INT32: rbusValue_SetInt32(valIn, -2009256); break;
            case RBUS_UINT32: rbusValue_SetUInt32(valIn, 112231); break;
            case RBUS_INT64: rbusValue_SetInt64(valIn, 500341898123); break;
            case RBUS_UINT64: rbusValue_SetUInt64(valIn, 12231412313); break;
            case RBUS_SINGLE: rbusValue_SetSingle(valIn, 3.141592653589793f); break;
            case RBUS_DOUBLE: rbusValue_SetDouble(valIn, 3.141592653589793); break;
            case RBUS_DATETIME:{
                time_t tnow;
                time(&tnow);
                struct tm* tlocal = localtime(&tnow);
                //rbus_time.m_time = *tlocal;
                rbusValue_MarshallTMtoRBUS(&rbus_time, tlocal);
                rbus_time.m_tz.m_tzhour = 5;
                rbus_time.m_tz.m_tzmin = 0;
                rbus_time.m_tz.m_isWest = true;
                rbusValue_SetTime(valIn, &rbus_time);
                break;
            }
            case RBUS_STRING: rbusValue_SetString(valIn, "This is a string"); break;
            case RBUS_BYTES: rbusValue_SetBytes(valIn, bytes, 10000); break;            
            default: set = false; break;
        }

        if(!set)    
        {
            rbusValue_Release(valIn);
            continue;
        }

        rbusBuffer_Create(&buff);
        rbusValue_Encode(valIn, buff);
        rbusValue_Decode(&valOut, buff);


        TEST(rbusValue_GetType(valIn) == rbusValue_GetType(valOut));

        switch(i)
        {
            case RBUS_BOOLEAN: TEST(rbusValue_GetBoolean(valOut)==true); break;
            case RBUS_CHAR: TEST(rbusValue_GetChar(valOut)=='a'); break;
            case RBUS_BYTE: TEST(rbusValue_GetByte(valOut)==0xa); break;
            //case RBUS_INT8: TEST(rbusValue_GetInt8(valOut)==-127); break;
            //case RBUS_UINT8: TEST(rbusValue_GetUInt8(valOut)==255); break;
            case RBUS_INT16: TEST(rbusValue_GetInt16(valOut)==1232); break;
            case RBUS_UINT16: TEST(rbusValue_GetUInt16(valOut)==54325); break;
            case RBUS_INT32: TEST(rbusValue_GetInt32(valOut)==-2009256); break;
            case RBUS_UINT32: TEST(rbusValue_GetUInt32(valOut)==112231); break;
            case RBUS_INT64: TEST(rbusValue_GetInt64(valOut)==500341898123); break;
            case RBUS_UINT64: TEST(rbusValue_GetUInt64(valOut)==12231412313); break;
            case RBUS_SINGLE: TEST(rbusValue_GetSingle(valOut)==rbusValue_GetSingle(valIn) && fabs(rbusValue_GetSingle(valOut))-3.14<0.01); break;
            case RBUS_DOUBLE: TEST(rbusValue_GetDouble(valOut)==rbusValue_GetDouble(valIn) && fabs(rbusValue_GetDouble(valOut))-3.14<0.01); break;
            case RBUS_DATETIME:TEST(compareDateTime(rbusValue_GetTime(valOut), &rbus_time)==0); break;
            case RBUS_STRING: TEST(strcmp(rbusValue_GetString(valOut, &len), "This is a string")==0 && len==strlen("This is a string")); break;
            case RBUS_BYTES: TEST(memcmp(rbusValue_GetBytes(valOut, &len), bytes, 10000)==0 && len==10000); break;            
            default: break;
        }

        rbusBuffer_Destroy(buff);
        rbusValue_Release(valOut);
        rbusValue_Release(valIn);
    }
}

void testValue_TLV()
{
    int i;
    int len;
    time_t nowtime = 0;
    uint8_t bytes[10000];
    rbusDateTime_t rbus_time = {{0},{0}};

    printf("%s\n",__FUNCTION__);

    for(i=0; i < 10000; ++i)
        bytes[i] = rand() % 256;

    for(i=RBUS_BOOLEAN; i<RBUS_NONE; ++i)
    {
        rbusValue_t valIn;
        rbusValue_t valOut;
        bool set = true;


        rbusValue_Init(&valIn);

        switch(i)
        {
            case RBUS_BOOLEAN: rbusValue_SetBoolean(valIn, true); break;
            case RBUS_CHAR: rbusValue_SetChar(valIn, 'a'); break;
            case RBUS_BYTE: rbusValue_SetByte(valIn, 0xa); break;
            //case RBUS_INT8: rbusValue_SetInt8(valIn, -127); break;
            //case RBUS_UINT8: rbusValue_SetUInt8(valIn, 255); break;
            case RBUS_INT16: rbusValue_SetInt16(valIn, 1232); break;
            case RBUS_UINT16: rbusValue_SetUInt16(valIn, 54325); break;
            case RBUS_INT32: rbusValue_SetInt32(valIn, -2009256); break;
            case RBUS_UINT32: rbusValue_SetUInt32(valIn, 112231); break;
            case RBUS_INT64: rbusValue_SetInt64(valIn, 500341898123); break;
            case RBUS_UINT64: rbusValue_SetUInt64(valIn, 12231412313); break;
            case RBUS_SINGLE: rbusValue_SetSingle(valIn, 3.141592653589793f); break;
            case RBUS_DOUBLE: rbusValue_SetDouble(valIn, 3.141592653589793); break;
            case RBUS_DATETIME:
                              rbusValue_MarshallTMtoRBUS(&rbus_time, localtime(&nowtime));
                              rbusValue_SetTime(valIn, &rbus_time);
                break;
            case RBUS_STRING: rbusValue_SetString(valIn, "This is a string"); break;
            case RBUS_BYTES: rbusValue_SetBytes(valIn, bytes, 10000); break;            
            default: set = false; break;
        }

        if(!set)    
        {
            rbusValue_Release(valIn);
            continue;
        }

        rbusValue_Init(&valOut);
        rbusValue_SetTLV(valOut, rbusValue_GetType(valIn),rbusValue_GetL(valIn),rbusValue_GetV(valIn));

        switch(i)
        {
            case RBUS_BOOLEAN: TEST(rbusValue_GetBoolean(valOut)==true); break;
            case RBUS_CHAR: TEST(rbusValue_GetChar(valOut)=='a'); break;
            case RBUS_BYTE: TEST(rbusValue_GetByte(valOut)==0xa); break;
            //case RBUS_INT8: TEST(rbusValue_GetInt8(valOut)==-127); break;
            //case RBUS_UINT8: TEST(rbusValue_GetUInt8(valOut)==255); break;
            case RBUS_INT16: TEST(rbusValue_GetInt16(valOut)==1232); break;
            case RBUS_UINT16: TEST(rbusValue_GetUInt16(valOut)==54325); break;
            case RBUS_INT32: TEST(rbusValue_GetInt32(valOut)==-2009256); break;
            case RBUS_UINT32: TEST(rbusValue_GetUInt32(valOut)==112231); break;
            case RBUS_INT64: TEST(rbusValue_GetInt64(valOut)==500341898123); break;
            case RBUS_UINT64: TEST(rbusValue_GetUInt64(valOut)==12231412313); break;
            case RBUS_SINGLE: TEST(rbusValue_GetSingle(valOut)==rbusValue_GetSingle(valIn) && fabs(rbusValue_GetSingle(valOut))-3.14<0.01); break;
            case RBUS_DOUBLE: TEST(rbusValue_GetDouble(valOut)==rbusValue_GetDouble(valIn) && fabs(rbusValue_GetDouble(valOut))-3.14<0.01); break;
            case RBUS_DATETIME: TEST(memcmp(rbusValue_GetTime(valOut), &rbus_time, sizeof(rbus_time))==0); break;
            case RBUS_STRING: TEST(strcmp(rbusValue_GetString(valOut, &len), "This is a string")==0 && len==strlen("This is a string")); break;
            case RBUS_BYTES: TEST(memcmp(rbusValue_GetBytes(valOut, &len), bytes, 10000)==0 && len==10000); break;            
            default: break;
        }

        rbusValue_Release(valOut);
        rbusValue_Release(valIn);
    }
}

void testValue_ToString()
{
    rbusValue_t v;
    char* s;
    char buff[1000];

    time_t nowtime = 0;
    rbusDateTime_t rbus_time = {{0},{0}};
    uint8_t bytes[256] = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed condimentum nibh vel justo mattis, ac blandit ex egestas. "
        "Integer eu ex vel mi ullamcorper rhoncus. Integer auctor venenatis "
        "neque, a sodales velit auctor id. Duis massa justo, consequat acet.";
    char bytesHex[513];
    int i;

    printf("%s\n",__FUNCTION__);

    bytes[255]=0;
    for(i = 0; i < 256; ++i)
    {
        sprintf(&bytesHex[i*2], "%02X", bytes[i]);    
    }
    bytesHex[512]=0;

    rbusValue_Init(&v);
        
    rbusValue_SetBoolean(v, true);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "true"));
    free(s);

    rbusValue_SetChar(v, 'a');
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "a"));
    free(s);

    rbusValue_SetByte(v, 0xc);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "c"));
    free(s);

    rbusValue_SetInt16(v, -16);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "-16"));
    free(s);

    rbusValue_SetUInt16(v, 16);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "16"));
    free(s);

    rbusValue_SetInt32(v, -32);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "-32"));
    free(s);

    rbusValue_SetUInt32(v, 32);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "32"));
    free(s);

    rbusValue_SetInt64(v, -64);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "-64"));
    free(s);

    rbusValue_SetUInt64(v, 64);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "64"));
    free(s);

    rbusValue_SetSingle(v, 3.141592653589793);
    s = rbusValue_ToString(v,0,0);
    TEST(!strncmp(s, "3.14159", 6));
    free(s);

    rbusValue_SetDouble(v, 3.141592653589793);
    s = rbusValue_ToString(v,0,0);
    TEST(!strncmp(s, "3.14159265358979", 15));
    free(s);

    (void)rbus_time;
    (void)nowtime;
    #if 0 //TODO FIXME
    memcpy(&(rbus_time.m_time), localtime(&nowtime),sizeof(struct tm));
    snprintf(buff, 1000, "%s", asctime(&rbus_time.m_time));
    if(buff[strlen(buff)-1]=='\n')/*asctime adds a '\n', so remove it*/
        buff[strlen(buff)-1] = 0;
    rbusValue_SetTime(v, &rbus_time);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(buff, s));
    printf("buff=[%s] s=[%s]\n", buff, s);
    free(s);
    #endif

    rbusValue_SetString(v, "This Is A String");
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, "This Is A String"));
    free(s);

    rbusValue_SetBytes(v, bytes, 256);
    s = rbusValue_ToString(v,0,0);
    TEST(!strcmp(s, bytesHex));
    free(s);

    /*test passing in a buff with sufficient length*/
    rbusValue_SetString(v, "0123456789");
    rbusValue_ToString(v,buff,1000);
    TEST(!strcmp(buff, "0123456789"));
    memset(buff, 0, 11);

    /*test passing in a buff without sufficient length*/
    rbusValue_SetString(v, "0123456789");
    rbusValue_ToString(v,buff,6);/*get 5 chars plus null terminator*/
    TEST(!strcmp(buff, "012345"));

    rbusValue_Release(v);
}

void testValue_ToDebugString()
{
    rbusValue_t v;
    char* s;
    char buff[1000];

    time_t nowtime = 0;
    rbusDateTime_t rbus_time = {{0},{0}};
    uint8_t bytes[256] = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed condimentum nibh vel justo mattis, ac blandit ex egestas. "
        "Integer eu ex vel mi ullamcorper rhoncus. Integer auctor venenatis "
        "neque, a sodales velit auctor id. Duis massa justo, consequat acet.";
    char bytesHex[513];
    int i;

    printf("%s\n",__FUNCTION__);

    rbusValue_Init(&v);
        
    bytes[255]=0;
    for(i = 0; i < 256; ++i)
    {
        sprintf(&bytesHex[i*2], "%02X", bytes[i]);    
    }
    bytesHex[512]=0;

    rbusValue_SetBoolean(v, true);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_BOOLEAN value:true"));
    free(s);

    rbusValue_SetChar(v, 'a');
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_CHAR value:a"));
    free(s);

    rbusValue_SetByte(v, 0xc);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_BYTE value:c"));
    free(s);

    rbusValue_SetInt16(v, -16);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_INT16 value:-16"));
    free(s);

    rbusValue_SetUInt16(v, 16);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_UINT16 value:16"));
    free(s);

    rbusValue_SetInt32(v, -32);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_INT32 value:-32"));
    free(s);

    rbusValue_SetUInt32(v, 32);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_UINT32 value:32"));
    free(s);

    rbusValue_SetInt64(v, -64);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_INT64 value:-64"));
    free(s);

    rbusValue_SetUInt64(v, 64);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_UINT64 value:64"));
    free(s);

    rbusValue_SetSingle(v, 3.141592653589793);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strncmp(s, "rbusValue type:RBUS_SINGLE value:3.14159", 
              strlen("rbusValue type:RBUS_SINGLE value:3.14159")));
    free(s);

    rbusValue_SetDouble(v, 3.141592653589793);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strncmp(s, "rbusValue type:RBUS_DOUBLE value:3.14159265358979", 
              strlen("rbusValue type:RBUS_DOUBLE value:3.14159265358979")));
    free(s);

    (void)rbus_time;
    (void)nowtime;
    #if 0 //TODO FIXME
    memcpy(&(rbus_time.m_time), localtime(&nowtime),sizeof(struct tm));
    snprintf(buff, 1000, "rbusValue type:RBUS_DATETIME value:%s", asctime(&rbus_time.m_time));
    if(buff[strlen(buff)-1]=='\n')/*asctime adds a '\n', so remove it*/
        buff[strlen(buff)-1] = 0;
    rbusValue_SetTime(v, &rbus_time);
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(buff, s));
    free(s);
    #endif

    rbusValue_SetString(v, "This Is A String");
    s = rbusValue_ToDebugString(v,0,0);
    TEST(!strcmp(s, "rbusValue type:RBUS_STRING value:This Is A String"));
    free(s);

    rbusValue_SetBytes(v, bytes, 256);
    s = rbusValue_ToDebugString(v,0,0);
    sprintf(buff, "rbusValue type:RBUS_BYTES value:%s", (char*)bytesHex);
    TEST(!strcmp(s, buff));
    free(s);

    /*test passing in a buff with sufficient length*/
    rbusValue_SetString(v, "0123456789");
    rbusValue_ToDebugString(v,buff,1000);
    TEST(!strcmp(buff, "rbusValue type:RBUS_STRING value:0123456789"));
    memset(buff, 0, 100);

    /*test passing in a buff without sufficient length*/
    rbusValue_SetString(v, "0123456789");
    rbusValue_ToDebugString(v,buff,
                strlen("rbusValue type:RBUS_STRING value:012345")+1);/*get 5 chars from value plus null terminator*/
    TEST(!strcmp(buff, "rbusValue type:RBUS_STRING value:012345"));

    rbusValue_Release(v);
}

void testValue_Print()
{
    int i;
    printf("%s\n",__FUNCTION__);

    for(i=RBUS_BOOLEAN; i<RBUS_NONE; ++i)
    {
        rbusValue_t v;
        rbusValue_Init(&v);
        time_t nowtime = 0;
        rbusDateTime_t rbus_time = {{0},{0}};
        uint8_t bytes[256] = 
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Sed condimentum nibh vel justo mattis, ac blandit ex egestas. "
            "Integer eu ex vel mi ullamcorper rhoncus. Integer auctor venenatis "
            "neque, a sodales velit auctor id. Duis massa justo, consequat acet.";
        bytes[255]=0;

        switch(i)
        {
        case RBUS_BOOLEAN:
            rbusValue_SetBoolean(v, true);
            break;
        case RBUS_CHAR:
            rbusValue_SetChar(v, 'a');
            break;
        case RBUS_BYTE:
            rbusValue_SetByte(v, 0xa);
            break;
        /*
        case RBUS_INT8:
            rbusValue_SetInt8(v, -127);
            break;
        case RBUS_UINT8:
            rbusValue_SetUInt8(v, 255);
            break;
        */
        case RBUS_INT16:
            rbusValue_SetInt16(v, -16);
            break;
        case RBUS_UINT16:
            rbusValue_SetUInt16(v, -16);
            break;
        case RBUS_INT32:
            rbusValue_SetInt32(v, -32);
            break;
        case RBUS_UINT32:
            rbusValue_SetUInt32(v, -32);
            break;
        case RBUS_INT64:
            rbusValue_SetInt64(v, -64);
            break;
        case RBUS_UINT64:
            rbusValue_SetUInt64(v, -64);
            break;
        case RBUS_SINGLE:
            rbusValue_SetSingle(v, 3.141592653589793);
            break;
        case RBUS_DOUBLE:
            rbusValue_SetDouble(v, 3.141592653589793);
            break;
        case RBUS_DATETIME:
            //memcpy(&(rbus_time.m_time), localtime(&nowtime),sizeof(struct tm));
            rbusValue_MarshallTMtoRBUS(&rbus_time, localtime(&nowtime));
            rbusValue_SetTime(v, &rbus_time);
            break;
        case RBUS_STRING:
            rbusValue_SetString(v, "The Is A String");
            break;
        case RBUS_BYTES:
            rbusValue_SetBytes(v, bytes, 256);
            break;
        default:
            break;
        }

        if(rbusValue_GetType(v) != RBUS_NONE)
        {
            char* sDebug = rbusValue_ToDebugString(v, NULL, 0);
            char* sValue = rbusValue_ToString(v, NULL, 0);
            printf("ToDebugString=[%s] ToString=[%s]\n", sDebug, sValue);
            free(sValue);
            free(sDebug);
        }
    
        rbusValue_Release(v);
    }
}

void testValue_InitType()
{
    rbusDateTime_t tv1 = {{0},{0}};
    time_t nowtime = 0;
    const char teststring[] = "Hello World";
    rbusProperty_t prop;
    rbusObject_t obj;

    printf("%s\n",__FUNCTION__);

    rbusValue_MarshallTMtoRBUS(&tv1, localtime(&nowtime));
    rbusProperty_Init(&prop, "MyProp", NULL);
    rbusObject_Init(&obj, "MyObj");

    rbusValue_t vbtrue = rbusValue_InitBoolean(true);
    rbusValue_t vbfalse = rbusValue_InitBoolean(false);
    rbusValue_t vi16_n1234 = rbusValue_InitInt16(-1234);
    rbusValue_t vu16_4321 = rbusValue_InitUInt16(4321);
    rbusValue_t vi32_689013 = rbusValue_InitInt32(689013);
    rbusValue_t vu32_856712 = rbusValue_InitUInt32(856712);
    rbusValue_t vi64_987654321213 = rbusValue_InitInt64(987654321213);
    rbusValue_t vu64_987654321213 = rbusValue_InitUInt64(987654321213);
    rbusValue_t vf32_354dot678 = rbusValue_InitSingle(354.678f);
    rbusValue_t vf64_789dot4738291023 = rbusValue_InitDouble(789.4738291023);
    rbusValue_t vtv = rbusValue_InitTime(&tv1);
    rbusValue_t vs = rbusValue_InitString(teststring);
    rbusValue_t vbytes = rbusValue_InitBytes((uint8_t const*)teststring, strlen(teststring)+1);
    rbusValue_t vprop = rbusValue_InitProperty(prop);
    rbusValue_t vobj = rbusValue_InitObject(obj);    

    TEST(rbusValue_GetBoolean(vbtrue) == true);
    TEST(rbusValue_GetBoolean(vbfalse) == false);
    TEST(rbusValue_GetInt16(vi16_n1234) == -1234);
    TEST(rbusValue_GetUInt16(vu16_4321) == 4321);
    TEST(rbusValue_GetInt32(vi32_689013) == 689013);
    TEST(rbusValue_GetUInt32(vu32_856712) == 856712);
    TEST(rbusValue_GetInt64(vi64_987654321213) == 987654321213);
    TEST(rbusValue_GetUInt64(vu64_987654321213) == 987654321213);
    TEST(rbusValue_GetSingle(vf32_354dot678) == 354.678f);
    TEST(rbusValue_GetDouble(vf64_789dot4738291023) == 789.4738291023);
    TEST(memcmp(rbusValue_GetTime(vtv), &tv1, sizeof(rbusDateTime_t)) == 0);
    TEST(rbusValue_GetString(vs, NULL) && !strcmp(rbusValue_GetString(vs, NULL), "Hello World"));
    TEST(rbusValue_GetBytes(vbytes, NULL) && !strcmp((char const*)rbusValue_GetBytes(vbytes, NULL), "Hello World"));
    TEST(rbusValue_GetProperty(vprop) != NULL && rbusProperty_GetName(rbusValue_GetProperty(vprop)) && !strcmp(rbusProperty_GetName(rbusValue_GetProperty(vprop)), "MyProp"));
    TEST(rbusValue_GetObject(vobj) != NULL && rbusObject_GetName(rbusValue_GetObject(vobj)) && !strcmp(rbusObject_GetName(rbusValue_GetObject(vobj)), "MyObj"));

    rbusValue_Release(vbtrue);
    rbusValue_Release(vbfalse);
    rbusValue_Release(vi16_n1234);
    rbusValue_Release(vu16_4321);
    rbusValue_Release(vi32_689013);
    rbusValue_Release(vu32_856712);
    rbusValue_Release(vi64_987654321213);
    rbusValue_Release(vu64_987654321213);
    rbusValue_Release(vf32_354dot678);
    rbusValue_Release(vf64_789dot4738291023);
    rbusValue_Release(vtv);
    rbusValue_Release(vs);
    rbusValue_Release(vbytes);
    rbusValue_Release(vprop);
    rbusValue_Release(vobj);
    rbusObject_Release(obj);
    rbusProperty_Release(prop);
}

void testValueAPI(rbusHandle_t handle, int* countPass, int* countFail)
{
    (void)handle;

    testValue_Type();
    testValue_Bool();
    testValue_Char();
    testValue_Ints();
    testValue_Floats();
    testValue_Time();
    testValue_String();
    testValue_Bytes();
    testValue_Compare();
    testValue_Copy();
    testValue_Buffer();
    testValue_ToString();
    testValue_ToDebugString();
    testValue_TLV();
    testValue_Print();
    testValue_InitType();

    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_ValueAPI");
}
