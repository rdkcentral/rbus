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
#ifndef RT_FUTURE_H
#define RT_FUTURE_H

#include <rtError.h>

struct rtFuture;
typedef struct rtFuture rtFuture_t;

rtFuture_t* rtFuture_Create();

void rtFuture_Destroy(rtFuture_t* f, void (*value_cleanup)(void*));

rtError rtFuture_Wait(rtFuture_t* f, int millis);

void* rtFuture_GetValue(rtFuture_t* f);

void rtFuture_SetCompleted(rtFuture_t* f, rtError err, void* val);

#endif
