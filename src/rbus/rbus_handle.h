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
#ifndef RBUS_HANDLE_H
#define RBUS_HANDLE_H

#include "rbus_element.h"
#include "rbus_subscriptions.h"
#include "rbus_valuechange.h"

#include <rtAtomic.h>
#include <rtConnection.h>
#include <rtVector.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  RBUS_MAX_HANDLES 16

  rtConnection has 64 listener limit.
  16 allows:
    1 inbox (registered on first rbus_open)
    1 advisory (also registered on the first rbus_open)
    16 component names (registered on each rbus_open call)
    46 additional listeners which can be used by the rbus_message api, other rtConnection clients or for rbus future requirements
*/
#define RBUS_MAX_HANDLES 16


struct rbusRunnable {
  // opaque user data
  void  *argp;

  // function to use to execute current runnable. this function will take
  // argp as it's one and only argument.
  void (*exec)(void *);

  // rbus will internall use this function to cleanup argp after the runnable
  // has been executed.
  void (*cleanup)(void *);

  // internally, we maintain a list of runnables
  struct rbusRunnable* next;

  uint32_t id;
};

struct rbusRunnableQueue {
  struct rbusRunnable *head;
  struct rbusRunnable *tail;
  int pipe_fds[2];
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

typedef struct rbusRunnable rbusRunnable_t;
typedef struct rbusRunnableQueue rbusRunnableQueue_t;
typedef void (*rbusRunnableQueue_MessageHandler_t)(rbusRunnable_t *r);

RTLIB_PRIVATE void rbusRunnableQueue_Init(rbusRunnableQueue_t *q);
RTLIB_PRIVATE void rbusRunnableQueue_PushBack(rbusRunnableQueue_t *q, rbusRunnable_t r);
RTLIB_PRIVATE rbusRunnable_t * rbusRunnableQueue_PopFront(rbusRunnableQueue_t *q);
RTLIB_PRIVATE int rbusRunnableQueue_GetReadFileDescriptor(rbusRunnableQueue_t* q);
RTLIB_PRIVATE void rbusRunnableQueue_Dispatch(rbusRunnableQueue_t* q, rbusRunnableQueue_MessageHandler_t h);


struct _rbusHandle
{
  char*                 componentName;
  int32_t               componentId;
  elementNode*          elementRoot;

  /* consumer side subscriptions FIXME -
    this needs to be an associative map instead of list/vector*/
  rtVector              eventSubs; 

  /* provider side subscriptions */
  rbusSubscriptions_t   subscriptions; 

  rtVector              messageCallbacks;
  rtConnection          connection;

  rbusRunnableQueue_t   eventQueue;
  bool                  useEventLoop;

  rbusValueChangeDetector_t valueChangeDetector;

  uint32_t              busyWaitSleepDurationMillis;
  atomic_int            reentrancyGuard;
};

void rbusHandleList_Add(struct _rbusHandle* handle);
void rbusHandleList_Remove(struct _rbusHandle* handle);
bool rbusHandleList_IsEmpty();
bool rbusHandleList_IsFull();
void rbusHandleList_ClientDisconnect(char const* clientListener);
struct _rbusHandle* rbusHandleList_GetByComponentID(int32_t componentId);
struct _rbusHandle* rbusHandleList_GetByName(char const* componentName);

#ifdef __cplusplus
}
#endif
#endif
