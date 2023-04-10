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
#ifndef __RT_TIME_H__
#define __RT_TIME_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timespec rtTime_t;
typedef struct timespec rtTimespec_t;

/** @fn void rtTime_Now (rtTime_t* t)
 *  @brief Get the current time
 *  @param  result  resulting time
 */
rtTime_t* rtTime_Now(rtTime_t* result);

/** @fn void rtTime_Later (rtTime_t* from, int ms, rtTime_t* result)
 *  @brief Get a the time 'ms' miliseconds from time 'from'
 *  @param  from    from time.  if NULL, now will be use
 *  @param  ms      miliseconds to add to 'from' time
 *  @param  result  resulting time
 */
void rtTime_Later(const rtTime_t* from, int ms, rtTime_t* result);


/** @fn int rtTime_Elapsed (rtTime_t* start, rtTime_t* end)
 *  @brief Get the elapsed miliseconds between start and end times
 *  @param  start    start time
 *  @param  end      end time.  if NULL, now will be use
 *  @return elapsed time in miliseconds
 */
int rtTime_Elapsed(const rtTime_t* start, const rtTime_t* end);


/** @fn int rtTime_Compare (rtTime_t* left, rtTime_t* right)
 *  @brief Compare two times returning a relational numeric value
 *  @param  left    left time. if NULL, now will be use
 *  @param  right   right time.  if NULL, now will be use
 *  @return -1 if left time is before right time
 *           1 if left time is after right time
 *           0 if left and right times are equal to the milisecond
 */
int rtTime_Compare(const  rtTime_t* left, const rtTime_t* right);

/** @fn const char* rtTime_ToString (rtTime_t* tm, char* buffer)
 *  @brief Convert time into a friendly string in the format HH:MM:SS.mmm (mmm=miliseconds) (e.g. 12:30:15.250)
 *  @param  tm      a time
 *  @param  buffer  a buffer to put the string into. should have minimum capactity of 13 bytes
 *  @return buffer param with result
 */
const char* rtTime_ToString(const rtTime_t* tm, char* buffer);

/** @fn const rtTimespec_t* rtTime_ToTimespec (rtTime_t* tm, struct timespec* result)
 *  @brief Convert time into a timespec needed by some functions such as pthread_cond_timedwait
 *  @param  tm      a time
 *  @param  result  resulting timespec
 *  @return timespec result
 */
const rtTimespec_t* rtTime_ToTimespec(const rtTime_t* tm, rtTimespec_t* result);

#ifdef __cplusplus
}
#endif
#endif
