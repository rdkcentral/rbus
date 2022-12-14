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

typedef struct ValueChangeRecord
{
    elementNode const* node;  // used to call the rbus_getHandler is contains
    rbusProperty_t property;  // the parameter with value that gets cached
} ValueChangeRecord;

void rbusValueChange_Init(rbusValueChangeDetector_t* d)
{
    pthread_mutexattr_t attrib;
    pthread_condattr_t cattrib;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    d->running = false;
    d->params = NULL;

    rtVector_Create(&d->params);

    ERROR_CHECK(pthread_mutexattr_init(&attrib));
    ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&d->mutex, &attrib));

    ERROR_CHECK(pthread_condattr_init(&cattrib));
    ERROR_CHECK(pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC));
    ERROR_CHECK(pthread_cond_init(&d->cond, &cattrib));
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

static ValueChangeRecord* vcParams_Find(rbusValueChangeDetector_t *d, const elementNode* paramNode)
{
    size_t i;
    for (i = 0; i < rtVector_Size(d->params); ++i)
    {
        ValueChangeRecord* rec = (ValueChangeRecord*)rtVector_At(d->params, i);
        if(rec && rec->node == paramNode)
            return rec;
    }
    return NULL;
}

static void rbusValueChange_Run(void* argp);

static void* rbusValueChange_pollingThreadFunc(void *userData)
{
    rbusHandle_t rbus = (rbusHandle_t) userData;
    rbusValueChangeDetector_t* d = &rbus->valueChangeDetector;

    ERROR_CHECK( pthread_mutex_lock(&d->mutex) );

    while (d->running)
    {
        int err;
        rtTime_t timeout;
        rtTimespec_t ts;

        rtTime_Later(NULL, rbusConfig_Get()->valueChangePeriod, &timeout);
        
        err = pthread_cond_timedwait(&d->cond, &d->mutex, rtTime_ToTimespec(&timeout, &ts));

        if(err != 0 && err != ETIMEDOUT)
        {
            RBUSLOG_ERROR("Error %d:%s running command pthread_cond_timedwait", err, strerror(err));
        }
        
        if (!d->running)
        {
            break;
        }

        if (rbus->useEventLoop)
        {
          rbusRunnable_t r;
          r.argp = rbus;
          r.exec = &rbusValueChange_Run;
          r.cleanup = NULL;
          r.next = NULL;

          RBUSLOG_INFO("rbusValueChange_pollingThreadFunc queueing get");
          rbusRunnableQueue_PushBack(&rbus->eventQueue, r);
        }
        else
        {
          RBUSLOG_INFO("rbusValueChange_pollingThreadFunc direct get");
          rbusValueChange_Run(rbus);
        }
    }

    ERROR_CHECK( pthread_mutex_unlock(&d->mutex) );

    return NULL;
}

