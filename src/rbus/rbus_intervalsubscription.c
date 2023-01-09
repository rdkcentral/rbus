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

#define _GNU_SOURCE 1 //needed for pthread_mutexattr_settype
#include "rbus_intervalsubscription.h"
#include "rbus_config.h"
#include "rbus_handle.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
//#include <assert.h>
#include <errno.h>
#include <rtVector.h>
#include <rtTime.h>
#include <rtMemory.h>
#include <rbuscore.h>

#define ERROR_CHECK(CMD) \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    RBUSLOG_ERROR("Error %d:%s running command " #CMD, err, strerror(err)); \
  } \
}
#define VERIFY_NULL(T)      if(NULL == T){ return; }

rtVector gRecord = NULL;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct sRecord
{
    int running;
    pthread_mutex_t  mutex;
    pthread_t        thread;
    pthread_cond_t   cond;
    rbusHandle_t     handle;
    elementNode const* node;
    rbusProperty_t   property;
    rbusSubscription_t* sub;
} sRecord;

void rbusEventData_appendToMessage(rbusEvent_t* event, rbusFilter_t filter, int32_t interval, int32_t duration, int32_t componentId, rbusMessage msg);

static void init_thread(sRecord* sub_rec)
{
    pthread_mutexattr_t attrib;
    pthread_condattr_t cattrib;

    sub_rec->running = 0;

    ERROR_CHECK(pthread_mutexattr_init(&attrib));
    ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&sub_rec->mutex, &attrib));

    ERROR_CHECK(pthread_condattr_init(&cattrib));
    ERROR_CHECK(pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC));
    ERROR_CHECK(pthread_cond_init(&sub_rec->cond, &cattrib));
    ERROR_CHECK(pthread_condattr_destroy(&cattrib));
}

static void sub_Free(void* p)
{
    sRecord* rec = (sRecord*)p;
    if(rec)
    {
        ERROR_CHECK(pthread_mutex_destroy(&rec->mutex));
        ERROR_CHECK(pthread_cond_destroy(&rec->cond));
        rbusProperty_Release(rec->property);
        //subscriptionFree(rec->sub);
        rec->sub = NULL;
        free(rec);
    }
}

static sRecord* sub_find(rbusSubscription_t* sub)
{
    size_t i;
    for(i=0; i < rtVector_Size(gRecord); ++i)
    {
        sRecord* rec = (sRecord*)rtVector_At(gRecord, i);
        if(rec && rec->sub == sub) {
            return rec;
	}
    }
    return NULL;
}

/* Publishing event based on subscriber interval */
static void* PublishingThreadFunc(void* rec)
{
    RBUSLOG_DEBUG("\n %s: start\n", __FUNCTION__);
    struct sRecord *sub_rec = (struct sRecord*)rec;
    int count = 0;
    int duration_count = 0;
    rbusCoreError_t error;
    rbusSubscription_t* sub = sub_rec->sub;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)sub_rec->handle;
    ERROR_CHECK(pthread_mutex_lock(&sub_rec->mutex));
    while(sub_rec->running)
    {
        int err;
        rtTime_t timeout;
        rtTimespec_t ts;
        rtTime_Later(NULL, (sub->interval*1000), &timeout);
        err = pthread_cond_timedwait(&sub_rec->cond,
                &sub_rec->mutex,
                rtTime_ToTimespec(&timeout, &ts));

        if(err != 0 && err != ETIMEDOUT)
        {
            RBUSLOG_ERROR("Error %d:%s running command pthread_cond_timedwait", err, strerror(err));
        }
        
        if (sub->duration != 0)
        {
            duration_count = sub->duration/sub->interval;
            if (count >= duration_count)
            {
                break;
            }
        }

        rbusProperty_t property;
        rbusValue_t Val;

        rbusProperty_Init(&property,rbusProperty_GetName(sub_rec->property), NULL);
        rbusGetHandlerOptions_t opts;
        memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
        opts.requestingComponent = "IntervalThread";

        int result = sub_rec->node->cbTable.getHandler(sub_rec->handle, property, &opts);
        //struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
        if(result != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_ERROR("%s: failed to get value of %s", __FUNCTION__, rbusProperty_GetName(property));
            continue;
        }

        /* Get data */
        Val = rbusProperty_GetValue(property);

        rbusEvent_t event = {0};
        rbusObject_t data;

        rbusObject_Init(&data, NULL);
        rbusObject_SetValue(data, "value", Val);
        event.name = rbusProperty_GetName(sub_rec->property);
        event.data = data;
        event.type = RBUS_EVENT_INTERVAL;
 
        rbusMessage msg;
        rbusMessage_Init(&msg);

        rbusEventData_appendToMessage(&event, sub->filter, sub->interval, sub->duration, sub->componentId, msg);

        RBUSLOG_DEBUG("rbusEvent_Publish: publishing event %s to listener %s interval %d", sub->eventName, sub->listener, sub->interval);
        error = rbus_publishSubscriberEvent(
                handleInfo->componentName,
                sub->eventName/*use the same eventName the consumer subscribed with; not event instance name eventData->name*/,
                sub->listener,
                msg);

        rbusMessage_Release(msg);

        rbusObject_Release(data);

        if(error != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s: rbusEvent_Publish failed with result=%d", __FUNCTION__, result);
        }

        /*update the record's property with new value*/
        rbusProperty_SetValue(sub_rec->property, rbusProperty_GetValue(property));
        rbusProperty_Release(property);
        count++;
    }
    ERROR_CHECK(pthread_mutex_unlock(&sub_rec->mutex));
    RBUSLOG_DEBUG("%s: stop\n", __FUNCTION__);
    return NULL;
}

