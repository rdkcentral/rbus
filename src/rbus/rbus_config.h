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

#ifndef RBUS_CONFIG_H
#define RBUS_CONFIG_H

#include "rbus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rbusConfig_t
{
    char*           tmpDir;             /* temp directory where rbus can persist data*/
    int             subscribeTimeout;   /* max time to attempt subscribe retries in milisecond*/
    int             subscribeMaxWait;   /* max time to wait between subscribe retries in miliseconds*/
    int             valueChangePeriod;  /* polling period for valuechange detector in miliseconds*/
    int             getTimeout;         /* default timeout in miliseconds for GET API*/
    int             setTimeout;         /* default timeout in miliseconds for SET API*/
} rbusConfig_t;

void rbusConfig_CreateOnce();
void rbusConfig_Destroy();
rbusConfig_t* rbusConfig_Get();
int rbusConfig_ReadGetTimeout();
int rbusConfig_ReadSetTimeout();

#ifdef __cplusplus
}
#endif
#endif
