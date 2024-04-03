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

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <rbus.h>
#include "rbusProviderConsumer.h"
static void exec_func_test(rbusGtest_t test)
{
  int runtime = 0;

  switch(test)
  {
    case RBUS_GTEST_METHOD_ASYNC:
    {
      runtime = 5;
      break;
    }
    case RBUS_GTEST_METHOD_ASYNC1:
    {
      runtime=5;
      break;
    }
    case RBUS_GTEST_FILTER2:
    case RBUS_GTEST_INTERVAL_SUB1:
    case RBUS_GTEST_ASYNC_SUB4:
    {
      runtime = 15;
      break;
    }
    default:
    {
      runtime = 3;
      break;
    }
  }
  pid_t pid = fork();
  if (0 == pid) {
    int ret = 0;
    usleep(50000);
    ret = rbusConsumer(test, 0, runtime);
    exit(ret);
  } else {
    int ret = 0;
    int consumer_status;
    int expected_consumer_status = RBUS_ERROR_SUCCESS;

    /* Run the appropriate provider */
    switch(test)
    {
      case RBUS_GTEST_GET13:
      case RBUS_GTEST_GET14:
      case RBUS_GTEST_GET15:
      case RBUS_GTEST_GET16:
      case RBUS_GTEST_GET17:
      case RBUS_GTEST_GET18:
      case RBUS_GTEST_GET19:
      case RBUS_GTEST_GET20:
      case RBUS_GTEST_GET21:
      case RBUS_GTEST_GET22:
      case RBUS_GTEST_GET23:
      case RBUS_GTEST_GET24:
        ret = rbuscoreProvider(test, pid, &consumer_status);
        break;
      case RBUS_GTEST_ASYNC_SUB5:
        {
          pid_t pid1 = fork();
          if(0 == pid1){
            printf("Provider1 starts.\n");
            ret = rbusProvider1((runtime/3),0);
            exit(ret);
          } else {
            printf("Provider2 starts.\n");
            waitpid(pid1, &consumer_status, 0);
            ret = rbusProvider1((runtime/2),1);
          }
        }
        break;
      default:
        ret = rbusProvider(test, pid, &consumer_status);
        break;
    }
    printf("Consumer result %d\n",WEXITSTATUS(consumer_status));

    if((WIFSIGNALED(consumer_status)) && (WTERMSIG(consumer_status) != SIGUSR1))
    {
      printf("/******************************************/\n");
      printf("/******************************************/\n");
      printf("                                            \n");
      printf("ABNORMAL EXIT by %d: %s\n",WTERMSIG(consumer_status),strsignal(WTERMSIG(consumer_status)));
      printf("                                            \n");
      printf("/******************************************/\n");
      printf("/******************************************/\n");
      expected_consumer_status = RBUS_ERROR_BUS_ERROR;
    } else {

      /* Update the expected_consumer_status */
      switch(test)
      {
        case RBUS_GTEST_GET1:
        case RBUS_GTEST_GET2:
        case RBUS_GTEST_GET3:
        case RBUS_GTEST_SET5:
        case RBUS_GTEST_SET6:
        case RBUS_GTEST_SET7:
        case RBUS_GTEST_SET8:
        case RBUS_GTEST_UNREG_ROW:
        case RBUS_GTEST_DISC_COMP2:
        case RBUS_GTEST_DISC_COMP3:
        case RBUS_GTEST_DISC_COMP4:
        case RBUS_GTEST_SET_MULTI2:
          expected_consumer_status = RBUS_ERROR_INVALID_INPUT;
          break;
        case RBUS_GTEST_SET_MULTI3:
        case RBUS_GTEST_SET3:
          expected_consumer_status = RBUS_ERROR_DESTINATION_NOT_FOUND;
          break;
        case RBUS_GTEST_SET_MULTI5:
        case RBUS_GTEST_SET2:
          expected_consumer_status = RBUS_ERROR_INVALID_OPERATION;
          break;
        case RBUS_GTEST_GET24:
          expected_consumer_status = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
          break;
        case RBUS_GTEST_GET29:
          expected_consumer_status = RBUS_ERROR_ACCESS_NOT_ALLOWED;
          break;
        case RBUS_GTEST_GET28:
          expected_consumer_status = RBUS_ERROR_BUS_ERROR;
          break;
      }
    }

    EXPECT_EQ(WEXITSTATUS(consumer_status),expected_consumer_status);
    EXPECT_EQ(ret,0);
  }
}

