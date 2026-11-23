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
#define _GNU_SOURCE 1
#include "rbus_asyncsubscribe.h"
#include "rbus_log.h"
#include "rbus_handle.h"
#include <rtTime.h>
#include <rtList.h>
#include <rtMemory.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define ERROR_CHECK(CMD) \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    RBUSLOG_ERROR("Error %d:%s running command " #CMD, err, strerror(err)); \
  } \
}
#define VERIFY_NULL(T) if(NULL == T){ return; }
#define LOCK() ERROR_CHECK(pthread_mutex_lock(&gRetrier->mutexQueue))
#define UNLOCK() ERROR_CHECK(pthread_mutex_unlock(&gRetrier->mutexQueue))


/*defined in rbus.c*/
void _subscribe_async_callback_handler(rbusEventSubscription_t* subscription, rbusError_t error);
void _subscribe_async_activate(rbusHandle_t handle, rbusEventSubscription_t* subscription, uint32_t subscriptionId);
int _event_callback_handler(char const* objectName, char const* eventName, rbusMessage message, void* userData);
void rbusEventSubscription_free(void* p);

typedef struct AsyncSubscribeRetrier_t
{
    rtList items;
    pthread_cond_t condItemAdded;
    pthread_cond_t condItemDone;
    pthread_mutex_t mutexQueue;
    int isRunning;
    pthread_t threadId;
} AsyncSubscribeRetrier_t;

typedef struct AsyncSubscription_t
{
    rbusEventSubscription_t* subscription;
    rbusHandle_t handle;
    char* eventName;
    rbusFilter_t filter;
    bool inFlight;
    bool cancelled;
    rbusMessage payload;
    int nextWaitTime;
    rtTime_t startTime;
    rtTime_t nextRetryTime;
} AsyncSubscription_t;

static AsyncSubscribeRetrier_t* gRetrier = NULL;
static pthread_mutex_t gRetrierMutex = PTHREAD_MUTEX_INITIALIZER;

static void rbusAsyncSubscribeRetrier_FreeSubscription(void *pitem)
{
    AsyncSubscription_t* sub = (AsyncSubscription_t*)pitem;
    if(!sub)
        return;
    if(sub->subscription)
        rbusEventSubscription_free(sub->subscription);
    if(sub->filter)
        rbusFilter_Release(sub->filter);
    free(sub->eventName);
    rbusMessage_Release(sub->payload);
    free(pitem);
}

static int rbusAsyncSubscribeRetrier_CompareHandle(const void *pitem, const void *phandle)
{
    AsyncSubscription_t* item = (AsyncSubscription_t*)pitem;
    if((!item)||(!phandle))
        return 1;
    if(item->handle == phandle)
        return 0;
    else
        return 1;
}

static int rbusAsyncSubscribeRetrier_CompareSubscription(const void *pitem, const void *psub)
{
    AsyncSubscription_t* item = (AsyncSubscription_t*)pitem;
    rbusEventSubscription_t* sub = (rbusEventSubscription_t*)psub;
    if((!item)||(!sub))
        return 1;

    if (!item->cancelled && (item->handle == sub->handle) &&
        strcmp(item->eventName, sub->eventName) == 0 &&
        rbusFilter_Compare(item->filter, sub->filter) == 0)
        return 0;
    else
        return 1;
}

static int rbusAsyncSubscribeRetrier_DetermineNextSendTime(rtTime_t* nextSendTime)
{
    rtTime_t now;
    rtListItem li;
    char tbuff[200];

    //find the earliest nextRetryTime using nextSendTime to compare and store
    rtTime_Now(&now);
    rtTime_Later(&now, RBUS_SUBSCRIBE_MAXWAIT + 1000, nextSendTime);

    RBUSLOG_DEBUG("now=%s", rtTime_ToString(&now, tbuff));

    rtList_GetFront(gRetrier->items, &li);

    if(!li)
    {
        return 0;
    }

    while(li)
    {
        AsyncSubscription_t* item;
        rtListItem_GetData(li, (void**)&item);
        RBUSLOG_INFO("item=%s", rtTime_ToString(&item->nextRetryTime, tbuff));

        if(rtTime_Compare(&item->nextRetryTime, nextSendTime) < 0)
        {
          *nextSendTime = item->nextRetryTime;
        }

        rtListItem_GetNext(li, &li);
     }

    //if nextSendTime is past due, return 1 so the caller knows to send requests now
    if(rtTime_Compare(nextSendTime, &now) <= 0)
    {
        RBUSLOG_INFO("subs past due");
        return 1;
    }

#if 0
    //add a second to avoid doing a cond_timedwait for less then a second
    //this will essentially batch all items due between now and this next time
    rtTime_Later(nextSendTime, 1000, nextSendTime);
#endif
    RBUSLOG_INFO("subs due in %d miliseconds nextSendTime=%s",
                rtTime_Elapsed(&now, nextSendTime), 
                rtTime_ToString(nextSendTime, tbuff));

    return 0;//meaning there's nothing past due currently
}

