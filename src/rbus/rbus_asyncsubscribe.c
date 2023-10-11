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
#include "rbus_config.h"
#include "rbus_log.h"
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
void _subscribe_async_callback_handler(rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error);
int _event_callback_handler(char const* objectName, char const* eventName, rbusMessage message, void* userData);
void rbusEventSubscription_free(void* p);

typedef struct AsyncSubscribeRetrier_t
{
    rtList items;
    pthread_cond_t condItemAdded;
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
    if((!item)||(!sub))
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

    rtList_GetFront(gRetrier->items, &li);

    //find the earliest nextRetryTime using nextSendTime to compare and store
    rtTime_Now(&now);
    rtTime_Later(&now, rbusConfig_Get()->subscribeMaxWait + 1000, nextSendTime);

    RBUSLOG_DEBUG("%s now=%s", __FUNCTION__, rtTime_ToString(&now, tbuff));

    if(!li)
    {
        return 0;
    }

    while(li)
    {
        AsyncSubscription_t* item;
        rtListItem_GetData(li, (void**)&item);

        RBUSLOG_INFO("%s item=%s", __FUNCTION__, rtTime_ToString(&item->nextRetryTime, tbuff));

        if(rtTime_Compare(&item->nextRetryTime, nextSendTime) < 0)
        {
          *nextSendTime = item->nextRetryTime;
        }

        rtListItem_GetNext(li, &li);
     }

    //if nextSendTime is past due, return 1 so the caller knows to send requests now
    if(rtTime_Compare(nextSendTime, &now) <= 0)
    {
        RBUSLOG_INFO("%s subs past due", __FUNCTION__);
        return 1;
    }

#if 0
    //add a second to avoid doing a cond_timedwait for less then a second
    //this will essentially batch all items due between now and this next time
    rtTime_Later(nextSendTime, 1000, nextSendTime);
#endif
    RBUSLOG_INFO("%s subs due in %d miliseconds nextSendTime=%s", __FUNCTION__,
                rtTime_Elapsed(&now, nextSendTime), 
                rtTime_ToString(nextSendTime, tbuff));

    return 0;//meaning there's nothing past due currently
}

static void rbusAsyncSubscribeRetrier_SendSubscriptionRequests()
{
    rtListItem li;
    rtTime_t now;

    RBUSLOG_DEBUG("%s enter", __FUNCTION__);

    rtTime_Now(&now);

    rtList_GetFront(gRetrier->items, &li);

    while(li)
    {
        AsyncSubscription_t* item = NULL;

        rtListItem_GetData(li, (void**)&item);

        if(rtTime_Compare(&item->nextRetryTime, &now) <= 0)
        {
            rbusCoreError_t coreerr;
            int elapsed;
            int providerError;

            RBUSLOG_INFO("%s: %s subscribing", __FUNCTION__, item->subscription->eventName);

            coreerr = rbus_subscribeToEvent(NULL, item->subscription->eventName, 
                        _event_callback_handler, item->payload, item->subscription, &providerError);

            rtTime_Now(&now);

            elapsed = rtTime_Elapsed(&item->startTime, &now);

            if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE &&  /*the only error that means provider not found yet*/
             elapsed < rbusConfig_Get()->subscribeTimeout)    /*if we haven't timeout out yet*/
            {
                if(item->nextWaitTime == 0)
                    item->nextWaitTime = 1000; //miliseconds
                else
                    item->nextWaitTime *= 2;//just double the time

                //apply a limit to our doubling
                if(item->nextWaitTime > rbusConfig_Get()->subscribeMaxWait)
                  item->nextWaitTime = rbusConfig_Get()->subscribeMaxWait;

                //update nextRetryTime to nextWaitTime miliseconds from now, without exceeding subscribeTimeout
                if(elapsed + item->nextWaitTime < rbusConfig_Get()->subscribeTimeout)
                {
                    rtTime_Later(&now, item->nextWaitTime, &item->nextRetryTime);
                }
                else
                {
                    //its possible to have the odd situation, based on how subscribeTimeout/subscribeMaxWait are configured, 
                    //where this final retry happens very close to the previous retry (e.g. ... wait 60, sub, wait 60, sub, wait 1, sub)
                    rtTime_Later(&item->startTime, rbusConfig_Get()->subscribeTimeout, &item->nextRetryTime);
                }

                RBUSLOG_INFO("%s: %s no provider. retry in %d ms with %d left", 
                    __FUNCTION__, 
                    item->subscription->eventName, 
                    rtTime_Elapsed(&now, &item->nextRetryTime), 
                    rbusConfig_Get()->subscribeTimeout - elapsed );
            }
            else
            {
                rbusError_t responseErr;
                rtListItem tmp;

                if(coreerr == RBUSCORE_SUCCESS)
                {
                    RBUSLOG_INFO("%s: %s subscribe retries succeeded", __FUNCTION__, item->subscription->eventName);
                    responseErr = RBUS_ERROR_SUCCESS;
                }
                else
                {
                    if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE)
                    {
                        RBUSLOG_INFO("%s: %s all subscribe retries failed and no provider found", __FUNCTION__, item->subscription->eventName);
                        RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_NO_PROVIDER_COMPONENT  %s", item->subscription->eventName);/*RDKB-33658-AC7*/
                        responseErr = RBUS_ERROR_TIMEOUT;
                    }
                    else if(providerError != RBUS_ERROR_SUCCESS)
                    {
                        RBUSLOG_INFO("%s: %s subscribe retries failed due provider error %d", __FUNCTION__, item->subscription->eventName, providerError);
                        RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_INVALID_INPUT  %s", item->subscription->eventName);/*RDKB-33658-AC9*/
                        responseErr = providerError;
                    }
                    else
                    {  
                        RBUSLOG_INFO("%s: %s subscribe retries failed due to core error %d", __FUNCTION__, item->subscription->eventName, coreerr);
                        responseErr = RBUS_ERROR_BUS_ERROR;
                    }
                }

                _subscribe_async_callback_handler(item->subscription->handle, item->subscription, responseErr);
                //store the next item, because we are removing this li item from list
                LOCK();
                item->subscription = NULL;
                rtListItem_GetNext(li, &tmp); 
                rtList_RemoveItem(gRetrier->items, li, rbusAsyncSubscribeRetrier_FreeSubscription);
                UNLOCK();
                li = tmp;
                continue;//li is already the next item so avoid GetNext below
            }
        }
        rtListItem_GetNext(li, &li);    
    }
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

            RBUSLOG_DEBUG("%s timedwait until %s", __FUNCTION__, rtTime_ToString(&nextSendTime, tbuff));
 
            err = pthread_cond_timedwait(&gRetrier->condItemAdded,
                                         &gRetrier->mutexQueue,
                                         rtTime_ToTimespec(&nextSendTime, &ts));
            if(err != 0 && err != ETIMEDOUT)
            {
                RBUSLOG_ERROR("Error %d:%s running command pthread_cond_timedwait", err, strerror(err));
            }

            RBUSLOG_DEBUG("%s waked up", __FUNCTION__);
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

    RBUSLOG_INFO("%s enter", __FUNCTION__);

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
    ERROR_CHECK(pthread_condattr_destroy(&cattrib));

    ERROR_CHECK(pthread_create(&gRetrier->threadId, NULL, AsyncSubscribeRetrier_threadFunc, NULL));

    RBUSLOG_INFO("%s exit", __FUNCTION__);
}

