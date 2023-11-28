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

static rtVector gHandleList = NULL;

#define VERIFY_NULL(T,R) if(NULL == T){ RBUSLOG_ERROR(#T" is NULL"); R; }

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
