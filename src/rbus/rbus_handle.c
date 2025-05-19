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
#include "rbus_handle.h"
#include <string.h>
#include <rbuscore.h>
#include <rtMemory.h>
#include <unistd.h>
#include <stdlib.h>
static rtVector gHandleList = NULL;

#define VERIFY_NULL(T,R) if(NULL == T){ RBUSLOG_ERROR(#T" is NULL"); R; }
#define BUF_LEN 128
#define RBUS_GET_DEFAULT_TIMEOUT          15000   /* default timeout in miliseconds for GET API */
#define RBUS_GET_WILDCARD_DEFAULT_TIMEOUT 120000  /* default timeout in miliseconds for Wildcard GET API */
#define RBUS_SET_DEFAULT_TIMEOUT          15000   /* default timeout in miliseconds for SET API */
#define RBUS_SET_MULTI_DEFAULT_TIMEOUT    60000   /* default timeout in miliseconds for SET Multi API */

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
extern char *__progname;

bool rbusHandleList_IsValidHandle(struct _rbusHandle* handle)
{
    struct _rbusHandle* tmphandle = NULL;
    if (handle == NULL)
    {
        return false;
    }
    int i;
    if(gHandleList)
    {
        int len = rtVector_Size(gHandleList);
        for(i = 0; i < len; i++)
        {
            tmphandle = (struct _rbusHandle*)rtVector_At(gHandleList, i);
            if(tmphandle == handle)
            {
                return true;
            }
        }
    }
    return false;
}

void rbusHandleList_Add(struct _rbusHandle* handle)
{
    VERIFY_NULL(handle,return);
    if(!gHandleList)
        rtVector_Create(&gHandleList);
    RBUSLOG_DEBUG("adding %p", handle);
    rtVector_PushBack(gHandleList, handle);
}

void rbusHandleList_Remove(struct _rbusHandle* handle)
{
    VERIFY_NULL(handle,return);
    RBUSLOG_DEBUG("removing %p", handle);
    rtVector_RemoveItem(gHandleList, handle, rtVector_Cleanup_Free);
    if(rtVector_Size(gHandleList) == 0)
    {
        rtVector_Destroy(gHandleList, NULL);
        gHandleList = NULL;
        return;
    }
}

bool rbusHandleList_IsEmpty()
{
    return gHandleList == NULL;
}

bool rbusHandleList_IsFull()
{
    RBUSLOG_DEBUG("size=%zu", rtVector_Size(gHandleList));
    return (gHandleList && rtVector_Size(gHandleList) >= RBUS_MAX_HANDLES);
}

void rbusHandleList_ClientDisconnect(char const* clientListener)
{
    size_t i;
    size_t len;
    VERIFY_NULL(clientListener,return);
    if(gHandleList)/*this could theoretically be null if advisory event comes in between the time rbus_close calls 
                  rbusHandleList_Remove and rbus_unregisterClientDisconnectHandler*/
    {
        len = rtVector_Size(gHandleList);
        for(i = 0; i < len; i++)
        {
            struct _rbusHandle* handle = (struct _rbusHandle*)rtVector_At(gHandleList, i);
            HANDLE_SUBS_MUTEX_LOCK(handle);
            if(handle->subscriptions)
            {
                /*assuming this doesn't reenter this api which could possibly deadlock*/
                rbusSubscriptions_handleClientDisconnect(handle, handle->subscriptions, clientListener);
            }
            HANDLE_SUBS_MUTEX_UNLOCK(handle);
        }

        /* Update the Direct Connection */
        rbuscore_terminatePrivateConnection(clientListener);
    }
}

struct _rbusHandle* rbusHandleList_GetByComponentID(int32_t componentId)
{
    size_t i;
    size_t len;
    struct _rbusHandle* handle = NULL;
    if(gHandleList)
    {
        len = rtVector_Size(gHandleList);
        for(i = 0; i < len; i++)
        {
            handle = (struct _rbusHandle*)rtVector_At(gHandleList, i);
            if(handle->componentId == componentId)
            {
                break;
            }
        }
    }
    return handle;
}

struct _rbusHandle* rbusHandleList_GetByName(char const* componentName)
{
    size_t i;
    size_t len;
    struct _rbusHandle* handle = NULL;
    VERIFY_NULL(componentName,return NULL);
    if(gHandleList)
    {
        len = rtVector_Size(gHandleList);
        for(i = 0; i < len; i++)
        {
            struct _rbusHandle* tmp = (struct _rbusHandle*)rtVector_At(gHandleList, i);
            if(strcmp(tmp->componentName, componentName) == 0)
            {
                handle = tmp;
                break;
            }
        }
    }
    return handle;
}

