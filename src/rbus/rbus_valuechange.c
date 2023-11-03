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

/*
    Value-Change Detection:
    Simple API that allows you to add/remove parameters you wish to check value-change for.
    Uses a single thread to poll parameter values across all rbus handles.
    The thread is started on first param added and stopped on last param removed.
    A poling period (default 30 seconds) helps to limit the cpu usage.
    Runs in the provider process, so the value are got with direct callbacks and not over the network.
    The technique is simple:
    1) when a param is added, get and cache its current value.
    2) on a background thread, periodically get the latest value and compare to cached value.
    3) if the value has change, publish an event.
*/

#define _GNU_SOURCE 1 //needed for pthread_mutexattr_settype

#include "rbus_valuechange.h"
#include "rbus_config.h"
#include "rbus_handle.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <rtVector.h>
#include <rtTime.h>
#include <rtMemory.h>

#define ERROR_CHECK(CMD) \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    RBUSLOG_ERROR("Error %d:%s running command " #CMD, err, strerror(err)); \
  } \
}
#define VERIFY_NULL(T)      if(NULL == T){ return; }
#define LOCK() ERROR_CHECK(pthread_mutex_lock(&gVC->mutex))
#define UNLOCK() ERROR_CHECK(pthread_mutex_unlock(&gVC->mutex))

typedef struct ValueChangeDetector_t
{
    int              running;
    rtVector         params;
    pthread_mutex_t  mutex;
    pthread_t        thread;
    pthread_cond_t   cond;
} ValueChangeDetector_t;

typedef struct ValueChangeRecord
{
    rbusHandle_t handle;    //needed when calling rbus_getHandler and rbusEvent_Publish
    elementNode const* node;    //used to call the rbus_getHandler is contains
    rbusProperty_t property;    //the parameter with value that gets cached
} ValueChangeRecord;

ValueChangeDetector_t* gVC = NULL;

static void rbusValueChange_Init()
{
    pthread_mutexattr_t attrib;
    pthread_condattr_t cattrib;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    if(gVC)
        return;

    gVC = rt_malloc(sizeof(struct ValueChangeDetector_t));

    gVC->running = 0;
    gVC->params = NULL;

    rtVector_Create(&gVC->params);

    ERROR_CHECK(pthread_mutexattr_init(&attrib));
    ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&gVC->mutex, &attrib));

    ERROR_CHECK(pthread_condattr_init(&cattrib));
    ERROR_CHECK(pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC));
    ERROR_CHECK(pthread_cond_init(&gVC->cond, &cattrib));
    ERROR_CHECK(pthread_condattr_destroy(&cattrib));
}

static void vcParams_Free(void* p)
{
    ValueChangeRecord* rec = (ValueChangeRecord*)p;
    if(rec){
        rbusProperty_Release(rec->property);
        free(rec);
    }
}

static ValueChangeRecord* vcParams_Find(const elementNode* paramNode)
{
    size_t i;
    for(i=0; i < rtVector_Size(gVC->params); ++i)
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(gVC->params, i);
        if(rec && rec->node == paramNode)
            return rec;
    }
    return NULL;
}

