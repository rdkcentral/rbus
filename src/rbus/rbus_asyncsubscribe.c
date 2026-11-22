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
void _subscribe_async_callback_handler(rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error, uint32_t subscriptionId);
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
    rbusMessage payload;
    int nextWaitTime;
    rtTime_t startTime;
    rtTime_t nextRetryTime;
    int inFlight;  /* worker is using this item without holding mutexQueue */
    int cancelled; /* removed while in flight; the worker frees it */
} AsyncSubscription_t;

static AsyncSubscribeRetrier_t* gRetrier = NULL;

static void rbusAsyncSubscribeRetrier_FreeSubscription(void *pitem)
{
    AsyncSubscription_t* sub = (AsyncSubscription_t*)pitem;
    if(!sub)
        return;
    if(sub->subscription)
        rbusEventSubscription_free(sub->subscription);
    rbusMessage_Release(sub->payload);
    free(pitem);
}

static int rbusAsyncSubscribeRetrier_CompareHandle(const void *pitem, const void *phandle)
{
    AsyncSubscription_t* item = (AsyncSubscription_t*)pitem;
    if((!item)||(!phandle))
        return 1;
    if(item->subscription->handle == phandle)
        return 0;
    else
        return 1;
}

static int rbusAsyncSubscribeRetrier_CompareSubscription(const void *pitem, const void *psub)
{
    AsyncSubscription_t* item = (AsyncSubscription_t*)pitem;
    rbusEventSubscription_t* sub = (rbusEventSubscription_t*)psub;
    if((!item)||(!sub)||item->cancelled)
        return 1;

    if (item->subscription && (item->subscription->handle == sub->handle) &&
        strcmp(item->subscription->eventName, sub->eventName) == 0 && 
        rbusFilter_Compare(item->subscription->filter, sub->filter) == 0)
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

/* Sends one subscribe request for item. Called without mutexQueue held;
   item->inFlight keeps other threads from freeing it.
   Returns 1 when the item is finished (success or final failure), 0 to retry. */
static int rbusAsyncSubscribeRetrier_TrySubscribe(AsyncSubscription_t* item, rbusError_t* responseErr, uint32_t* subscriptionId)
{
    rbusCoreError_t coreerr;
    uint32_t elapsed;
    int providerError;
    rbusMessage response = NULL;
    rtTime_t now;

    RBUSLOG_INFO("%s subscribing", item->subscription->eventName);

    coreerr = rbus_subscribeToEventTimeout(NULL, item->subscription->eventName,
                _event_callback_handler, item->payload, item->subscription, &providerError, 0, false, &response, false);

    rtTime_Now(&now);

    elapsed = rtTime_Elapsed(&item->startTime, &now);

    if(coreerr == RBUSCORE_ERROR_ENTRY_NOT_FOUND &&  /*the only error that means provider not found yet*/
     elapsed < rbusHandle_FetchSubscribeTimeout(item->subscription->handle))    /*if we haven't timeout out yet*/
    {
        if(item->nextWaitTime == 0)
            item->nextWaitTime = 1000; //miliseconds
        else
            item->nextWaitTime *= 2;//just double the time

        //apply a limit to our doubling
        if(item->nextWaitTime > RBUS_SUBSCRIBE_MAXWAIT)
          item->nextWaitTime = RBUS_SUBSCRIBE_MAXWAIT;

        //update nextRetryTime to nextWaitTime miliseconds from now, without exceeding subscribeTimeout
        if(elapsed + item->nextWaitTime < rbusHandle_FetchSubscribeTimeout(item->subscription->handle))
        {
            rtTime_Later(&now, item->nextWaitTime, &item->nextRetryTime);
        }
        else
        {
            //its possible to have the odd situation, based on how subscribeTimeout/subscribeMaxWait are configured,
            //where this final retry happens very close to the previous retry (e.g. ... wait 60, sub, wait 60, sub, wait 1, sub)
            rtTime_Later(&item->startTime, rbusHandle_FetchSubscribeTimeout(item->subscription->handle), &item->nextRetryTime);
        }

        RBUSLOG_INFO("%s no provider. retry in %d ms with %d left",
            item->subscription->eventName,
            rtTime_Elapsed(&now, &item->nextRetryTime),
            rbusHandle_FetchSubscribeTimeout(item->subscription->handle)- elapsed );
        return 0;
    }

    if(coreerr == RBUSCORE_SUCCESS)
    {
        if(response)
            rbusMessage_GetUInt32(response, subscriptionId);
        RBUSLOG_INFO("%s subscribe retries succeeded", item->subscription->eventName);
        *responseErr = RBUS_ERROR_SUCCESS;
    }
    else if(coreerr == RBUSCORE_ERROR_ENTRY_NOT_FOUND)
    {
        RBUSLOG_INFO("%s all subscribe retries failed and no provider found", item->subscription->eventName);
        RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_NO_PROVIDER_COMPONENT  %s", item->subscription->eventName);/*RDKB-33658-AC7*/
        *responseErr = RBUS_ERROR_TIMEOUT;
    }
    else if(providerError != RBUS_ERROR_SUCCESS)
    {
        RBUSLOG_INFO("%s subscribe retries failed due provider error %d", item->subscription->eventName, providerError);
        RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_INVALID_INPUT  %s", item->subscription->eventName);/*RDKB-33658-AC9*/
        *responseErr = providerError;
    }
    else
    {
        RBUSLOG_INFO("%s subscribe retries failed due to core error %d", item->subscription->eventName, coreerr);
        *responseErr = RBUS_ERROR_BUS_ERROR;
    }
    if(response)
        rbusMessage_Release(response);
    return 1;
}

static void rbusAsyncSubscribeRetrier_SendSubscriptionRequests()
{
    rtListItem li;
    rtTime_t start;

    RBUSLOG_DEBUG("%s enter", __FUNCTION__);

    rtTime_Now(&start);

    LOCK();
    rtList_GetFront(gRetrier->items, &li);
    while(li)
    {
        AsyncSubscription_t* item = NULL;
        rbusError_t responseErr = RBUS_ERROR_SUCCESS;
        uint32_t subscriptionId = 0;
        int done;

        rtListItem_GetData(li, (void**)&item);
        if(!item || item->cancelled || rtTime_Compare(&item->nextRetryTime, &start) > 0)
        {
            rtListItem_GetNext(li, &li);
            continue;
        }

        item->inFlight = 1;
        UNLOCK();
        done = rbusAsyncSubscribeRetrier_TrySubscribe(item, &responseErr, &subscriptionId);
        LOCK();

        if(done && !item->cancelled)
        {
            /* item stays in the list and in flight during the callback, as before */
            rbusEventSubscription_t* subscription = item->subscription;
            UNLOCK();
            _subscribe_async_callback_handler(subscription->handle, subscription, responseErr, subscriptionId);
            LOCK();
            item->subscription = NULL;/*ownership passed to the callback handler*/
        }

        item->inFlight = 0;
        ERROR_CHECK(pthread_cond_broadcast(&gRetrier->condItemDone));

        if(done || item->cancelled)
            rtList_RemoveItem(gRetrier->items, li, rbusAsyncSubscribeRetrier_FreeSubscription);

        /* li may be stale after unlocking, so restart from the front. Items already
           tried have nextRetryTime > start or were removed, so each due item is
           processed at most once per call. */
        rtList_GetFront(gRetrier->items, &li);
    }
    UNLOCK();
    RBUSLOG_DEBUG("%s exit", __FUNCTION__);
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

static void rbusAsyncSubscribeRetrier_Destroy()
{
    RBUSLOG_DEBUG("%s enter", __FUNCTION__);

    LOCK();
    gRetrier->isRunning = false;
    UNLOCK();

    //wake up worker thread so it can exit and join
    ERROR_CHECK(pthread_cond_signal(&gRetrier->condItemAdded));
    ERROR_CHECK(pthread_join(gRetrier->threadId, NULL));

    ERROR_CHECK(pthread_mutex_destroy(&gRetrier->mutexQueue));
    ERROR_CHECK(pthread_cond_destroy(&gRetrier->condItemAdded));
    ERROR_CHECK(pthread_cond_destroy(&gRetrier->condItemDone));
    rtList_Destroy(gRetrier->items, rbusAsyncSubscribeRetrier_FreeSubscription);

    free(gRetrier);
    gRetrier = NULL;

    RBUSLOG_DEBUG("%s exit", __FUNCTION__);
}

void rbusAsyncSubscribe_AddSubscription(rbusEventSubscription_t* subscription, rbusMessage payload)
{
    int rc;
    char tbuff[50];
    VERIFY_NULL(subscription);

    if(!gRetrier)
    {
        rbusAsyncSubscribeRetrier_Create();
    }

    AsyncSubscription_t* item = rt_malloc(sizeof(struct AsyncSubscription_t));

    rbusMessage_Retain(payload);

    item->subscription = subscription;
    item->payload = payload;
    item->nextWaitTime = 0;
    item->inFlight = 0;
    item->cancelled = 0;

    rtTime_Now(&item->startTime);
    item->nextRetryTime = item->startTime; /*set to now also so we do our first sub immediately*/

    RBUSLOG_INFO("%s %s", subscription->eventName, rtTime_ToString(&item->startTime, tbuff));

    LOCK();
    rtList_PushBack(gRetrier->items, item, NULL);
    UNLOCK();

    //wake up worker thread so it can process new item
    rc = pthread_cond_signal(&gRetrier->condItemAdded);
    (void)rc;
}

bool rbusAsyncSubscribe_RemoveSubscription(rbusEventSubscription_t* subscription)
{
    AsyncSubscription_t* item;
    bool sub_removed = false;
    if(!gRetrier || subscription == NULL)
        return false;
    LOCK();
    item = rtList_Find(gRetrier->items, subscription, rbusAsyncSubscribeRetrier_CompareSubscription);
    if(item)
    {
        if(item->inFlight)
            item->cancelled = 1;/*the worker frees it when its request completes*/
        else
            rtList_RemoveItemWithData(gRetrier->items, item, rbusAsyncSubscribeRetrier_FreeSubscription);
        sub_removed = true;
    }
    UNLOCK();
    return sub_removed;
}

bool rbusAsyncSubscribe_GetSubscription(rbusHandle_t handle, char const* eventName, rbusFilter_t filter)
{
    rbusEventSubscription_t sub = {0};
    bool sub_exist = false;

    if(!gRetrier)
        return sub_exist;
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
    return sub_exist;
}

void rbusAsyncSubscribe_CloseHandle(rbusHandle_t handle)
{
    size_t size1, size;

    if(!gRetrier)
        return;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    LOCK();

    rtList_GetSize(gRetrier->items, &size1);

    /* the worker may be using one of this handle's items without the lock;
       cancel it and wait until the worker is done before the handle goes away */
    for(;;)
    {
        rtListItem li;
        int busy = 0;
        rtList_GetFront(gRetrier->items, &li);
        while(li)
        {
            AsyncSubscription_t* item = NULL;
            rtListItem_GetData(li, (void**)&item);
            if(item && item->inFlight && item->subscription && item->subscription->handle == handle)
            {
                item->cancelled = 1;
                busy = 1;
            }
            rtListItem_GetNext(li, &li);
        }
        if(!busy)
            break;
        ERROR_CHECK(pthread_cond_wait(&gRetrier->condItemDone, &gRetrier->mutexQueue));
    }

    //remove all items with this handle
    rtList_RemoveItemByCompare(gRetrier->items, handle, 
                             rbusAsyncSubscribeRetrier_CompareHandle, rbusAsyncSubscribeRetrier_FreeSubscription);

    //if list is empty, we can destruct
    rtList_GetSize(gRetrier->items, &size);

    UNLOCK();

    RBUSLOG_INFO("removed %d pending async subscriptions.  %d subs left in list.", (int)(size1-size), (int)size);

    if(size == 0)
    {
        RBUSLOG_INFO("all handles removed");
        rbusAsyncSubscribeRetrier_Destroy();
    }
}