static void rbusAsyncSubscribeRetrier_Complete(AsyncSubscription_t* item, rbusError_t error, uint32_t subscriptionId)
{
    bool cancelled;
    LOCK();
    cancelled = item->cancelled;
    UNLOCK();
    if(!cancelled)
        _subscribe_async_callback_handler(item->subscription, error);

    HANDLE_EVENTSUBS_MUTEX_LOCK(item->handle);
    LOCK();
    cancelled = item->cancelled;
    if(error == RBUS_ERROR_SUCCESS && !cancelled)
    {
        _subscribe_async_activate(item->handle, item->subscription, subscriptionId);
        item->subscription = NULL;
    }
    UNLOCK();
    HANDLE_EVENTSUBS_MUTEX_UNLOCK(item->handle);

    if(error == RBUS_ERROR_SUCCESS && cancelled)
    {
        rbusCoreError_t err = rbus_unsubscribeFromEvent(NULL, item->eventName, item->payload, false);
        if(err != RBUSCORE_SUCCESS)
            RBUSLOG_ERROR("%s cancellation failed with core error %d", item->eventName, err);
    }
}

static bool rbusAsyncSubscribeRetrier_TrySubscribe(AsyncSubscription_t* item)
{
    rbusMessage response = NULL;
    int providerError = RBUS_ERROR_SUCCESS;
    uint32_t subscriptionId = 0;
    rtTime_t now;

    RBUSLOG_INFO("%s subscribing", item->eventName);
    rbusCoreError_t coreerr = rbus_subscribeToEventTimeout(NULL, item->eventName,
        _event_callback_handler, item->payload, item->subscription, &providerError, 0, false, &response, false);

    rtTime_Now(&now);
    uint32_t elapsed = rtTime_Elapsed(&item->startTime, &now);
    uint32_t timeout = rbusHandle_FetchSubscribeTimeout(item->handle);
    LOCK();
    bool retry = !item->cancelled && coreerr == RBUSCORE_ERROR_ENTRY_NOT_FOUND && elapsed < timeout;
    if(retry)
    {
        if(item->nextWaitTime == 0)
            item->nextWaitTime = 1000;
        else
            item->nextWaitTime *= 2;
        if(item->nextWaitTime > RBUS_SUBSCRIBE_MAXWAIT)
            item->nextWaitTime = RBUS_SUBSCRIBE_MAXWAIT;
        if(elapsed + item->nextWaitTime < timeout)
            rtTime_Later(&now, item->nextWaitTime, &item->nextRetryTime);
        else
            rtTime_Later(&item->startTime, timeout, &item->nextRetryTime);
        RBUSLOG_INFO("%s no provider. retry in %d ms with %d left", item->eventName,
            rtTime_Elapsed(&now, &item->nextRetryTime), timeout - elapsed);
    }
    UNLOCK();

    if(!retry)
    {
        rbusError_t error;
        if(coreerr == RBUSCORE_SUCCESS)
        {
            if(response)
                rbusMessage_GetUInt32(response, &subscriptionId);
            RBUSLOG_INFO("%s subscribe retries succeeded", item->eventName);
            error = RBUS_ERROR_SUCCESS;
        }
        else if(coreerr == RBUSCORE_ERROR_ENTRY_NOT_FOUND)
        {
            RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_NO_PROVIDER_COMPONENT  %s", item->eventName);
            error = RBUS_ERROR_TIMEOUT;
        }
        else if(providerError != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_INVALID_INPUT  %s", item->eventName);
            error = providerError;
        }
        else
        {
            RBUSLOG_INFO("%s subscribe retries failed due to core error %d", item->eventName, coreerr);
            error = RBUS_ERROR_BUS_ERROR;
        }
        rbusAsyncSubscribeRetrier_Complete(item, error, subscriptionId);
    }
    if(response)
        rbusMessage_Release(response);
    return !retry;
}

