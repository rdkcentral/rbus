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
#include "rtTime.h"
#include "rtError.h"
#include "rtLog.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define RT_CLOCK_ID CLOCK_MONOTONIC

rtTime_t* rtTime_Now(rtTime_t* t)
{
    int rc;

    rc = clock_gettime(RT_CLOCK_ID, t);

    if(rc == -1)
    {
        rtLog_Error("clock_gettime failed: %s", rtStrError(rtErrorFromErrno(errno)));
    }
    return t;
}


void rtTime_Later(const rtTime_t* start, int ms, rtTime_t* result)
{
    rtTime_t now;

    if(!start)
    {
        rtTime_Now(&now);
        start = &now;
    }

    result->tv_sec = start->tv_sec + ms / 1000;
    result->tv_nsec = start->tv_nsec + (ms % 1000) * 1000000;

    if(result->tv_nsec >= 1000000000)
    {
        result->tv_sec++;
        result->tv_nsec -= 1000000000;
    }
}

int rtTime_Elapsed(const rtTime_t* start, const rtTime_t* end)
{
    int ms = 0;
    rtTime_t now;

    if(!end)
    {
        rtTime_Now(&now);
        end = &now;
    }

    if(end->tv_nsec >= start->tv_nsec)
    {
        ms = (end->tv_sec - start->tv_sec) * 1000
           + (end->tv_nsec - start->tv_nsec) / 1000000;
    }
    else
    {
        ms = (end->tv_sec - start->tv_sec - 1) * 1000
           + (1000000000 + end->tv_nsec - start->tv_nsec) / 1000000;
    }

    return ms;
}

int rtTime_Compare(const rtTime_t* left, const rtTime_t* right)
{
    rtTime_t now;

    if(!left || !right)
    {
        rtTime_Now(&now);
        if(!left)
            left = &now;
        if(!right)
            right = &now;
    }

    if(left->tv_sec < right->tv_sec ||
        ((left->tv_sec == right->tv_sec) && 
          left->tv_nsec < right->tv_nsec))
    {
        return -1;
    }
    else if(left->tv_sec > right->tv_sec ||
        ((left->tv_sec == right->tv_sec) && 
          left->tv_nsec > right->tv_nsec))
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

const char* rtTime_ToString(const rtTime_t* tm, char* buffer)
{
    /*localtime may not be thread safe*/
    struct tm* lt = localtime(&tm->tv_sec);

    sprintf(buffer, "%.2d:%.2d:%.2d.%.6ld",
        lt->tm_hour, lt->tm_min, lt->tm_sec, tm->tv_nsec / 1000);

    return buffer;
}

const rtTimespec_t* rtTime_ToTimespec(const rtTime_t* tm, rtTimespec_t* result)
{
    *result = *tm;

    return result;
}