static void* rbusValueChange_pollingThreadFunc(void *userData)
{
    (void)(userData);
    RBUSLOG_DEBUG("%s: start", __FUNCTION__);
    LOCK();
    while(gVC->running)
    {
        size_t i;
        int err;
        rtTime_t timeout;
        rtTimespec_t ts;

        rtTime_Later(NULL, rbusConfig_Get()->valueChangePeriod, &timeout);
        
        err = pthread_cond_timedwait(&gVC->cond, 
                                    &gVC->mutex, 
                                    rtTime_ToTimespec(&timeout, &ts));

        if(err != 0 && err != ETIMEDOUT)
        {
            RBUSLOG_ERROR("Error %d:%s running command pthread_cond_timedwait", err, strerror(err));
        }
        
        if(!gVC->running)
        {
            break;
        }

        for(i=0; i < rtVector_Size(gVC->params); ++i)
        {
            rbusProperty_t property;
            rbusValue_t newVal, oldVal;

            ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(gVC->params, i);
            if(!rec)
                continue;

            rbusProperty_Init(&property,rbusProperty_GetName(rec->property), NULL);

            rbusGetHandlerOptions_t opts;
            memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
            opts.requestingComponent = "valueChangePollThread";

            ELM_PRIVATE_LOCK(rec->node);
            int result = rec->node->cbTable.getHandler(rec->handle, property, &opts);
            ELM_PRIVATE_UNLOCK(rec->node);

            if(result != RBUS_ERROR_SUCCESS)
            {
                RBUSLOG_WARN("failed to get current value of %s", rbusProperty_GetName(property));
                continue;
            }

            char* sValue = rbusValue_ToString(rbusProperty_GetValue(property), NULL, 0);
            RBUSLOG_DEBUG("%s=%s", rbusProperty_GetName(property), sValue);
            free(sValue);

            newVal = rbusProperty_GetValue(property);
            oldVal = rbusProperty_GetValue(rec->property);

            if(rbusValue_Compare(newVal, oldVal))
            {
                rbusEvent_t event = {0};
                rbusObject_t data;
                rbusValue_t byVal = NULL;

                RBUSLOG_INFO("value change detected for %s", rbusProperty_GetName(rec->property));

                /* The "by" field is set to the component's name which made the last value change.
                   The source of a value-change could be an external component calling rbus_set or the provider internally updating
                   the value.  changeComp/changeTime are updated through the rbus_set path, but not through the provider internal path.
                   We must deduce if the provider has updated the value and reflect that change to the changeComp/changeTime, right here.
                   If we don't have a changeComp or we do but the changeTime is older then the current polling period,
                   then we know it was the provider who updated the value we are now detecting.
                */
                if(rec->node->changeComp == NULL || 
                   (rtTime_Elapsed(&rec->node->changeTime, NULL) >= rbusConfig_Get()->valueChangePeriod &&
                   strcmp(rec->handle->componentName, rec->node->changeComp) == 0))
                {
                    printf("VC detected provider-side value-change oldcomp=%s elapsed=%d period=%d\n", rec->node->changeComp, rtTime_Elapsed(&rec->node->changeTime, NULL), rbusConfig_Get()->valueChangePeriod);
                    setPropertyChangeComponent((elementNode*)rec->node, rec->handle->componentName);
                }

                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "value", newVal);
                rbusObject_SetValue(data, "oldValue", oldVal);

                rbusValue_Init(&byVal);
                rbusValue_SetString(byVal, rec->node->changeComp);
                rbusObject_SetValue(data, "by", byVal);
                rbusValue_Release(byVal);

                event.name = rbusProperty_GetName(rec->property);
                event.data = data;
                event.type = RBUS_EVENT_VALUE_CHANGED;
                result = rbusEvent_Publish(rec->handle, &event);

                rbusObject_Release(data);

                if(result != RBUS_ERROR_SUCCESS)
                {
                    RBUSLOG_WARN("Event_Publish failed with result=%d", result);
                }

                /*update the record's property with new value*/
                rbusProperty_SetValue(rec->property, rbusProperty_GetValue(property));
                rbusProperty_Release(property);
            }
            else
            {
                RBUSLOG_DEBUG("value change not detected for %s", rbusProperty_GetName(rec->property));
                rbusProperty_Release(property);
            }
        }
    }
    UNLOCK();
    RBUSLOG_DEBUG("%s: stop", __FUNCTION__);
    return NULL;
}

