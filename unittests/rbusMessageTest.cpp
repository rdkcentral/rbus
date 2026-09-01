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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

void isElementPresent(rbusHandle_t handle, const char *elementName);
#define hasReceiverStarted(handle, name)  isElementPresent(handle, name)
#define hasSenderStarted(handle, name)    isElementPresent(handle, name)
typedef enum
{
  RBUS_GTEST_MSG1 = 0,
  RBUS_GTEST_MSG2,
  RBUS_GTEST_MSG3,
  RBUS_GTEST_MSG4,
  RBUS_GTEST_MSG5,
  RBUS_GTEST_MSG6,
  RBUS_GTEST_MSG7,
  RBUS_GTEST_MSG8,
  RBUS_GTEST_MSG9
} rbusGtestMsg_t;

typedef struct rbusMsgData
{
  rbusGtestMsg_t test;
  pthread_cond_t *cond;
  char componentName[64];
} rbusMsgData_t;

static void rbusRecvHandler(rbusHandle_t handle, rbusMessage_t* msg, void * pUserData)
{
  (void)handle;
  int count = 10;
  char buf[64] = {0};
  rbusMsgData_t userData;

  snprintf(buf, sizeof(buf),"%.*s",msg->length, (char const *)msg->data);

  memcpy(&userData, (rbusMsgData_t *)pUserData, sizeof(userData));
  printf("%s: %s got the message \"%s\"\n", __func__,(userData.componentName),buf);

  if((RBUS_GTEST_MSG8 == userData.test) && (NULL != (userData.cond)))
    pthread_cond_signal((userData.cond));
  else if(((RBUS_GTEST_MSG5 == userData.test) || (RBUS_GTEST_MSG6 == userData.test) ) && (NULL != (userData.cond)))
  {
    while(count--)
      sleep(1);

    if(strcmp("0: Hello!",buf) == 0)
      pthread_cond_signal((userData.cond));
  }

  printf("%s: return\n",__func__);
}

static int rbus_send(const char *topic, rbusMessageSendOptions_t opt, int expectedRet)
{
  rbusHandle_t handle;
  char buff[256];
  rbusMessage_t msg;
  int ret = RBUS_ERROR_BUS_ERROR;
  char *componentName = NULL;

  componentName = strdup(__func__);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  msg.topic = topic;
  msg.data = (uint8_t*)buff;
  msg.length = snprintf(buff, sizeof(buff), "%ld: Hello!", time(NULL));

  printf("%s send the message \"%.*s\" to the topic \"%s\" \n", componentName, msg.length, (char const *)msg.data, msg.topic);
  ret = rbusMessage_Send(handle, &msg, opt);
  EXPECT_EQ(ret, expectedRet);

  ret = rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  free(componentName);
  printf("%s: exit\n",__func__);
  return ret;
}

static int rbus_send_neg(const char *topic, pid_t pid, int *rbus_recv_status, rbusMessageSendOptions_t opt)
{
  rbusHandle_t handle;
  char buff[256];
  rbusMessage_t msg;
  int ret = RBUS_ERROR_BUS_ERROR;
  char *componentName = NULL;
  int i = 2;

  componentName = strdup(__func__);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  sleep(1);
  while(i--)
  {
    msg.topic = topic;
    msg.data = (uint8_t*)buff;
    msg.length = snprintf(buff, sizeof(buff), "%d: Hello!", i);

    ret = rbusMessage_Send(handle, &msg, opt);
    if(RBUS_MESSAGE_CONFIRM_RECEIPT == opt)
      EXPECT_NE(ret, RBUS_ERROR_SUCCESS);
    else
      EXPECT_EQ(ret, RBUS_ERROR_SUCCESS);
    printf("rbusMessage_Send topic=%s data=%.*s\n", msg.topic, msg.length, (char const *)msg.data);
    sleep(2);
  }

  ret = rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
  waitpid(pid, rbus_recv_status, 0);

exit:
  free(componentName);
  printf("%s: exit\n",__func__);
  return ret;
}

