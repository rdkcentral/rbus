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
#ifndef __RT_SEMAPHORE_H__
#define __RT_SEMAPHORE_H__

#include "rtError.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* simple unnamed single process semaphore 
 * all function will return RT_OK on success
 * or RT_ERROR for any error except as noted below
 */

struct _rtSemaphore;
typedef struct _rtSemaphore* rtSemaphore;

/** @fn rtError rtSemaphore_Create(rtSemaphore* sem, int, unsigned int)
 *  @brief Create an unnamed single process semaphore with
 *         an initial value of 0.
 *  @param sem  the semaphore to init
 */
rtError rtSemaphore_Create(rtSemaphore* sem);

/** @fn rtError rtSemaphore_Destroy(rtSemaphore sem)
 *  @brief Destroy a semophore
 *  @param sem  a semaphore to destroy
 */
rtError rtSemaphore_Destroy(rtSemaphore sem);

/** @fn rtError rtSemaphore_Post(rtSemaphore sem)
 *  @brief increment the samephore value by 1 and 
 *         wake up any thread waiting on the semaphore
 *  @param sem  a semaphore to post to
 */
rtError rtSemaphore_Post(rtSemaphore sem);

/** @fn rtError rtSemaphore_GetValue(rtSemaphore sem)
 *  @brief Get the value of the semaphore
 *  @param sem  a semaphore
 *  @param val  out param to pass the value
 */
rtError rtSemaphore_GetValue(rtSemaphore sem, int* val);

/** @fn rtError rtSemaphore_Wait(rtSemaphore sem)
 *  @brief Wait for the semaphore to obtain a value > 0
 *  @param sem  a semaphore
 *  @return RT_OK if success or RT_ERROR if fail
 */
rtError rtSemaphore_Wait(rtSemaphore sem);

/** @fn rtError rtSemaphore_TimedWait(rtSemaphore sem, struct timespec* t)
 *  @brief Wait a period of time for the semaphore to obtain a value > 0
 *  @param sem  a semaphore
 *  @param t  the future monotonic time to wait until (e.g. the timeout)
 *  @return RT_OK if success, RT_ERROR if fail, RT_ERROR_TIMEOUT is timeout reached
 */
rtError rtSemaphore_TimedWait(rtSemaphore sem, struct timespec* t);

#ifdef __cplusplus
}
#endif
#endif