void rbusValueChange_AddPropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;

    if(!gVC)
    {
        rbusValueChange_Init();
    }

    /* basic sanity tests */    
    assert(propNode);
    if(!propNode)
    {
        RBUSLOG_WARN("propNode NULL error");
        return;
    }
    RBUSLOG_DEBUG(" Add Property Node %s", propNode->fullName);
    assert(propNode->type == RBUS_ELEMENT_TYPE_PROPERTY);
    if(propNode->type != RBUS_ELEMENT_TYPE_PROPERTY)
    {
        RBUSLOG_WARN("propNode type %d error", propNode->type);
        return;
    }
    assert(propNode->cbTable.getHandler);
    if(!propNode->cbTable.getHandler)
    {
        RBUSLOG_WARN("propNode getHandler NULL error");
        return;
    }

    /* only add the property if its not already in the list */

    LOCK();//############ LOCK ############

    rec = vcParams_Find(propNode);

    UNLOCK();//############ UNLOCK ############

    if(!rec)
    {
        rec = (ValueChangeRecord*)rt_malloc(sizeof(ValueChangeRecord));
        rec->handle = handle;
        rec->node = propNode;

        rbusProperty_Init(&rec->property, propNode->fullName, NULL);

        rbusGetHandlerOptions_t opts;
        memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
        opts.requestingComponent = "valueChangePollThread";
        /*get and cache the current value
          the polling thread will periodically re-get and compare to detect value changes*/
        ELM_PRIVATE_LOCK(propNode);
        int result = propNode->cbTable.getHandler(handle, rec->property, &opts);
        ELM_PRIVATE_UNLOCK(propNode);

        if(result != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_WARN("failed to get current value for %s as the node is not found", propNode->fullName);
            vcParams_Free(rec);
            rec = NULL;
            return;
        }

        char* sValue;
        RBUSLOG_DEBUG("%s=%s", propNode->fullName, (sValue = rbusValue_ToString(rbusProperty_GetValue(rec->property), NULL, 0)));
        free(sValue);

        LOCK();//############ LOCK ############

        rtVector_PushBack(gVC->params, rec);

        /* start polling thread if needed */

        if(!gVC->running)
        {
            gVC->running = 1;
            pthread_create(&gVC->thread, NULL, rbusValueChange_pollingThreadFunc, NULL);
        }

        UNLOCK();//############ UNLOCK ############
    }
}

void rbusValueChange_RemovePropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;
    bool stopThread = false;

    (void)(handle);
    VERIFY_NULL(propNode);
    RBUSLOG_DEBUG("RemovePropertyNode %s", propNode->fullName);

    if(!gVC)
    {
        return;
    }

    LOCK();//############ LOCK ############
    rec = vcParams_Find(propNode);
    if(rec)
    {
        rtVector_RemoveItem(gVC->params, rec, vcParams_Free);
        /* if there's nothing left to poll then shutdown the polling thread */
        if(gVC->running && rtVector_Size(gVC->params) == 0)
        {
            stopThread = true;
            gVC->running = 0;
        }
        else 
        {
            stopThread = false;
        }
    }
    else
    {
        RBUSLOG_WARN("value change param not found: %s", propNode->fullName);
    }
    UNLOCK();//############ UNLOCK ############
    if(stopThread)
    {
        ERROR_CHECK(pthread_cond_signal(&gVC->cond));
        ERROR_CHECK(pthread_join(gVC->thread, NULL));
    }
}

void rbusValueChange_CloseHandle(rbusHandle_t handle)
{
    RBUSLOG_DEBUG("%s", __FUNCTION__);

    if(!gVC)
    {
        return;
    }

    //remove all params for this bus handle
    LOCK();//############ LOCK ############
    size_t i = 0;
    while(i < rtVector_Size(gVC->params))
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(gVC->params, i);
        if(rec && rec->handle == handle)
        {
            rtVector_RemoveItem(gVC->params, rec, vcParams_Free);
        }
        else
        {
            //only i++ here because rtVector_RemoveItem does a right shift on all the elements after remove index
            i++; 
        }
    }

    //clean up everything once all params are removed
    //but check the size to ensure we do not clean up if params for other rbus handles exist
    if(rtVector_Size(gVC->params) == 0)
    {
        if(gVC->running)
        {
            gVC->running = 0;
            UNLOCK();//############ UNLOCK ############
            ERROR_CHECK(pthread_cond_signal(&gVC->cond));
            ERROR_CHECK(pthread_join(gVC->thread, NULL));
        }
        else
        {
            UNLOCK();//############ UNLOCK ############
        }
        ERROR_CHECK(pthread_mutex_destroy(&gVC->mutex));
        ERROR_CHECK(pthread_cond_destroy(&gVC->cond));
        rtVector_Destroy(gVC->params, NULL);
        gVC->params = NULL;
        free(gVC);
        gVC = NULL;
    }
    else
    {
        UNLOCK();//############ UNLOCK ############
    }
}