static void rbusAsyncSubscribeRetrier_Destroy()
{
    RBUSLOG_INFO("%s enter", __FUNCTION__);

    LOCK();
    gRetrier->isRunning = false;
    UNLOCK();

    //wake up worker thread so it can exit and join
    ERROR_CHECK(pthread_cond_signal(&gRetrier->condItemAdded));
    ERROR_CHECK(pthread_join(gRetrier->threadId, NULL));

    ERROR_CHECK(pthread_mutex_destroy(&gRetrier->mutexQueue));
    ERROR_CHECK(pthread_cond_destroy(&gRetrier->condItemAdded));
    rtList_Destroy(gRetrier->items, rbusAsyncSubscribeRetrier_FreeSubscription);

    free(gRetrier);
    gRetrier = NULL;

    RBUSLOG_INFO("%s exit", __FUNCTION__);
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

    rtTime_Now(&item->startTime);
    item->nextRetryTime = item->startTime; /*set to now also so we do our first sub immediately*/

    RBUSLOG_INFO("%s %s %s", __FUNCTION__, subscription->eventName, rtTime_ToString(&item->startTime, tbuff));

    LOCK();
    rtList_PushBack(gRetrier->items, item, NULL);
    UNLOCK();

    //wake up worker thread so it can process new item
    rc = pthread_cond_signal(&gRetrier->condItemAdded);
    (void)rc;
}

bool rbusAsyncSubscribe_RemoveSubscription(rbusEventSubscription_t* subscription)
{
    rtError err = RT_OK;
    bool sub_removed = false;
    if(!gRetrier || subscription == NULL)
        return false;
    LOCK();
    err = rtList_RemoveItemByCompare(gRetrier->items, subscription,
        rbusAsyncSubscribeRetrier_CompareSubscription, rbusAsyncSubscribeRetrier_FreeSubscription);
    if(err == RT_OK)
        sub_removed = true;
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

    RBUSLOG_INFO("%s", __FUNCTION__);

    LOCK();

    rtList_GetSize(gRetrier->items, &size1);

    //remove all items with this handle
    rtList_RemoveItemByCompare(gRetrier->items, handle, 
                             rbusAsyncSubscribeRetrier_CompareHandle, rbusAsyncSubscribeRetrier_FreeSubscription);

    //if list is empty, we can destruct
    rtList_GetSize(gRetrier->items, &size);

    UNLOCK();

    RBUSLOG_INFO("removed %d pending async subscriptions.  %d subs left in list.", (int)(size1-size), (int)size);

    if(size == 0)
    {
        RBUSLOG_INFO("%s all handles removed", __FUNCTION__);
        rbusAsyncSubscribeRetrier_Destroy();
    }
}