TEST(rbusApiGetExt, test1)
{
  exec_func_test(RBUS_GTEST_GET_EXT1);
}

TEST(rbusApiGetExt, test2)
{
  int j = 0;
  pid_t pid_arr[3];
  for(j = 0 ; j < 3 ; j++ )
  {
    pid_arr[j] = fork();

    if (0 == pid_arr[j]) {
      int ret = 0;
      ret = rbusMultiProvider(j);

      exit(ret);
    } else {
      int ret = 0;
      ret = rbusConsumer(RBUS_GTEST_GET_EXT2, pid_arr[j], 0);
    }
  }

  for(j = 0 ; j < 3 ; j++ )
    wait(NULL);
}

TEST(rbusApiGet, test1)
{
  exec_func_test(RBUS_GTEST_GET1);
}

TEST(rbusApiGet, test2)
{
  exec_func_test(RBUS_GTEST_GET2);
}

TEST(rbusApiGet, test3)
{
  exec_func_test(RBUS_GTEST_GET3);
}

TEST(rbusApiGet, test4)
{
  exec_func_test(RBUS_GTEST_GET4);
}

TEST(rbusApiGet, test5)
{
  exec_func_test(RBUS_GTEST_GET5);
}

TEST(rbusApiGet, test6)
{
  exec_func_test(RBUS_GTEST_GET6);
}

TEST(rbusApiGet, test7)
{
  exec_func_test(RBUS_GTEST_GET7);
}

TEST(rbusApiGet, test8)
{
  exec_func_test(RBUS_GTEST_GET8);
}

TEST(rbusApiGet, test9)
{
  exec_func_test(RBUS_GTEST_GET9);
}

TEST(rbusApiGet, test10)
{
  exec_func_test(RBUS_GTEST_GET10);
}

TEST(rbusApiGet, test11)
{
  exec_func_test(RBUS_GTEST_GET11);
}

TEST(rbusApiGet, test12)
{
  exec_func_test(RBUS_GTEST_GET12);
}

TEST(rbusApiGet, test13)
{
  exec_func_test(RBUS_GTEST_GET13);
}

TEST(rbusApiGet, test14)
{
  exec_func_test(RBUS_GTEST_GET14);
}

TEST(rbusApiGet, test15)
{
  exec_func_test(RBUS_GTEST_GET15);
}

TEST(rbusApiGet, test16)
{
  exec_func_test(RBUS_GTEST_GET16);
}

TEST(rbusApiGet, test17)
{
  exec_func_test(RBUS_GTEST_GET17);
}

TEST(rbusApiGet, test18)
{
  exec_func_test(RBUS_GTEST_GET18);
}

TEST(rbusApiGet, test19)
{
  exec_func_test(RBUS_GTEST_GET19);
}

TEST(rbusApiGet, test20)
{
  exec_func_test(RBUS_GTEST_GET20);
}

TEST(rbusApiGet, test21)
{
  exec_func_test(RBUS_GTEST_GET21);
}

TEST(rbusApiGet, test22)
{
  exec_func_test(RBUS_GTEST_GET22);
}

TEST(rbusApiGet, test23)
{
  exec_func_test(RBUS_GTEST_GET23);
}

TEST(rbusApiGet, test24)
{
  exec_func_test(RBUS_GTEST_GET24);
}

TEST(rbusApiGet, test25)
{
  exec_func_test(RBUS_GTEST_GET25);
}

TEST(rbusApiGet, test26)
{
  exec_func_test(RBUS_GTEST_GET26);
}

TEST(rbusApiGet, test27)
{
  exec_func_test(RBUS_GTEST_GET27);
}

TEST(rbusApiGet, test28)
{
  exec_func_test(RBUS_GTEST_GET28);
}

TEST(rbusApiGet, test29)
{
  exec_func_test(RBUS_GTEST_GET29);
}

TEST(rbusApiGet, test30)
{
  exec_func_test(RBUS_GTEST_GET30);
}

TEST(rbusApiGet, test31)
{
  exec_func_test(RBUS_GTEST_GET31);
}

TEST(rbusApiSet, test1)
{
  exec_func_test(RBUS_GTEST_SET1);
}