int rbusHandle_TimeoutValuesInit(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return -1)
    RBUSLOG_DEBUG("%s", __FUNCTION__);
    initInt(handle->timeoutValues.setTimeout,            RBUS_SET_DEFAULT_TIMEOUT);
    initInt(handle->timeoutValues.getTimeout,            RBUS_GET_DEFAULT_TIMEOUT);
    initInt(handle->timeoutValues.setMultiTimeout,       RBUS_SET_MULTI_DEFAULT_TIMEOUT);
    initInt(handle->timeoutValues.getMultiTimeout,       RBUS_GET_WILDCARD_DEFAULT_TIMEOUT);
    initInt(handle->timeoutValues.subscribeTimeout,      RBUS_SUBSCRIBE_TIMEOUT);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_ConfigGetTimeout(rbusHandle_t handle, uint32_t timeout)
{
    VERIFY_NULL(handle, return RBUS_ERROR_INVALID_INPUT)
    if (timeout)
        handle->timeoutValues.getTimeout = timeout;
    else
        handle->timeoutValues.getTimeout = RBUS_GET_DEFAULT_TIMEOUT;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_ConfigSetTimeout(rbusHandle_t handle, uint32_t timeout)
{
    VERIFY_NULL(handle, return RBUS_ERROR_INVALID_INPUT)
    if (timeout)
        handle->timeoutValues.setTimeout = timeout;
    else
        handle->timeoutValues.setTimeout = RBUS_SET_DEFAULT_TIMEOUT;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_ConfigGetMultiTimeout(rbusHandle_t handle, uint32_t timeout)
{
    VERIFY_NULL(handle, return RBUS_ERROR_INVALID_INPUT)
    if (timeout)
        handle->timeoutValues.getMultiTimeout = timeout;
    else
        handle->timeoutValues.getMultiTimeout = RBUS_GET_WILDCARD_DEFAULT_TIMEOUT;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_ConfigSetMultiTimeout(rbusHandle_t handle, uint32_t timeout)
{
    VERIFY_NULL(handle, return RBUS_ERROR_INVALID_INPUT)
    if (timeout)
        handle->timeoutValues.setMultiTimeout = timeout;
    else
        handle->timeoutValues.setMultiTimeout = RBUS_SET_MULTI_DEFAULT_TIMEOUT;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_ConfigSubscribeTimeout(rbusHandle_t handle, uint32_t timeout)
{
    VERIFY_NULL(handle, return RBUS_ERROR_INVALID_INPUT)
    if (timeout)
        handle->timeoutValues.subscribeTimeout = timeout;
    else
        handle->timeoutValues.subscribeTimeout = RBUS_SUBSCRIBE_TIMEOUT;
    return RBUS_ERROR_SUCCESS;
}

uint32_t rbusHandle_FetchGetTimeout(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return 0)
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};
    char fileName[BUF_LEN] = {'\0'};
    snprintf(fileName, BUF_LEN-1, "%s/rbus_%s_timeout_get", RBUS_TMP_DIRECTORY, __progname);
    fileName[BUF_LEN-1] = '\0';
    if (access(fileName, F_OK) == 0)
    {
        fp = fopen(fileName, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    return handle->timeoutValues.getTimeout;
}

uint32_t rbusHandle_FetchSetTimeout(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return 0)
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};
    char fileName[BUF_LEN] = {'\0'};
    snprintf(fileName, BUF_LEN-1, "%s/rbus_%s_timeout_set", RBUS_TMP_DIRECTORY, __progname);
    fileName[BUF_LEN-1] = '\0';
    if (access(fileName, F_OK) == 0)
    {
        fp = fopen(fileName, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    return handle->timeoutValues.setTimeout;
}

uint32_t rbusHandle_FetchGetMultiTimeout(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return 0)
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};
    char fileName[BUF_LEN] = {'\0'};
    snprintf(fileName, BUF_LEN-1, "%s/rbus_%s_timeout_get_wildcard_query", RBUS_TMP_DIRECTORY, __progname);
    fileName[BUF_LEN-1] = '\0';
    if (access(fileName, F_OK) == 0)
    {
        fp = fopen(fileName, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    return handle->timeoutValues.getMultiTimeout;
}

uint32_t rbusHandle_FetchSetMultiTimeout(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return 0)
    int timeout = 0;
    FILE *fp = NULL;
    char buf[25] = {0};
    char fileName[BUF_LEN] = {'\0'};
    snprintf(fileName, BUF_LEN-1, "%s/rbus_%s_timeout_setMulti", RBUS_TMP_DIRECTORY, __progname);
    fileName[BUF_LEN-1] = '\0';
    if (access(fileName, F_OK) == 0)
    {
        fp = fopen(fileName, "r");
        if(fp != NULL) {
            if (fread(buf, 1, sizeof(buf), fp) > 0)
                timeout = atoi(buf);
            fclose(fp);
        }
        if (timeout > 0)
            return timeout * 1000;
    }
    return handle->timeoutValues.setMultiTimeout;
}

uint32_t rbusHandle_FetchSubscribeTimeout(rbusHandle_t handle)
{
    VERIFY_NULL(handle, return 0)
    return handle->timeoutValues.subscribeTimeout;
}
