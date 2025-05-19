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
#include "rtThreadPool.h"
#include "rtList.h"
#include "rtLog.h"
#include "rtMemory.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define RT_THREAD_POOL_DEFAULT_EXPIRE_TIME 10

typedef struct _rtThread* rtThread;
typedef struct _rtThreadTask* rtThreadTask;

struct _rtThreadPool
{
  rtList threadList;
  rtList taskList;
  size_t maxThreadCount;
  size_t threadCount;
  size_t busyThreadCount;
  size_t stackSize;
  int expireTime;
  int isRunning;
  int isShutdown;
  pthread_cond_t taskCond;
  pthread_cond_t idleCond;
  pthread_mutex_t poolLock;
};

struct _rtThreadTask
{
  rtThreadPoolFunc func;
  void* userData;
};

struct _rtThread
{
  pthread_t threadId;
  rtThreadPool pool;
  struct _rtThreadTask task;
};

void rtThreadPool_CleanupThread(void* data)
{
  rtLog_Debug("%s enter", __FUNCTION__);
  free(data);
  rtLog_Debug("%s exit", __FUNCTION__);
}

void rtThreadPool_TryRealDestroy(rtThreadPool pool)
{
  int shouldDestroy;
  pthread_mutex_lock(&pool->poolLock);
  shouldDestroy = pool->threadCount == 0 && pool->isShutdown == 1;
  pthread_mutex_unlock(&pool->poolLock);
  if(!shouldDestroy)
    return;
  rtLog_Debug("%s enter", __FUNCTION__);
  rtList_Destroy(pool->threadList, rtThreadPool_CleanupThread);
  rtList_Destroy(pool->taskList, rtList_Cleanup_Free);
  pthread_mutex_destroy(&pool->poolLock);
  pthread_cond_destroy(&pool->taskCond);
  pthread_cond_destroy(&pool->idleCond);
  free(pool);
  rtLog_Debug("%s exit", __FUNCTION__);
}

int rtThread_DequeTask(rtThread thread)
{
  rtListItem item = NULL;
  rtLog_Debug("%s enter", __FUNCTION__);
  if(thread->pool->isRunning)
  {
    rtList_GetFront(thread->pool->taskList, &item);
    if(item)
    {
      size_t size;
      rtLog_Debug("%s found item", __FUNCTION__);
      rtThreadTask task;
      rtListItem_GetData(item, (void**)&task);
      //copy the data because taskList is using rtListReuseData and will recycle task
      thread->task = *task;
      //remove the item now and pass NULL so we reuse item later
      rtList_RemoveItem(thread->pool->taskList, item, NULL);
      rtList_GetSize(thread->pool->taskList, &size);
      rtLog_Debug("%s exit with new task. %zu tasks remaining in queue", __FUNCTION__, size);
      return 1;
    }
  }
  rtLog_Debug("%s exit without task", __FUNCTION__);
  return 0;
}

void* rtThread_WorkerThreadFunc(void* data)
{
  rtLog_Debug("%s enter", __FUNCTION__);
  rtThread thread = (rtThread)data;
  rtThreadPool pool = thread->pool;
  pthread_mutex_lock(&pool->poolLock);
  pool->threadCount++;
  while(pool->isRunning)
  {
    while(pool->isRunning && rtThread_DequeTask(thread))
    {
      pool->busyThreadCount++;
      pthread_mutex_unlock(&pool->poolLock);
      thread->task.func(thread->task.userData);
      pthread_mutex_lock(&pool->poolLock);
      pool->busyThreadCount--;
      pthread_cond_broadcast(&pool->idleCond);
    }
    if(pool->isRunning)
    {
      int rc;
      struct timespec expireTime;
      clock_gettime(CLOCK_REALTIME, &expireTime);
      expireTime.tv_sec += pool->expireTime;
      rtLog_Debug("%s pthread_cond_wait before", __FUNCTION__);
      rc = pthread_cond_timedwait(&pool->taskCond, &pool->poolLock, &expireTime);
      rtLog_Debug("%s pthread_cond_wait after rc=%d", __FUNCTION__, rc);
      if(rc == ETIMEDOUT)
      { 
        rtLog_Debug("%s thread expired", __FUNCTION__);
        break;
      }
    }
  }
  pool->threadCount--;
  rtList_RemoveItemWithData(pool->threadList, thread, rtThreadPool_CleanupThread);
  pthread_mutex_unlock(&pool->poolLock);
  rtThreadPool_TryRealDestroy(pool);
  rtLog_Debug("%s exit", __FUNCTION__);
  return NULL;
}