void rbusValueChange_Run(void* argp)
{
  size_t i;

  rbusHandle_t rbus = (rbusHandle_t) argp;
  rbusValueChangeDetector_t* d = &rbus->valueChangeDetector;

  for (i = 0; i < rtVector_Size(d->params); ++i)
  {
    rbusProperty_t property;
    rbusValue_t newVal, oldVal;

    ValueChangeRecord* rec = (ValueChangeRecord *) rtVector_At(d->params, i);
    if (!rec)
      continue;

    rbusProperty_Init(&property,rbusProperty_GetName(rec->property), NULL);

    rbusGetHandlerOptions_t opts;
    memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
    opts.requestingComponent = "valueChangePollThread";

    int result = rec->node->cbTable.getHandler(rbus, property, &opts);

    if(result != RBUS_ERROR_SUCCESS)
    {
      RBUSLOG_WARN("%s: failed to get current value of %s", __FUNCTION__, rbusProperty_GetName(property));
      continue;
    }

    char* sValue = rbusValue_ToString(rbusProperty_GetValue(property), NULL, 0);
    RBUSLOG_DEBUG("%s: %s=%s", __FUNCTION__, rbusProperty_GetName(property), sValue);
    free(sValue);

    newVal = rbusProperty_GetValue(property);
    oldVal = rbusProperty_GetValue(rec->property);

    if(rbusValue_Compare(newVal, oldVal))
    {
      rbusEvent_t event = {0};
      rbusObject_t data;
      rbusValue_t byVal = NULL;

      RBUSLOG_INFO("%s: value change detected for %s", __FUNCTION__, rbusProperty_GetName(rec->property));

      /* The "by" field is set to the component's name which made the last value change.
         The source of a value-change could be an external component calling rbus_set or the provider internally updating
         the value.  changeComp/changeTime are updated through the rbus_set path, but not through the provider internal path.
         We must deduce if the provider has updated the value and reflect that change to the changeComp/changeTime, right here.
         If we don't have a changeComp or we do but the changeTime is older then the current polling period,
         then we know it was the provider who updated the value we are now detecting.
       */
      if(rec->node->changeComp == NULL || 
        (rtTime_Elapsed(&rec->node->changeTime, NULL) >= rbusConfig_Get()->valueChangePeriod &&
         strcmp(rbus->componentName, rec->node->changeComp) == 0))
      {
        RBUSLOG_INFO("VC detected provider-side value-change oldcomp=%s elapsed=%d period=%d\n", rec->node->changeComp,
          rtTime_Elapsed(&rec->node->changeTime, NULL), rbusConfig_Get()->valueChangePeriod);
          setPropertyChangeComponent((elementNode*)rec->node, rbus->componentName);
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
      result = rbusEvent_Publish(rbus, &event);

      rbusObject_Release(data);

      if(result != RBUS_ERROR_SUCCESS)
      {
        RBUSLOG_WARN("%s: rbusEvent_Publish failed with result=%d", __FUNCTION__, result);
      }

      /*update the record's property with new value*/
      rbusProperty_SetValue(rec->property, rbusProperty_GetValue(property));
      rbusProperty_Release(property);
    }
    else
    {
      RBUSLOG_DEBUG("%s: value change not detected for %s", __FUNCTION__, rbusProperty_GetName(rec->property));
      rbusProperty_Release(property);
    }
  }
}

void rbusValueChange_AddPropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;

    /* basic sanity tests */    
    assert(propNode);
    if(!propNode)
    {
        RBUSLOG_WARN("%s: propNode NULL error", __FUNCTION__);
        return;
    }
    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, propNode->fullName);
    assert(propNode->type == RBUS_ELEMENT_TYPE_PROPERTY);
    if(propNode->type != RBUS_ELEMENT_TYPE_PROPERTY)
    {
        RBUSLOG_WARN("%s: propNode type %d error", __FUNCTION__, propNode->type);
        return;
    }
    assert(propNode->cbTable.getHandler);
    if(!propNode->cbTable.getHandler)
    {
        RBUSLOG_WARN("%s: propNode getHandler NULL error", __FUNCTION__);
        return;
    }

    /* only add the property if its not already in the list */

    ERROR_CHECK( pthread_mutex_lock(&handle->valueChangeDetector.mutex) );

    rec = vcParams_Find(&handle->valueChangeDetector, propNode);

    ERROR_CHECK( pthread_mutex_unlock(&handle->valueChangeDetector.mutex) );

    if(!rec)
    {
        rec = (ValueChangeRecord*)rt_malloc(sizeof(ValueChangeRecord));
        rec->node = propNode;

        rbusProperty_Init(&rec->property, propNode->fullName, NULL);

        rbusGetHandlerOptions_t opts;
        memset(&opts, 0, sizeof(rbusGetHandlerOptions_t));
        opts.requestingComponent = "valueChangePollThread";
        /*get and cache the current value
          the polling thread will periodically re-get and compare to detect value changes*/
        int result = propNode->cbTable.getHandler(handle, rec->property, &opts);

        if(result != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_WARN("%s: failed to get current value for %s as the node is not found", __FUNCTION__, propNode->fullName);
            vcParams_Free(rec);
            rec = NULL;
            return;
        }

        char* sValue;
        RBUSLOG_DEBUG("%s: %s=%s", __FUNCTION__, propNode->fullName, (sValue = rbusValue_ToString(rbusProperty_GetValue(rec->property), NULL, 0)));
        free(sValue);

        ERROR_CHECK( pthread_mutex_lock(&handle->valueChangeDetector.mutex) );

        rtVector_PushBack(handle->valueChangeDetector.params, rec);

        /* start polling thread if needed */

        if (!handle->valueChangeDetector.running)
        {
            handle->valueChangeDetector.running = true;
            pthread_create(&handle->valueChangeDetector.thread, NULL, rbusValueChange_pollingThreadFunc, handle);
        }

        ERROR_CHECK( pthread_mutex_unlock(&handle->valueChangeDetector.mutex) );
    }
}

void rbusValueChange_RemovePropertyNode(rbusHandle_t handle, elementNode* propNode)
{
    ValueChangeRecord* rec;
    bool stopThread = false;

    VERIFY_NULL(propNode);
    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, propNode->fullName);

    ERROR_CHECK( pthread_mutex_lock(&handle->valueChangeDetector.mutex) );

    rec = vcParams_Find(&handle->valueChangeDetector, propNode);
    if(rec)
    {
        rtVector_RemoveItem(handle->valueChangeDetector.params, rec, vcParams_Free);
        /* if there's nothing left to poll then shutdown the polling thread */
        if (handle->valueChangeDetector.running && rtVector_Size(handle->valueChangeDetector.params) == 0)
        {
            stopThread = true;
            handle->valueChangeDetector.running = false;
        }
        else 
        {
            stopThread = false;
        }
    }
    else
    {
        RBUSLOG_WARN("%s: value change param not found: %s", __FUNCTION__, propNode->fullName);
    }

    ERROR_CHECK( pthread_mutex_unlock(&handle->valueChangeDetector.mutex) );


    if(stopThread)
    {
        ERROR_CHECK( pthread_cond_signal(&handle->valueChangeDetector.cond) );
        ERROR_CHECK( pthread_join(handle->valueChangeDetector.thread, NULL) );
    }
}

void rbusValueChange_Destroy(rbusValueChangeDetector_t *d)
{
    RBUSLOG_DEBUG("%s", __FUNCTION__);

    //remove all params for this bus handle
    ERROR_CHECK( pthread_mutex_lock(&d->mutex) );

    size_t i = 0;
    for (i = 0; i < rtVector_Size(d->params); ++i)
    {
        ValueChangeRecord* rec = (ValueChangeRecord *) rtVector_At(d->params, i);
        rtVector_RemoveItem(d->params, rec, vcParams_Free);
    }

    //clean up everything once all params are removed
    //but check the size to ensure we do not clean up if params for other rbus handles exist
    if (rtVector_Size(d->params) == 0)
    {
        if (d->running)
        {
            d->running = false;
            ERROR_CHECK( pthread_mutex_unlock(&d->mutex) );
            ERROR_CHECK( pthread_cond_signal(&d->cond) );
            ERROR_CHECK( pthread_join(d->thread, NULL) );
        }
        else
        {
            ERROR_CHECK( pthread_mutex_unlock(&d->mutex) );
        }
        ERROR_CHECK( pthread_mutex_destroy(&d->mutex) );
        ERROR_CHECK( pthread_cond_destroy(&d->cond) );
        rtVector_Destroy(d->params, NULL);
        d->params = NULL;
    }
    else
    {
        ERROR_CHECK( pthread_mutex_unlock(&d->mutex) );
    }
}