/* Add Subscription Record to global record and start subscription thread */
rbusError_t rbusInterval_AddSubscriptionRecord(
        rbusHandle_t handle,
        elementNode* propNode,
        rbusSubscription_t* sub)
{
    sRecord *sub_rec = NULL;
    if(!propNode)
    {
        RBUSLOG_ERROR("%s: propNode NULL error", __FUNCTION__);
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(propNode->type != RBUS_ELEMENT_TYPE_PROPERTY)
    {
        RBUSLOG_ERROR("%s: propNode type %d error", __FUNCTION__, propNode->type);
        return RBUS_ERROR_NOSUBSCRIBERS ;
    }
    
    if(!propNode->cbTable.getHandler)
    {
        RBUSLOG_ERROR("%s: as it does not have getHandler", __FUNCTION__);
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }

    ERROR_CHECK(pthread_mutex_lock(&gMutex));
    if (!gRecord)
        rtVector_Create(&gRecord);

    sub_rec = sub_find(sub);
    ERROR_CHECK(pthread_mutex_unlock(&gMutex));
    if(!sub_rec)
    {
        sub_rec = (sRecord*)rt_malloc(sizeof(sRecord));
        sub_rec->handle = handle;
        sub_rec->node = propNode;
        sub_rec->sub = sub;
        rbusProperty_Init(&sub_rec->property, propNode->fullName, NULL);
        init_thread(sub_rec);

        /* Thread create */
        sub_rec->running = 1;
        pthread_create(&sub_rec->thread, NULL, PublishingThreadFunc, (void *)sub_rec);

        rtVector_PushBack(gRecord, sub_rec);
    }
    return RBUS_ERROR_SUCCESS; 
}

/* Delete Subscription Record from global record and stop it's running thread */
void rbusInterval_RemoveSubscriptionRecord(
        rbusHandle_t handle,
        elementNode* propNode,
        rbusSubscription_t* sub)
{
    (void)(handle);
    (void)(propNode);
    VERIFY_NULL(sub);

    ERROR_CHECK(pthread_mutex_lock(&gMutex));
    if (!gRecord)
    {
        return;
    }

    sRecord* rec;
    rec = sub_find(sub);
    ERROR_CHECK(pthread_mutex_unlock(&gMutex));

    if(rec)
    {
        if(rec->running)
        {
            rec->running = 0;
            ERROR_CHECK(pthread_cond_signal(&rec->cond));
            ERROR_CHECK(pthread_join(rec->thread, NULL));
        }
        rtVector_RemoveItem(gRecord, rec, sub_Free);
    }
    else
    {
        RBUSLOG_ERROR("%s: value not found\n", __FUNCTION__);
    }
}
