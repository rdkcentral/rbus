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
#ifndef __RT_ATOMIC_H__
#define __RT_ATOMIC_H__

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 8))) && !defined(NO_ATOMICS)
  #define RT_ATOMIC_HAS_ATOMIC_FETCH
#elif defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 0))) && !defined(NO_ATOMICS)
  #define RT_ATOMIC_HAS_SYNC_FETCH
#endif

#if defined(RT_ATOMIC_HAS_ATOMIC_FETCH)
  #ifdef __cplusplus
    #include <atomic>
    using namespace std;
  #else
    #include <stdatomic.h>
  #endif
#elif defined(RT_ATOMIC_HAS_SYNC_FETCH)
  #define atomic_int volatile int
#else
  #include <pthread.h>
  #define atomic_int volatile int
  static pthread_mutex_t g_atomic_mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline int rtAtomicFetchAdd(atomic_int* var, int value)
{
#if defined(RT_ATOMIC_HAS_ATOMIC_FETCH)
  #ifdef __cplusplus
    return atomic_fetch_add(var, value);
  #else
    return __atomic_fetch_add(var, value, __ATOMIC_SEQ_CST);
  #endif
#elif defined(RT_ATOMIC_HAS_SYNC_FETCH)
    return __sync_fetch_and_add(var, value);
#else
    int original_value = 0;
    pthread_mutex_lock(&g_atomic_mutex);
    if (NULL != var) {
        original_value = *var;
        *(var) = *(var) + value;
    }
    pthread_mutex_unlock(&g_atomic_mutex);
    return original_value;
#endif
}

static inline void rt_atomic_fetch_add(atomic_int* var, int value)
{
  return (void) rtAtomicFetchAdd(var, value);
}


static inline int rtAtomicFetchSub(atomic_int* var, int value)
{
#if defined(RT_ATOMIC_HAS_ATOMIC_FETCH)
  #ifdef __cplusplus
    return atomic_fetch_sub(var, value);
  #else
    return __atomic_fetch_sub(var, value, __ATOMIC_SEQ_CST);
  #endif
#elif defined(RT_ATOMIC_HAS_SYNC_FETCH)
    return __sync_fetch_and_sub(var, value);
#else
    int original_value = 0
    pthread_mutex_lock(&g_atomic_mutex);
    if (NULL != var) {
        original_value = *var;
        *(var) = *(var) - value;
    }
    pthread_mutex_unlock(&g_atomic_mutex);
    return original_value;
#endif
}

static inline void rt_atomic_fetch_sub(atomic_int* var, int value)
{
  return (void) rt_atomic_fetch_add(var, value);
}

#ifdef __cplusplus
}
#endif
#endif
