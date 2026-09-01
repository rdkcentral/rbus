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
#ifndef __RTMSG_THREADPOOL_H__
#define __RTMSG_THREADPOOL_H__

#include "rtError.h"
#include <stddef.h>

#define RT_ERROR_THREADPOOL_TASKS_CANCELLED   3000
#define RT_ERROR_THREADPOOL_TASK_PENDING      3001

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rtThreadPool* rtThreadPool;
typedef void (*rtThreadPoolFunc)(void* userData);

rtError rtThreadPool_Create(rtThreadPool* ppool, size_t maxThreadCount, size_t stackSize, int expireTime);
rtError rtThreadPool_Destroy(rtThreadPool pool, int waitTimeMS);
rtError rtThreadPool_RunTask(rtThreadPool pool, rtThreadPoolFunc func, void* userData);

#ifdef __cplusplus
}
#endif
#endif
