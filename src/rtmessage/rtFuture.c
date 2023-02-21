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
# This file contains material from pxCore which is Copyright 2005-2018 John Robinson
# Licensed under the Apache-2.0 license.
*/
#include "rtFuture.h"
#include "rtLog.h"
#include "rtMemory.h"

#include <pthread.h>
#include <stdbool.h>

struct rtFuture {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  void* value;
  bool completed;
  rtError status;
};

rtFuture_t* rtFuture_Create()
{
  rtFuture_t* f = rt_malloc(sizeof(rtFuture_t));
  pthread_mutex_init(&f->mutex, NULL);
  pthread_cond_init(&f->cond, NULL);
  f->value = NULL;
  f->completed = false;
  f->status = 0;
  return f;
}

void rtFuture_Destroy(rtFuture_t* f, void (*value_cleanup)(void *))
{
  if (f) {
    if (value_cleanup && f->value) {
      value_cleanup(f->value);
    }
    rt_free(f);
  }
}

rtError rtFuture_Wait(rtFuture_t* f, int millis)
{
  rtError err = RT_ERROR_TIMEOUT;

  struct timespec wait_until = { 0, 0 };

  if (millis != RT_TIMEOUT_INFINITE) {
    int32_t s = millis / 1000;
    int32_t m = (millis - (s * 1000));

    clock_gettime(CLOCK_REALTIME, &wait_until);
    wait_until.tv_sec += s;
    wait_until.tv_nsec += (m * 1000 * 1000);
  }

  pthread_mutex_lock(&f->mutex);
  while (!f->completed) {
    if (millis != RT_TIMEOUT_INFINITE)
      pthread_cond_timedwait(&f->cond, &f->mutex, &wait_until);
    else
      pthread_cond_wait(&f->cond, &f->mutex);
  }
  if (f->completed) {
    err = f->status;
    err = RT_OK;
  }
  pthread_mutex_unlock(&f->mutex);

  return err;
}

void* rtFuture_GetValue(rtFuture_t* f)
{
  return f->value;
}

void rtFuture_SetCompleted(rtFuture_t* f, rtError err, void* val)
{
  pthread_mutex_lock(&f->mutex);
  f->value = val;
  f->status = err;
  f->completed = true;
  pthread_mutex_unlock(&f->mutex);
  pthread_cond_signal(&f->cond);
}