static void rbusAsyncSubscribeRetrier_SendSubscriptionRequests()
{
    for(;;)
    {
        rtListItem li;
        AsyncSubscription_t* item = NULL;
        rtTime_t now;
        rtTime_Now(&now);
        LOCK();
        if(gRetrier->isRunning)
        {
            for(rtList_GetFront(gRetrier->items, &li); li; rtListItem_GetNext(li, &li))
            {
                AsyncSubscription_t* candidate;
                rtListItem_GetData(li, (void**)&candidate);
                if(!candidate->inFlight && rtTime_Compare(&candidate->nextRetryTime, &now) <= 0)
                {
                    item = candidate;
                    item->inFlight = true;
                    break;
                }
            }
        }
        UNLOCK();
        if(!item)
            break;

        rbusHandle_t handle = item->handle;
        bool retained = rbusEvent_HandleRetain(handle);
        bool done = !retained || rbusAsyncSubscribeRetrier_TrySubscribe(item);
        LOCK();
        if(done || item->cancelled)
            rtList_RemoveItem(gRetrier->items, li, rbusAsyncSubscribeRetrier_FreeSubscription);
        else
            item->inFlight = false;
        ERROR_CHECK(pthread_cond_broadcast(&gRetrier->condItemDone));
        UNLOCK();
        if(retained)
            rbusEvent_HandleRelease(handle);
    }
}

static void* AsyncSubscribeRetrier_threadFunc(void* data)
{
    (void)data;
    LOCK();
    while(gRetrier->isRunning)
    {
        rtTime_t nextSendTime;

        while(rbusAsyncSubscribeRetrier_DetermineNextSendTime(&nextSendTime))
        {
            UNLOCK();
            rbusAsyncSubscribeRetrier_SendSubscriptionRequests();
            LOCK();
        }

        if(gRetrier->isRunning)
        {
            char tbuff[200];
            rtTimespec_t ts;
            int err;

            RBUSLOG_DEBUG("timedwait until %s", rtTime_ToString(&nextSendTime, tbuff));
 
            err = pthread_cond_timedwait(&gRetrier->condItemAdded,
                                         &gRetrier->mutexQueue,
                                         rtTime_ToTimespec(&nextSendTime, &ts));
            if(err != 0 && err != ETIMEDOUT)
            {
                RBUSLOG_ERROR("Error %d:%s running command pthread_cond_timedwait", err, strerror(err));
            }

            RBUSLOG_DEBUG("waked up");
            //either we timed out or a new subscription was added
            //in either case, loop back to top and things will get handled properly
        }
    }
    UNLOCK();
    return NULL;
}

static void rbusAsyncSubscribeRetrier_Create()
{
    pthread_mutexattr_t mattrib;
    pthread_condattr_t cattrib;

    RBUSLOG_DEBUG("%s enter", __FUNCTION__);

    gRetrier = rt_malloc(sizeof(struct AsyncSubscribeRetrier_t));

    gRetrier->isRunning = true;
    rtList_Create(&gRetrier->items);

    ERROR_CHECK(pthread_mutexattr_init(&mattrib));
    ERROR_CHECK(pthread_mutexattr_settype(&mattrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&gRetrier->mutexQueue, &mattrib));  
    ERROR_CHECK(pthread_mutexattr_destroy(&mattrib));

    ERROR_CHECK(pthread_condattr_init(&cattrib));
    ERROR_CHECK(pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC));
    ERROR_CHECK(pthread_cond_init(&gRetrier->condItemAdded, &cattrib));
    ERROR_CHECK(pthread_cond_init(&gRetrier->condItemDone, &cattrib));
    ERROR_CHECK(pthread_condattr_destroy(&cattrib));

    ERROR_CHECK(pthread_create(&gRetrier->threadId, NULL, AsyncSubscribeRetrier_threadFunc, NULL));

    RBUSLOG_DEBUG("%s exit", __FUNCTION__);
}