static int rbus_recv(const char *topic, pid_t pid, int *rbus_send_status, rbusGtestMsg_t test)
{
  rbusHandle_t handle;
  uint32_t listenerId;
  int ret = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  char *componentName = NULL;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  rbusMsgData_t userData;

  componentName = strdup(__func__);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  if(NULL == rbus_send_status)
    userData.cond = &cond;
  else
    userData.cond = NULL;
  userData.test = test;
  strcpy(userData.componentName, componentName);

  ret |= rbusMessage_AddListener(handle, topic, &rbusRecvHandler, (void *)(&userData), 0);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  if(rbus_send_status) {
    wait_ret = waitpid(pid, rbus_send_status, 0);
    EXPECT_EQ(wait_ret,pid);

    if(wait_ret != pid) printf("%s: waitpid() failed %d: %s\n",__func__,errno,strerror(errno));
    ret = (wait_ret != pid) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;

  } else {
    pthread_cond_wait(&cond, &lock);
  }

  ret |= rbusMessage_RemoveListener(handle, topic, 0);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  ret |= rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  free(componentName);
  printf("%s: exit\n",__func__);
  return ret;
}

static int rbus_multi_send(const char *topic, int index)
{
  rbusHandle_t handle;
  char buff[256];
  rbusMessage_t msg;
  int ret = RBUS_ERROR_BUS_ERROR;
  char componentName[32] = {0};
  sigset_t set;
  int sig;

  snprintf(componentName,sizeof(componentName),"%s%d",__func__,index);
  printf("%s: start\n",componentName);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);
  sigwait(&set, &sig);
  printf("%s Got signal %d: %s\n", componentName, sig, strsignal(sig));

  msg.topic = topic;
  msg.data = (uint8_t*)buff;
  msg.length = snprintf(buff, sizeof(buff), "Hello from %s", componentName);

  printf("%s send the message \"%.*s\" to the topic \"%s\" \n", componentName, msg.length, (char const *)msg.data, msg.topic);
  ret = rbusMessage_Send(handle, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
  EXPECT_EQ(ret, RBUS_ERROR_SUCCESS);

  ret = rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  printf("%s: exit\n",componentName);
  return ret;
}

static int rbus_single_recv(const char *topic, pid_t pid, rbusGtestMsg_t test)
{
  int i = 0, ret = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  uint32_t listenerId;
  char user_data[32] = {0};
  rbusHandle_t handle;
  char *componentName = NULL;
  static pid_t pid_arr[3] = {0};
  int rbus_send_status = RBUS_ERROR_BUS_ERROR;
  rbusMsgData_t userData;

  for(i = 0 ; i < 3 ; i++)
  {
    if(0 == pid_arr[i])
    {
      pid_arr[i] = pid;
      if(2 != i)
        return 0;
    }
  }

  printf("%s: start \n",__func__);
  componentName = strdup(__func__);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  userData.test = test;
  userData.cond = NULL;
  strcpy(userData.componentName, componentName);

  ret |= rbusMessage_AddListener(handle, topic, &rbusRecvHandler, (void *)(&userData), 0);
  EXPECT_EQ(ret, RBUS_ERROR_SUCCESS);

  hasSenderStarted(handle, "rbus_multi_send0");
  hasSenderStarted(handle, "rbus_multi_send1");
  hasSenderStarted(handle, "rbus_multi_send2");

  kill(pid_arr[0],SIGUSR1);
  kill(pid_arr[1],SIGUSR1);
  kill(pid_arr[2],SIGUSR1);

  sleep(2);
  for(i = 0 ; i < 3 ; i++)
  {
    wait_ret = waitpid(pid_arr[i], &rbus_send_status, 0);
    EXPECT_EQ(wait_ret, pid_arr[i]);

    if(wait_ret != pid_arr[i]) printf("%s: waitpid() failed %d: %s\n",__func__,errno,strerror(errno));
    ret |= (wait_ret != pid_arr[i]) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;
  }

  ret |= rbusMessage_RemoveListener(handle, topic, listenerId);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  ret |= rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  free(componentName);

  printf("%s: exit \n",__func__);

  return ret;
}

static int rbus_multi_recv(const char *topic, int index, rbusGtestMsg_t test)
{
  rbusHandle_t handle;
  uint32_t listenerId;
  int ret = RBUS_ERROR_BUS_ERROR;
  char componentName[32] = {0};
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  rbusMsgData_t userData;

  snprintf(componentName,sizeof(componentName),"%s%d",__func__,index);
  printf("%s: start\n",componentName);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  userData.test = test;
  userData.cond = &cond;
  strcpy(userData.componentName, componentName);

  ret |= rbusMessage_AddListener(handle, topic, &rbusRecvHandler, (void *)(&userData), 0);
  EXPECT_EQ(ret, RBUS_ERROR_SUCCESS);

  pthread_cond_wait(&cond, &lock);

  ret |= rbusMessage_RemoveListener(handle, topic, listenerId);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

  ret |= rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  printf("%s: exit\n",componentName);
  return ret;
}

