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
#include "rbus_config.h"
#include "rbus_log.h"
#include <rtMemory.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* These RBUS_* defines are the list of config settings with their default values
 * Each can be overridden using an environment variable of the same name
 */
#define RBUS_TMP_DIRECTORY      "/tmp"      /*temp directory where persistent data can be stored*/
#define RBUS_SUBSCRIBE_TIMEOUT   600000     /*subscribe retry timeout in miliseconds*/
#define RBUS_SUBSCRIBE_MAXWAIT   60000      /*subscribe retry max wait between retries in miliseconds*/
#define RBUS_VALUECHANGE_PERIOD  2000       /*polling period for valuechange detector*/
#define RBUS_GET_DEFAULT_TIMEOUT 15000      /* default timeout in miliseconds for GET API */
#define RBUS_GET_WILDCARD_DEFAULT_TIMEOUT 120000 /* default timeout in miliseconds for Wildcard GET API */
#define RBUS_SET_DEFAULT_TIMEOUT 15000      /* default timeout in miliseconds for SET API */
#define RBUS_GET_TIMEOUT_OVERRIDE "/tmp/rbus_timeout_get"
#define RBUS_GET_WILDCARD_TIMEOUT_OVERRIDE "/tmp/rbus_timeout_get_wildcard_query"
#define RBUS_SET_TIMEOUT_OVERRIDE "/tmp/rbus_timeout_set"

#define initStr(P,N) \
{ \
    char* V = getenv(#N); \
    P=strdup((V && strlen(V)) ? V : N); \
    RBUSLOG_DEBUG(#N"=%s",P); \
}

#define initInt(P,N) \
{ \
    char* V = getenv(#N); \
    P=((V && strlen(V)) ? atoi(V) : N); \
    RBUSLOG_DEBUG(#N"=%d",P); \
}

static rbusConfig_t* gConfig = NULL;

void rbusConfig_CreateOnce()
{
    if(gConfig)
        return;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    gConfig = rt_malloc(sizeof(struct _rbusConfig_t));

    initStr(gConfig->tmpDir,                RBUS_TMP_DIRECTORY);
    initInt(gConfig->subscribeTimeout,      RBUS_SUBSCRIBE_TIMEOUT);
    initInt(gConfig->subscribeMaxWait,      RBUS_SUBSCRIBE_MAXWAIT);
    initInt(gConfig->valueChangePeriod,     RBUS_VALUECHANGE_PERIOD);
    initInt(gConfig->getTimeout,            RBUS_GET_DEFAULT_TIMEOUT);
    initInt(gConfig->setTimeout,            RBUS_SET_DEFAULT_TIMEOUT);
    initInt(gConfig->getWildcardTimeout,    RBUS_GET_WILDCARD_DEFAULT_TIMEOUT);
}

void rbusConfig_Destroy()
{
    if(!gConfig)
        return;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    free(gConfig->tmpDir);
    free(gConfig);
    gConfig = NULL;
}

rbusConfig_t* rbusConfig_Get()
{
    return gConfig;
}

int rbusConfig_ReadGetTimeout()
{
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};

    if (access(RBUS_GET_TIMEOUT_OVERRIDE, F_OK) == 0)
    {
        fp = fopen(RBUS_GET_TIMEOUT_OVERRIDE, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    rbusConfig_CreateOnce();
    return gConfig->getTimeout;
}

int rbusConfig_ReadWildcardGetTimeout()
{
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};

    if (access(RBUS_GET_WILDCARD_TIMEOUT_OVERRIDE, F_OK) == 0)
    {
        fp = fopen(RBUS_GET_WILDCARD_TIMEOUT_OVERRIDE, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    rbusConfig_CreateOnce();
    return gConfig->getWildcardTimeout;
}

int rbusConfig_ReadSetTimeout()
{
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};

    if (access(RBUS_SET_TIMEOUT_OVERRIDE, F_OK) == 0)
    {
        fp = fopen(RBUS_SET_TIMEOUT_OVERRIDE, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    rbusConfig_CreateOnce();
    return gConfig->setTimeout;
}