rtError rtThreadPool_CreateWorkerThread(rtThreadPool pool)
{
  rtThread thread;
  pthread_attr_t attr;
  rtLog_Debug("%s enter", __FUNCTION__);
  thread = rt_try_malloc(sizeof(struct _rtThread));
  if(!thread)
    return rtErrorFromErrno(ENOMEM);
  thread->pool = pool;
  pthread_attr_init(&attr);
  if(pool->stackSize > 0)
    pthread_attr_setstacksize(&attr, pool->stackSize);
  pthread_create(&thread->threadId, &attr, rtThread_WorkerThreadFunc, thread);
  pthread_attr_destroy(&attr);
  rtList_PushBack(pool->threadList, thread, NULL);
  rtLog_Debug("%s exit", __FUNCTION__);
  return RT_OK;
}

rtError rtThreadPool_Create(rtThreadPool* ppool, size_t maxThreadCount, size_t stackSize, int expireTime)
{
  rtLog_Debug("%s enter", __FUNCTION__);
  rtThreadPool pool = rt_try_malloc(sizeof(struct _rtThreadPool));
  if(!pool)
    return rtErrorFromErrno(ENOMEM);
  rtList_Create(&pool->threadList);
  rtList_Create(&pool->taskList);
  pool->maxThreadCount = maxThreadCount;
  pool->threadCount = 0;
  pool->busyThreadCount = 0;
  pool->stackSize = stackSize;
  if(expireTime > 0)
    pool->expireTime = expireTime;
  else
    pool->expireTime = RT_THREAD_POOL_DEFAULT_EXPIRE_TIME;
  pool->isRunning = 1;
  pool->isShutdown = 0;
  pthread_mutexattr_t mutAttr;
  pthread_mutexattr_init(&mutAttr);
  pthread_mutexattr_settype(&mutAttr, PTHREAD_MUTEX_NORMAL);
  pthread_mutex_init(&pool->poolLock, &mutAttr);
  pthread_mutexattr_destroy(&mutAttr);
  pthread_condattr_t condAttr;
  pthread_condattr_init(&condAttr);
  pthread_cond_init(&pool->taskCond, &condAttr);
  pthread_cond_init(&pool->idleCond, &condAttr);
  pthread_condattr_destroy(&condAttr);
  *ppool = pool;
  rtLog_Debug("%s exit", __FUNCTION__);
  return RT_OK;
}

void timeAddMS(struct timespec* tsIn, int ms, struct timespec* tsOut)
{
  tsOut->tv_sec =  tsIn->tv_sec + ms / 1000;
  tsOut->tv_nsec = tsIn->tv_nsec + (ms % 1000) * 1000000;
  if(tsOut->tv_nsec >= 1000000000)
  {
    tsOut->tv_sec++;
    tsOut->tv_nsec -= 1000000000;
  }
}
int timeGetElapsed(struct timespec* startTime)
{
  int ms = 0;
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  if(now.tv_nsec >= startTime->tv_nsec)
  {
    ms = (now.tv_sec - startTime->tv_sec) * 1000 +
         (now.tv_nsec - startTime->tv_nsec) / 1000000;
  }
  else
  {
    ms = (now.tv_sec - startTime->tv_sec - 1) * 1000 +
        (1000000000 + now.tv_nsec - startTime->tv_nsec);
  }
  return ms;
}