TEST(rbusApiSet, test2)
{
  exec_func_test(RBUS_GTEST_SET2);
}

TEST(rbusApiSet, test3)
{
  exec_func_test(RBUS_GTEST_SET3);
}

TEST(rbusApiSet, test4)
{
  exec_func_test(RBUS_GTEST_SET4);
}

TEST(rbusApiSet, test5)
{
  exec_func_test(RBUS_GTEST_SET5);
}

TEST(rbusApiSet, test6)
{
  exec_func_test(RBUS_GTEST_SET6);
}

TEST(rbusApiSet, test7)
{
  exec_func_test(RBUS_GTEST_SET7);
}

TEST(rbusApiSet, test8)
{
  exec_func_test(RBUS_GTEST_SET8);
}

TEST(rbusApiSet, test9)
{
  exec_func_test(RBUS_GTEST_SET9);
}

TEST(rbusApiSet, test10)
{
  exec_func_test(RBUS_GTEST_SET10);
}

TEST(rbusApiSet, test11)
{
  exec_func_test(RBUS_GTEST_SET11);
}

TEST(rbusApiSetMulti, test1)
{
  exec_func_test(RBUS_GTEST_SET_MULTI1);
}

TEST(rbusApiSetMulti, test2)
{
  exec_func_test(RBUS_GTEST_SET_MULTI2);
}

TEST(rbusApiSetMulti, test3)
{
  exec_func_test(RBUS_GTEST_SET_MULTI3);
}

TEST(rbusApiSetMulti, test4)
{
  exec_func_test(RBUS_GTEST_SET_MULTI4);
}

TEST(rbusApiSetMulti, test5)
{
  exec_func_test(RBUS_GTEST_SET_MULTI5);
}

TEST(rbusApiDiscoverComp, test1)
{
  exec_func_test(RBUS_GTEST_DISC_COMP1);
}

TEST(rbusApiDiscoverComp, test2)
{
  exec_func_test(RBUS_GTEST_DISC_COMP2);
}

TEST(rbusApiDiscoverComp, test3)
{
  exec_func_test(RBUS_GTEST_DISC_COMP3);
}

TEST(rbusApiDiscoverComp, test4)
{
  exec_func_test(RBUS_GTEST_DISC_COMP4);
}

TEST(rbusApiMethod, test1)
{
  exec_func_test(RBUS_GTEST_METHOD1);
}

TEST(rbusApiMethod, test2)
{
  exec_func_test(RBUS_GTEST_METHOD2);
}

TEST(rbusApiMethod, test3)
{
  exec_func_test(RBUS_GTEST_METHOD3);
}

TEST(rbusApiMethod, test4)
{
  exec_func_test(RBUS_GTEST_METHOD4);
}

TEST(rbusApiMethodAsync, test)
{
  exec_func_test(RBUS_GTEST_METHOD_ASYNC);
}

TEST(rbusApiMethodAsync_1, test)
{
  exec_func_test(RBUS_GTEST_METHOD_ASYNC1);
}

TEST(rbusApiValueChangeTest, test1)
{
  exec_func_test(RBUS_GTEST_FILTER1);
}

TEST(rbusApiValueChangeTest, test2)
{
  exec_func_test(RBUS_GTEST_FILTER2);
}

TEST(rbusApiIntervalSubTest, test1)
{
  exec_func_test(RBUS_GTEST_INTERVAL_SUB1);
}

TEST(rbusAsyncSubTest, test1)
{
  exec_func_test(RBUS_GTEST_ASYNC_SUB1);
}

TEST(rbusAsyncSubTest, test2)
{
  exec_func_test(RBUS_GTEST_ASYNC_SUB2);
}

TEST(rbusAsyncSubTest, test3)
{
  exec_func_test(RBUS_GTEST_ASYNC_SUB3);
}

TEST(rbusAsyncSubTest, test4)
{
  exec_func_test(RBUS_GTEST_ASYNC_SUB4);
}

TEST(rbusAsyncSubTest, test5)
{
  exec_func_test(RBUS_GTEST_ASYNC_SUB5);
}

TEST(rbusRegRowTest, test)
{
  exec_func_test(RBUS_GTEST_REG_ROW);
}

TEST(rbusUnRegRowTest, test)
{
  exec_func_test(RBUS_GTEST_UNREG_ROW);
}
