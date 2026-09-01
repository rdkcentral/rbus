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

#ifndef __RBUS_TEST_RUNNINGPARAM_H
#define __RBUS_TEST_RUNNINGPARAM_H

#include <stdlib.h>
#include <rbus.h>

#include <rbuscore.h>

#ifdef __cplusplus
extern "C" {
#endif

rbusError_t
runningParamProvider_Init(rbusHandle_t handle, char* paramName);

int
runningParamProvider_IsRunning();

int
runningParamConsumer_Set(rbusHandle_t handle, char* paramName, bool running);

#ifdef __cplusplus
}
#endif
#endif