static int rbus_single_send(const char *topic, pid_t pid)
{
  int i = 0, ret = RBUS_ERROR_BUS_ERROR, wait_ret = -1;
  char user_data[32] = {0};
  rbusHandle_t handle;
  char *componentName = NULL;
  static pid_t pid_arr[3] = {0};
  int rbus_send_status = RBUS_ERROR_BUS_ERROR;
  rbusMessage_t msg;
  char buff[256];

  for(i = 0 ; i < 3 ; i++)
  {
    if(0 == pid_arr[i])
    {
      pid_arr[i] = pid;
      if(2 != i)
        return 0;
    }
  }

  printf("%s: start \n",__func__);
  componentName = strdup(__func__);
  ret = rbus_open(&handle, componentName);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
  if(RBUS_ERROR_SUCCESS != ret) goto exit;

  hasReceiverStarted(handle, "rbus_multi_recv0");
  hasReceiverStarted(handle, "rbus_multi_recv1");
  hasReceiverStarted(handle, "rbus_multi_recv2");

  msg.topic = topic;
  msg.data = (uint8_t*)buff;
  msg.length = snprintf(buff, sizeof(buff), "Hello from %s", componentName);

  printf("%s send the message \"%.*s\" to the topic \"%s\" \n", componentName, msg.length, (char const *)msg.data, msg.topic);
  ret = rbusMessage_Send(handle, &msg, RBUS_MESSAGE_CONFIRM_RECEIPT);
  EXPECT_EQ(ret, RBUS_ERROR_SUCCESS);

  for(i = 0 ; i < 3 ; i++)
  {
    wait_ret = waitpid(pid_arr[i], &rbus_send_status, 0);
    EXPECT_EQ(wait_ret, pid_arr[i]);
    if(wait_ret != pid_arr[i]) printf("%s: waitpid() failed %d: %s\n",__func__,errno,strerror(errno));
    ret |= (wait_ret != pid_arr[i]) ? RBUS_ERROR_BUS_ERROR : RBUS_ERROR_SUCCESS;
  }

  ret |= rbus_close(handle);
  EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);

exit:
  free(componentName);
  printf("%s: exit \n",__func__);
  return ret;
}