static rbusError_t rbusAsyncSubscribeRetrier_Destroy()
{
    RBUSLOG_DEBUG("%s enter", __FUNCTION__);

    LOCK();
    gRetrier->isRunning = false;
    UNLOCK();

    //wake up worker thread so it can exit and join
    ERROR_CHECK(pthread_cond_signal(&gRetrier->condItemAdded));
    int joinErr = pthread_join(gRetrier->threadId, NULL);
    if(joinErr != 0)
    {
        RBUSLOG_ERROR("Joining the async retry worker failed: %s", strerror(joinErr));
        return RBUS_ERROR_BUS_ERROR;
    }

    ERROR_CHECK(pthread_mutex_destroy(&gRetrier->mutexQueue));
    ERROR_CHECK(pthread_cond_destroy(&gRetrier->condItemAdded));
    ERROR_CHECK(pthread_cond_destroy(&gRetrier->condItemDone));
    rtList_Destroy(gRetrier->items, rbusAsyncSubscribeRetrier_FreeSubscription);

    free(gRetrier);
    gRetrier = NULL;

    RBUSLOG_DEBUG("%s exit", __FUNCTION__);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusAsyncSubscribe_Destroy(void)
{
    ERROR_CHECK(pthread_mutex_lock(&gRetrierMutex));
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    if(gRetrier)
        ret = rbusAsyncSubscribeRetrier_Destroy();
    ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
    return ret;
}

void rbusAsyncSubscribe_AddSubscription(rbusEventSubscription_t* subscription, rbusMessage payload)
{
    int rc;
    char tbuff[50];
    VERIFY_NULL(subscription);

    ERROR_CHECK(pthread_mutex_lock(&gRetrierMutex));
    if(!gRetrier)
    {
        rbusAsyncSubscribeRetrier_Create();
    }

    AsyncSubscription_t* item = rt_malloc(sizeof(struct AsyncSubscription_t));

    rbusMessage_Retain(payload);

    item->subscription = subscription;
    item->handle = subscription->handle;
    item->eventName = rt_malloc(strlen(subscription->eventName) + 1);
    strcpy(item->eventName, subscription->eventName);
    item->filter = subscription->filter;
    if(item->filter)
        rbusFilter_Retain(item->filter);
    item->inFlight = false;
    item->cancelled = false;
    item->payload = payload;
    item->nextWaitTime = 0;

    rtTime_Now(&item->startTime);
    item->nextRetryTime = item->startTime; /*set to now also so we do our first sub immediately*/

    RBUSLOG_INFO("%s %s", subscription->eventName, rtTime_ToString(&item->startTime, tbuff));

    LOCK();
    rtList_PushBack(gRetrier->items, item, NULL);
    UNLOCK();

    //wake up worker thread so it can process new item
    rc = pthread_cond_signal(&gRetrier->condItemAdded);
    if(rc != 0)
        RBUSLOG_ERROR("Signalling the async retry worker failed: %s", strerror(rc));
    ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
}

bool rbusAsyncSubscribe_RemoveSubscription(rbusEventSubscription_t* subscription)
{
    bool sub_removed = false;
    if(subscription == NULL)
        return false;
    ERROR_CHECK(pthread_mutex_lock(&gRetrierMutex));
    if(!gRetrier)
    {
        ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
        return false;
    }
    LOCK();
    rtListItem li = rtList_Find(gRetrier->items, subscription, rbusAsyncSubscribeRetrier_CompareSubscription);
    if(li)
    {
        AsyncSubscription_t* item;
        rtListItem_GetData(li, (void**)&item);
        if(item->inFlight)
            item->cancelled = true;
        else
            rtList_RemoveItem(gRetrier->items, li, rbusAsyncSubscribeRetrier_FreeSubscription);
        sub_removed = true;
    }
    UNLOCK();
    ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
    return sub_removed;
}

bool rbusAsyncSubscribe_GetSubscription(rbusHandle_t handle, char const* eventName, rbusFilter_t filter)
{
    rbusEventSubscription_t sub = {0};
    bool sub_exist = false;

    ERROR_CHECK(pthread_mutex_lock(&gRetrierMutex));
    if(!gRetrier)
    {
        ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
        return sub_exist;
    }
    sub.handle = handle;
    sub.eventName = eventName;
    sub.filter = filter;
    LOCK();
    if(rtList_Find(gRetrier->items, &sub, rbusAsyncSubscribeRetrier_CompareSubscription) != NULL)
    {
        sub_exist = true;
    }
    else
    {
        sub_exist = false;
    }
    UNLOCK();
    ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
    return sub_exist;
}

rbusError_t rbusAsyncSubscribe_CloseHandle(rbusHandle_t handle)
{
    ERROR_CHECK(pthread_mutex_lock(&gRetrierMutex));
    if(!gRetrier)
    {
        ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
        return RBUS_ERROR_SUCCESS;
    }

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    LOCK();
    /* The closing handle stays registered until this wait finishes. */
    ERROR_CHECK(pthread_mutex_unlock(&gRetrierMutex));
    for(;;)
    {
        rtListItem li, next;
        bool pending = false;
        for(rtList_GetFront(gRetrier->items, &li); li; li = next)
        {
            AsyncSubscription_t* item;
            rtListItem_GetNext(li, &next);
            rtListItem_GetData(li, (void**)&item);
            if(rbusAsyncSubscribeRetrier_CompareHandle(item, handle) == 0)
            {
                item->cancelled = true;
                if(item->inFlight)
                    pending = true;
                else
                    rtList_RemoveItem(gRetrier->items, li, rbusAsyncSubscribeRetrier_FreeSubscription);
            }
        }
        if(!pending)
            break;
        int waitErr = pthread_cond_wait(&gRetrier->condItemDone, &gRetrier->mutexQueue);
        if(waitErr != 0)
        {
            UNLOCK();
            RBUSLOG_ERROR("Waiting for async subscription completion failed: %s", strerror(waitErr));
            return RBUS_ERROR_BUS_ERROR;
        }
    }
    UNLOCK();
    return RBUS_ERROR_SUCCESS;
}
