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

#include <rbus.h>
#include "rbus_buffer.h"
#include "../common/test_macros.h"
#include <string.h>
#include <stdlib.h>
static int gDuration = 1;

int getDurationFilter()
{
    return gDuration;
}

static void testApply(rbusFilter_t f1)
{
    int i;
    rbusValue_t vIn;
    rbusValue_Init(&vIn);
    for(i = -12; i <= 12; ++i)
    {
        rbusValue_SetInt32(vIn, i);
        TEST(rbusFilter_Apply(f1, vIn) == (int32_t)(( (i < -10 || i > 10) || (i > -5 && i < 5) )));
    }
    rbusValue_Release(vIn);
}

static void testSerialize(rbusFilter_t f1)
{
    rbusFilter_t f2;
    rbusBuffer_t buffOut, buffIn;
    FILE* file;

    rbusBuffer_Create(&buffOut);
    rbusFilter_Encode(f1, buffOut);
    file = fopen("/tmp/filter.dat", "wb");
    if(!file)
    {
        TEST(file != NULL);
        return;
    }
    fwrite(buffOut->data, 1, buffOut->posWrite, file);
    fclose(file);

    rbusBuffer_Create(&buffIn);
    file = fopen("/tmp/filter.dat", "rb");
    if(!file)
    {
        TEST(file != NULL);
        return;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    rbusBuffer_Reserve(buffIn, size);
    int re = fread(buffIn->data, 1, size, file); 
    (void)re;
    fclose(file);
    buffIn->posWrite += size;
    
    printf("buffOut->posWrite=%d buffIn->posWrite=%d\n", buffOut->posWrite, buffIn->posWrite);
    TEST(buffOut->posWrite == buffIn->posWrite);
    TEST(memcmp(buffOut->data, buffIn->data, size) == 0);

    TEST(buffIn->posRead == 0);
    rbusFilter_Decode(&f2, buffIn);

    TEST(rbusFilter_Compare(f1, f2) == 0);

    testApply(f2);

    rbusFilter_Release(f2);
    rbusBuffer_Destroy(buffOut);
    rbusBuffer_Destroy(buffIn);
}

/*
    f1( j1( r1(>10) | r2(<-10) ) | j2( r3(>-5) && r4(<5) ) )
    this filter will trigger if the test value as such
    ( (v>10 || v<-10) | (v>-5 || v<5 ) )
*/
static void test1()
{
    rbusValue_t v1, v2, v3, v4;
    rbusFilter_t r1, r2, r3, r4, j1, j2, f1;

    rbusValue_Init(&v1);
    rbusValue_Init(&v2);
    rbusValue_Init(&v3);
    rbusValue_Init(&v4);

    rbusValue_SetInt32(v1, 10);
    rbusValue_SetInt32(v2, -10);
    rbusValue_SetInt32(v3, -5);
    rbusValue_SetInt32(v4, 5);

    rbusFilter_InitRelation(&r1, RBUS_FILTER_OPERATOR_GREATER_THAN, v1);
    rbusFilter_InitRelation(&r2, RBUS_FILTER_OPERATOR_LESS_THAN, v2);
    rbusFilter_InitLogic(&j1, RBUS_FILTER_OPERATOR_OR, r1, r2);

    rbusFilter_InitRelation(&r4, RBUS_FILTER_OPERATOR_GREATER_THAN, v3);
    rbusFilter_InitRelation(&r3, RBUS_FILTER_OPERATOR_LESS_THAN, v4);
    rbusFilter_InitLogic(&j2, RBUS_FILTER_OPERATOR_AND, r3, r4);

    rbusFilter_InitLogic(&f1, RBUS_FILTER_OPERATOR_OR, j1, j2);

    testApply(f1);

    testSerialize(f1);

    rbusValue_Release(v1);
    rbusValue_Release(v2);
    rbusValue_Release(v3);
    rbusValue_Release(v4);
    rbusFilter_Release(r1);
    rbusFilter_Release(r2);
    rbusFilter_Release(r3);
    rbusFilter_Release(r4);
    rbusFilter_Release(j1);
    rbusFilter_Release(j2);
    rbusFilter_Release(f1);
}

void testFilter(rbusHandle_t handle, int* countPass, int* countFail)
{
    (void)handle;
    test1();
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("test_Filter");
}

