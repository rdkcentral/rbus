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

#include <unistd.h>
typedef enum
{
  RBUS_GTEST_FILTER1 = 0,
  RBUS_GTEST_FILTER2,
  RBUS_GTEST_INTERVAL_SUB1,
  RBUS_GTEST_ASYNC_SUB1,
  RBUS_GTEST_ASYNC_SUB2,
  RBUS_GTEST_ASYNC_SUB3,
  RBUS_GTEST_ASYNC_SUB4,
  RBUS_GTEST_ASYNC_SUB5,
  RBUS_GTEST_GET1,
  RBUS_GTEST_GET2,
  RBUS_GTEST_GET3,
  RBUS_GTEST_GET4,
  RBUS_GTEST_GET5,
  RBUS_GTEST_GET6,
  RBUS_GTEST_GET7,
  RBUS_GTEST_GET8,
  RBUS_GTEST_GET9,
  RBUS_GTEST_GET10,
  RBUS_GTEST_GET11,
  RBUS_GTEST_GET12,
  RBUS_GTEST_GET13,
  RBUS_GTEST_GET14,
  RBUS_GTEST_GET15,
  RBUS_GTEST_GET16,
  RBUS_GTEST_GET17,
  RBUS_GTEST_GET18,
  RBUS_GTEST_GET19,
  RBUS_GTEST_GET20,
  RBUS_GTEST_GET21,
  RBUS_GTEST_GET22,
  RBUS_GTEST_GET23,
  RBUS_GTEST_GET24,
  RBUS_GTEST_GET25,
  RBUS_GTEST_GET26,
  RBUS_GTEST_GET27,
  RBUS_GTEST_GET28,
  RBUS_GTEST_GET29,
  RBUS_GTEST_GET30,
  RBUS_GTEST_GET31,
  RBUS_GTEST_GET_EXT1,
  RBUS_GTEST_GET_EXT2,
  RBUS_GTEST_SET1,
  RBUS_GTEST_SET2,
  RBUS_GTEST_SET3,
  RBUS_GTEST_SET4,
  RBUS_GTEST_SET5,
  RBUS_GTEST_SET6,
  RBUS_GTEST_SET7,
  RBUS_GTEST_SET8,
  RBUS_GTEST_SET9,
  RBUS_GTEST_SET10,
  RBUS_GTEST_SET11,
  RBUS_GTEST_SET_MULTI1,
  RBUS_GTEST_SET_MULTI2,
  RBUS_GTEST_SET_MULTI3,
  RBUS_GTEST_SET_MULTI4,
  RBUS_GTEST_SET_MULTI5,
  RBUS_GTEST_SET_MULTI_EXT1,
  RBUS_GTEST_SET_MULTI_EXT2,
  RBUS_GTEST_DISC_COMP1,
  RBUS_GTEST_DISC_COMP2,
  RBUS_GTEST_DISC_COMP3,
  RBUS_GTEST_DISC_COMP4,
  RBUS_GTEST_METHOD1,
  RBUS_GTEST_METHOD2,
  RBUS_GTEST_METHOD3,
  RBUS_GTEST_METHOD4,
  RBUS_GTEST_METHOD_ASYNC,
  RBUS_GTEST_METHOD_ASYNC1,
  RBUS_GTEST_REG_ROW,
  RBUS_GTEST_UNREG_ROW,
} rbusGtest_t;

int rbusConsumer(rbusGtest_t test, pid_t pid, int runtime);
int rbusProvider(rbusGtest_t test, pid_t pid, int *consumer_status);
int rbusProvider1(int runtime,int should_exit);
int rbusMultiProvider(int index);
int rbuscoreProvider(rbusGtest_t test, pid_t pid, int *consumer_status);

#define GTEST_VAL_BOOL   1
#define GTEST_VAL_INT16  (int16_t)0xABCD
#define GTEST_VAL_INT64  (int64_t)0xABCD
#define GTEST_VAL_INT32  (int32_t)0xABCD
#define GTEST_VAL_UINT16 (uint16_t)0xABCD
#define GTEST_VAL_UINT64 (uint64_t)0xABCD
#define GTEST_VAL_UINT32 (uint32_t)0xABCD
#define GTEST_VAL_SINGLE (float)3.141592653589793f
#define GTEST_VAL_DOUBLE (double)3.141592653589793
#define GTEST_VAL_STRING "legacy_test"
