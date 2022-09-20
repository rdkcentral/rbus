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
#ifndef __RT_RETAINABLE_H__
#define __RT_RETAINABLE_H__

#include "rtAtomic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtRetainable
{
  atomic_int refCount;
} rtRetainable;

#define rtRetainable_retain(X) if (X) rtRetainable_retainInternal((rtRetainable *)(X))
#define rtRetainable_release(X, D) if (X) rtRetainable_releaseInternal((rtRetainable *)(X), D)

void rtRetainable_retainInternal(rtRetainable* r);
void rtRetainable_releaseInternal(rtRetainable* r, void (*Destructor)(rtRetainable*));

#ifdef __cplusplus
}
#endif
#endif