static void exec_msg_test(rbusGtestMsg_t test)
{
  switch(test)
  {
    case RBUS_GTEST_MSG1:
    case RBUS_GTEST_MSG2:
      {
        char topic[] = "A.B.C";
        pid_t pid = 0;

        pid = fork();
        if (0 == pid) {
          int ret = RBUS_ERROR_BUS_ERROR;
          rbusMessageSendOptions_t opt = RBUS_MESSAGE_CONFIRM_RECEIPT;

          if(RBUS_GTEST_MSG2 == test)
            opt = RBUS_MESSAGE_NONE;

          usleep(100000);
          ret = rbus_send(topic, opt, RBUS_ERROR_SUCCESS);
          exit(ret);
        } else {
          int ret = RBUS_ERROR_BUS_ERROR;
          int rbus_send_status = RBUS_ERROR_BUS_ERROR;
          ret = rbus_recv(topic, pid, &rbus_send_status, test);
          printf("rbus_send result %d\n",WEXITSTATUS(rbus_send_status));
          EXPECT_EQ(WEXITSTATUS(rbus_send_status),RBUS_ERROR_SUCCESS);
          EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
        }
      }
      break;
    case RBUS_GTEST_MSG3:
      {
        rbus_send("A.B.C", RBUS_MESSAGE_CONFIRM_RECEIPT, RBUS_ERROR_DESTINATION_NOT_FOUND);
      }
      break;
    case RBUS_GTEST_MSG4:
      {
        pid_t pid = fork();
        if (0 == pid){
          int ret = RBUS_ERROR_BUS_ERROR;
          usleep(100000);
          ret = rbus_send("A.B.D", RBUS_MESSAGE_CONFIRM_RECEIPT, RBUS_ERROR_DESTINATION_NOT_FOUND);
          exit(ret);
        }else{
          int ret = RBUS_ERROR_BUS_ERROR;
          int rbus_send_status = RBUS_ERROR_BUS_ERROR;
          ret = rbus_recv("A.B.C", pid, &rbus_send_status, test);
          printf("rbus_send result %d\n",WEXITSTATUS(rbus_send_status));
          EXPECT_EQ(WEXITSTATUS(rbus_send_status),RBUS_ERROR_SUCCESS);
          EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
        }
      }
      break;
    case RBUS_GTEST_MSG5:
    case RBUS_GTEST_MSG6:
      {
        char topic[] = "A.B.C";
        pid_t pid = 0;
        rbusMessageSendOptions_t opt = RBUS_MESSAGE_CONFIRM_RECEIPT;

        if(RBUS_GTEST_MSG6 == test)
          opt = RBUS_MESSAGE_NONE;

        pid = fork();
        if (0 == pid) {
          int ret = RBUS_ERROR_BUS_ERROR;
          ret = rbus_recv(topic, pid, NULL, test);
          EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
          exit(ret);
        } else {
          int rbus_recv_status = RBUS_ERROR_BUS_ERROR;
          int ret = RBUS_ERROR_BUS_ERROR;
          ret = rbus_send_neg(topic, pid, &rbus_recv_status, opt);
          printf("rbus_recv result %d\n",WEXITSTATUS(rbus_recv_status));
          EXPECT_EQ(WEXITSTATUS(rbus_recv_status),RBUS_ERROR_SUCCESS);
          EXPECT_EQ(ret,RBUS_ERROR_SUCCESS);
        }
      }
      break;
    case RBUS_GTEST_MSG7:
      {
        int i = 0;
        pid_t pid_arr[3];
        char topic[] = "A.B.C";
        int recv_ret = 0;

        for(i = 0 ; i < 3 ; i++ )
        {
          pid_arr[i] = fork();

          if (0 == pid_arr[i]) {
            int ret = RBUS_ERROR_BUS_ERROR;
            ret = rbus_multi_send(topic, i);
            exit(ret);
          } else {
            recv_ret = rbus_single_recv(topic, pid_arr[i], test);
          }
        }

        EXPECT_EQ(recv_ret, RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_MSG8:
      {
        int i = 0;
        pid_t pid_arr[3];
        char topic[] = "A.B.C";
        int recv_ret = 0;

        for(i = 0 ; i < 3 ; i++ )
        {
          pid_arr[i] = fork();

          if (0 == pid_arr[i]) {
            int ret = RBUS_ERROR_BUS_ERROR;
            ret = rbus_multi_recv(topic, i, test);
            exit(ret);
          } else {
            recv_ret = rbus_single_send(topic, pid_arr[i]);
          }
        }

        EXPECT_EQ(recv_ret, RBUS_ERROR_SUCCESS);
      }
      break;
    case RBUS_GTEST_MSG9:
      {
          rbusHandle_t rbus;
          uint32_t listenerId;

          EXPECT_EQ(rbus_open(&rbus, "rbus_recv"),0);
          EXPECT_EQ(rbusMessage_AddListener(rbus, "A.B.C", &rbusRecvHandler, NULL, 0),0);
          EXPECT_NE(rbusMessage_AddListener(rbus, "A.B.C", &rbusRecvHandler, NULL, 0),0);
          EXPECT_EQ(rbusMessage_RemoveListener(rbus, "A.B.C", 0),0);
          EXPECT_EQ(rbus_close(rbus),0);
      }
      break;
  }
}

TEST(rbusMessageTest, testsend1)
{
  exec_msg_test(RBUS_GTEST_MSG1);
}

TEST(rbusMessageTest, testsend2)
{
  exec_msg_test(RBUS_GTEST_MSG2);
}

TEST(rbusMessageTest, test_negsend1)
{
  exec_msg_test(RBUS_GTEST_MSG3);
}

TEST(rbusMessageTest, test_negsend2)
{
  exec_msg_test(RBUS_GTEST_MSG4);
}

TEST(rbusMessageTest, DISABLED_test_negsend3)
{
  exec_msg_test(RBUS_GTEST_MSG5);
}

TEST(rbusMessageTest, test_negsend4)
{
  exec_msg_test(RBUS_GTEST_MSG6);
}

TEST(rbusMessageTest, test_negsend5)
{
  exec_msg_test(RBUS_GTEST_MSG9);
}

TEST(rbusMessageTest, test_multi_send)
{
  exec_msg_test(RBUS_GTEST_MSG7);
}

TEST(rbusMessageTest, test_multi_recv)
{
  exec_msg_test(RBUS_GTEST_MSG8);
}