rtError rtThreadPool_StopAllThread(rtThreadPool pool, int waitTimeMS)
{
  size_t cancelledTaskCount;
  rtLog_Debug("%s enter waitTimeMS=%d", __FUNCTION__, waitTimeMS);
  pthread_mutex_lock(&pool->poolLock);
  if(waitTimeMS < 0)
  {
    //wait for all tasks to complete
    while(pool->busyThreadCount > 0)
      pthread_cond_wait(&pool->idleCond, &pool->poolLock);
  }
  else //waitTimeMS > 0
  {
    int waitRemainingMS = waitTimeMS;
    while(waitRemainingMS > 0 && pool->busyThreadCount > 0)
    {
      struct timespec startTime, endTime;
      clock_gettime(CLOCK_REALTIME, &startTime);
      timeAddMS(&startTime, waitRemainingMS, &endTime);
      pthread_cond_timedwait(&pool->idleCond, &pool->poolLock, &endTime);
      waitRemainingMS -= timeGetElapsed(&startTime);
      rtLog_Debug("%s waitRemainingMS=%d", __FUNCTION__, waitRemainingMS);
    }
  }
  pool->isRunning = 0;
  pthread_cond_broadcast(&pool->taskCond);
  rtList_GetSize(pool->taskList, &cancelledTaskCount);

  //temporary verification that the code does what I think it does
  //that is, the thread calling rtThreadPool_StopAllThread will never
  //see the pool->busyThreadCount go to 0 without the task list going to zero
  if(pool->busyThreadCount == 0 && cancelledTaskCount != 0)
    rtLog_Warn("Assertion failed: task list not empty");

  rtLog_Debug("%s exit with %zu cancelled tasks and %zu tasks still finishing", 
    __FUNCTION__, cancelledTaskCount, pool->busyThreadCount);
  pthread_mutex_unlock(&pool->poolLock);
  if(cancelledTaskCount)
    return RT_ERROR_THREADPOOL_TASKS_CANCELLED;
  else
    return RT_OK;
}

rtError rtThreadPool_Destroy(rtThreadPool pool, int waitTimeMS)
{
  rtError err;
  rtLog_Debug("%s enter", __FUNCTION__);
  err = rtThreadPool_StopAllThread(pool, waitTimeMS);
  pthread_mutex_lock(&pool->poolLock);
  pool->isShutdown = 1;
  pthread_mutex_unlock(&pool->poolLock);
  rtThreadPool_TryRealDestroy(pool);
  rtLog_Debug("%s exit", __FUNCTION__);
  return err;
}

rtError rtThreadPool_RunTask(rtThreadPool pool, rtThreadPoolFunc func, void* userData)
{
  rtLog_Debug("%s enter", __FUNCTION__);
  rtError err = RT_OK;
  rtListItem item;
  rtThreadTask task;
  pthread_mutex_lock(&pool->poolLock);
  //enque task
  rtList_PushBack(pool->taskList, rtListReuseData/*reuse data to avoid many allocs*/, &item);
  rtListItem_GetData(item, (void**)&task);
  if(!task)//first time data being used
  {
    task = rt_try_malloc(sizeof(struct _rtThreadTask));
    if(!task)
    {
        pthread_mutex_unlock(&pool->poolLock);
        return rtErrorFromErrno(ENOMEM);
    }
    rtLog_Debug("taskList data null so alloc new %p", (void*)task);
    rtListItem_SetData(item, task);
  }
  task->func = func;
  task->userData = userData;
  //find and existing thread to handle the task
  if(pool->busyThreadCount < pool->threadCount)
  {
    rtLog_Debug("%s pthread_cond_signal before", __FUNCTION__);
    pthread_cond_signal(&pool->taskCond);
    rtLog_Debug("%s pthread_cond_signal after", __FUNCTION__);
  }
  else if(pool->threadCount < pool->maxThreadCount)
  {
    rtLog_Debug("%s creating new thread", __FUNCTION__);
    if((err = rtThreadPool_CreateWorkerThread(pool)) != RT_OK)
    {
        pthread_mutex_unlock(&pool->poolLock);
        return err;
    }
    if(pool->threadCount == pool->maxThreadCount - 1)
      rtLog_Debug("%s reached max thread count %zu", __FUNCTION__, pool->maxThreadCount);
  }
  else
  {
    rtLog_Debug("%s at max thread count %zu", __FUNCTION__, pool->maxThreadCount);
    err = RT_ERROR_THREADPOOL_TASK_PENDING;
  }
  pthread_mutex_unlock(&pool->poolLock);
  rtLog_Debug("%s exit", __FUNCTION__);
  return err;
}
