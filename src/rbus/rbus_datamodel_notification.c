/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2026
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

#include <rbus.h>
#include <rbus_datamodel_notification.h>
#include <rtVector.h>
#include <rtLog.h>
#include <rtTime.h>
#include <rtMemory.h>
#include <rbuscore.h>
#include <rbuscore_message.h>
#include "rbus_log.h"

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define VERIFY_NULL_RET(T, R) if((T) == NULL) { return (R); }
#define ERROR_CHECK(CMD) do { int _e = (CMD); if(_e != 0) { RBUSLOG_ERROR("Error %d:%s running %s", _e, strerror(_e), #CMD); } } while(0)

#define RBUS_DMLNOTIFY_PROVIDER_EVENT_SUFFIX "._RBUS.DML!"

typedef struct _dmTok
{
    char* text;   /* owned */
    bool isStar;
} dmTok_t;

typedef struct _dmPattern
{
    char* original;     /* owned */
    dmTok_t* toks;      /* owned array */
    size_t tokCount;
} dmPattern_t;

typedef struct _dmBinding
{
    char* eventName; /* owned concrete RBUS eventName */
    bool subscribed;
} dmBinding_t;

typedef struct _dmQueuedEvent
{
    rbusDataModelNotificationEvent_t ev; /* strings are owned; values/objects are retained */
} dmQueuedEvent_t;

typedef struct _dmSub
{
    struct _rbusDataModelNotificationManager* mgr;
    rbusDataModelNotificationHandle_t handle;
    dmPattern_t pattern;
    rbusDataModelNotificationScope_t scope;
    rbusDataModelNotificationEventMask_t mask;
    rbusFilter_t filter;
    bool initialState;
    uint32_t expirationSeconds;
    rbusDataModelNotificationBatching_t batching;
    rbusDataModelNotificationHandler_t handler;
    rbusDataModelNotificationBatchHandler_t batchHandler;
    void* userData;

    rtVector bindings; /* dmBinding_t* */

    /* batching */
    rtVector queue; /* dmQueuedEvent_t* */
    rtTime_t batchWindowStart;
    uint32_t changeCountInWindow; /* total changes, used for heuristics */
    bool syncPending;
    uint64_t createdAtMs;
} dmSub_t;

struct _rbusDataModelNotificationManager
{
    rbusHandle_t handle;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int running;
    pthread_t thread;

    rbusDataModelNotificationHandle_t nextHandle;
    rtVector subs; /* dmSub_t* */

    rbusDataModelNotificationStats_t stats;
    bool discoverySubscribed;
    uint64_t lastDiscoveryRetryMs;
    uint64_t lastSyncAllMs;
    bool runDiscovery;
    rtVector boundEvents; /* char* */
};

static rbusError_t dmBindEvent(rbusDataModelNotificationManager_t mgr, dmSub_t* sub, char const* eventName);
static void dmOnDiscoverySignal(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);
static void dmHandleEvent(rbusHandle_t handle, rbusEvent_t const* eventData, rbusEventSubscription_t* subscription);
static void dmSyncSub(rbusDataModelNotificationManager_t mgr, dmSub_t* sub);

static void dmPattern_Free(dmPattern_t* p)
{
    if(!p) return;
    if(p->toks)
    {
        for(size_t i=0;i<p->tokCount;i++)
            free(p->toks[i].text);
        free(p->toks);
    }
    free(p->original);
    memset(p, 0, sizeof(*p));
}

static bool dmPattern_Compile(dmPattern_t* out, char const* pattern)
{
    memset(out, 0, sizeof(*out));
    if(!pattern || !pattern[0])
        return false;

    out->original = strdup(pattern);
    if(!out->original)
        return false;

    /* tokenize by '.' */
    char* tmp = strdup(pattern);
    if(!tmp)
        return false;

    size_t cap = 8;
    dmTok_t* toks = (dmTok_t*)calloc(cap, sizeof(dmTok_t));
    if(!toks)
    {
        free(tmp);
        return false;
    }

    size_t count = 0;
    char* saveptr = NULL;
    char* tok = strtok_r(tmp, ".", &saveptr);
    while(tok)
    {
        if(count == cap)
        {
            cap *= 2;
            dmTok_t* nt = (dmTok_t*)realloc(toks, cap * sizeof(dmTok_t));
            if(!nt)
            {
                free(tmp);
                for(size_t i=0;i<count;i++) free(toks[i].text);
                free(toks);
                return false;
            }
            toks = nt;
            memset(&toks[count], 0, (cap-count) * sizeof(dmTok_t));
        }
        toks[count].isStar = (strcmp(tok, "*") == 0);
        toks[count].text = strdup(tok);
        if(!toks[count].text)
        {
            free(tmp);
            for(size_t i=0;i<count;i++) free(toks[i].text);
            free(toks);
            return false;
        }
        count++;
        tok = strtok_r(NULL, ".", &saveptr);
    }
    free(tmp);

    out->toks = toks;
    out->tokCount = count;
    return true;
}

static bool dmPattern_Match(dmPattern_t const* p, char const* path)
{
    if(!p || !path) return false;

    /* tokenize path by '.' without allocations (scan segments) */
    size_t pi = 0;
    char const* s = path;
    while(*s && pi < p->tokCount)
    {
        char const* segStart = s;
        char const* dot = strchr(segStart, '.');
        size_t segLen = dot ? (size_t)(dot - segStart) : strlen(segStart);

        if(p->toks[pi].isStar)
        {
            /* If it's the last token, it's a recursive wildcard that matches everything remaining */
            if(pi == p->tokCount - 1)
            {
                s = path + strlen(path); /* consumed all */
                pi++;
                break;
            }
            /* Otherwise, it matches exactly one segment. Advancing s logic below handles it */
        }
        else
        {
            if(strlen(p->toks[pi].text) != segLen || strncmp(p->toks[pi].text, segStart, segLen) != 0)
                return false;
        }

        pi++;
        if(!dot)
        {
            s = segStart + segLen;
            break;
        }
        s = dot + 1;
    }

    /* exact match requires same token count */
    if(pi != p->tokCount)
        return false;
    /* allow optional single trailing dot */
    if(*s == '.' && *(s+1) == '\0')
        s++;
    if(*s != '\0')
        return false;
    return true;
}

static uint64_t dmNowMs(void)
{
    rtTime_t now;
    rtTime_Now(&now);
    return (uint64_t)now.tv_sec * 1000u + (uint64_t)(now.tv_nsec / 1000000u);
}

static void dmEvent_Free(rbusDataModelNotificationEvent_t* ev)
{
    if(!ev) return;
    free((void*)ev->path);
    free((void*)ev->sourceComponent);
    if(ev->oldValue) rbusValue_Release(ev->oldValue);
    if(ev->newValue) rbusValue_Release(ev->newValue);
    if(ev->details) rbusObject_Release(ev->details);
    memset(ev, 0, sizeof(*ev));
}

static void dmQueuedEvent_Free(void* p)
{
    dmQueuedEvent_t* q = (dmQueuedEvent_t*)p;
    if(!q) return;
    dmEvent_Free(&q->ev);
    free(q);
}

static void dmBinding_Free(void* p)
{
    dmBinding_t* b = (dmBinding_t*)p;
    if(!b) return;
    free(b->eventName);
    free(b);
}

static void dmSub_Free(void* p)
{
    dmSub_t* s = (dmSub_t*)p;
    if(!s) return;
    if(s->mgr && s->bindings)
    {
        for(size_t i=0;i<rtVector_Size(s->bindings);i++)
        {
            dmBinding_t* b = (dmBinding_t*)rtVector_At(s->bindings, (int)i);
            if(b && b->subscribed && b->eventName)
            {
                (void)rbusEvent_Unsubscribe(s->mgr->handle, b->eventName);
            }
        }
    }
    dmPattern_Free(&s->pattern);
    if(s->filter) rbusFilter_Release(s->filter);
    if(s->bindings) rtVector_Destroy(s->bindings, dmBinding_Free);
    if(s->queue) rtVector_Destroy(s->queue, dmQueuedEvent_Free);
    free(s);
}

static dmSub_t* dmFindSubByHandle(rbusDataModelNotificationManager_t mgr, rbusDataModelNotificationHandle_t h)
{
    if(!mgr) return NULL;
    for(size_t i=0;i<rtVector_Size(mgr->subs);i++)
    {
        dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (int)i);
        if(s && s->handle == h)
            return s;
    }
    return NULL;
}

static void dmDeliverImmediate(rbusDataModelNotificationManager_t mgr, dmSub_t* sub, rbusDataModelNotificationEvent_t const* ev)
{
    if(!mgr || !sub || !ev) return;
    if(sub->handler)
    {
        sub->handler(mgr->handle, ev, sub->userData);
        mgr->stats.notificationsDelivered++;
    }
}

static void dmFlushSub(rbusDataModelNotificationManager_t mgr, dmSub_t* sub);

static void dmQueueOrDeliver(rbusDataModelNotificationManager_t mgr, dmSub_t* sub, rbusDataModelNotificationEvent_t* evOwned)
{
    if(!mgr || !sub || !evOwned) return;

    if(sub->batching.batchWindowMs == 0 && sub->batchHandler == NULL)
    {
        dmDeliverImmediate(mgr, sub, evOwned);
        dmEvent_Free(evOwned);
        return;
    }

    if(!sub->queue)
        rtVector_Create(&sub->queue);

    /* Check if current batch window has expired or is full, and flush if so */
    if(rtVector_Size(sub->queue) > 0)
    {
        bool flush = false;
        if(sub->batching.maxBatchSize && rtVector_Size(sub->queue) >= sub->batching.maxBatchSize)
            flush = true;
        
        if(!flush && sub->batching.batchWindowMs)
        {
            uint64_t nowMs = dmNowMs();
            uint64_t startMs = (uint64_t)sub->batchWindowStart.tv_sec * 1000u + (uint64_t)sub->batchWindowStart.tv_nsec / 1000000u;
            if(nowMs >= startMs + sub->batching.batchWindowMs)
                flush = true;
        }

        if(flush)
            dmFlushSub(mgr, sub);
    }

    /* Skip rate limit and coalescing for structural events (creation/deletion) */
    if(evOwned->type == RBUS_DMLNOTIFY_OBJECT_CREATION || evOwned->type == RBUS_DMLNOTIFY_OBJECT_DELETION)
    {
        goto enqueue;
    }

    /* Rate limit: best-effort drop once queue is beyond a per-window bound. */
    if(sub->batching.rateLimitPerSec && sub->batching.batchWindowMs)
    {
        uint64_t maxPerWindow = ((uint64_t)sub->batching.rateLimitPerSec * (uint64_t)sub->batching.batchWindowMs) / 1000u;
        if(maxPerWindow == 0) maxPerWindow = 1;
        if(rtVector_Size(sub->queue) >= maxPerWindow)
        {
            mgr->stats.notificationsDropped++;
            dmEvent_Free(evOwned);
            return;
        }
    }

    /* Hybrid coalescing: if a path changes > threshold within a batch window, coalesce ValueChange events for that path. */
    if(sub->batching.coalesceThreshold && evOwned->type == RBUS_DMLNOTIFY_VALUE_CHANGE && evOwned->path)
    {
        size_t countForPath = 0;
        for(size_t i=0;i<rtVector_Size(sub->queue);i++)
        {
            dmQueuedEvent_t* q = (dmQueuedEvent_t*)rtVector_At(sub->queue, (int)i);
            if(q && q->ev.type == RBUS_DMLNOTIFY_VALUE_CHANGE && q->ev.path && strcmp(q->ev.path, evOwned->path) == 0)
                countForPath++;
        }
        if(countForPath + 1 > sub->batching.coalesceThreshold)
        {
            /* Replace last queued ValueChange for this path (keep first oldValue). */
            for(int i=(int)rtVector_Size(sub->queue)-1; i>=0; i--)
            {
                dmQueuedEvent_t* q = (dmQueuedEvent_t*)rtVector_At(sub->queue, (size_t)i);
                if(q && q->ev.type == RBUS_DMLNOTIFY_VALUE_CHANGE && q->ev.path && strcmp(q->ev.path, evOwned->path) == 0)
                {
                    if(q->ev.newValue) rbusValue_Release(q->ev.newValue);
                    q->ev.newValue = evOwned->newValue;
                    evOwned->newValue = NULL;
                    q->ev.timestampMs = evOwned->timestampMs;
                    dmEvent_Free(evOwned);
                    ERROR_CHECK(pthread_cond_signal(&mgr->cond));
                    return;
                }
            }
        }
    }

enqueue:
    dmQueuedEvent_t* q = (dmQueuedEvent_t*)rt_calloc(1, sizeof(dmQueuedEvent_t));
    q->ev = *evOwned; /* move */
    memset(evOwned, 0, sizeof(*evOwned));
    rtVector_PushBack(sub->queue, q);

    if(rtVector_Size(sub->queue) == 1)
        rtTime_Now(&sub->batchWindowStart);

    ERROR_CHECK(pthread_cond_signal(&mgr->cond));
}

static void dmFlushSub(rbusDataModelNotificationManager_t mgr, dmSub_t* sub)
{
    if(!mgr || !sub) return;
    if(!sub->queue || rtVector_Size(sub->queue) == 0) return;

    size_t n = rtVector_Size(sub->queue);
    rbusDataModelNotificationEvent_t* evs = (rbusDataModelNotificationEvent_t*)rt_calloc((int)n, sizeof(rbusDataModelNotificationEvent_t));
    if(!evs)
        return;

    for(size_t i=0;i<n;i++)
    {
        dmQueuedEvent_t* q = (dmQueuedEvent_t*)rtVector_At(sub->queue, (int)i);
        evs[i] = q->ev; /* move */
        memset(&q->ev, 0, sizeof(q->ev));
    }

    if(sub->batchHandler)
    {
        rbusDataModelNotificationEventBatch_t batch = { .events = evs, .count = n };
        sub->batchHandler(mgr->handle, &batch, sub->userData);
        mgr->stats.notificationsBatched += n;
    }
    else
    {
        for(size_t i=0;i<n;i++)
            dmDeliverImmediate(mgr, sub, &evs[i]);
    }

    /* free moved event resources */
    for(size_t i=0;i<n;i++)
        dmEvent_Free(&evs[i]);
    rt_free(evs);

    rtVector_Destroy(sub->queue, dmQueuedEvent_Free);
    rtVector_Create(&sub->queue);
}

static void* dmThread(void* arg)
{
    rbusDataModelNotificationManager_t mgr = (rbusDataModelNotificationManager_t)arg;
    
    /* Stabilization delay: wait for handle registration and bus to settle. */
    sleep(1);

    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    while(mgr->running)
    {
        /* Expiration cleanup and Background Sync */
        uint64_t nowMs = dmNowMs();
        for(int i=(int)rtVector_Size(mgr->subs)-1; i>=0; i--)
        {
            dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (size_t)i);
            if(!s) continue;
            
            /* Background Sync */
            if(s->syncPending)
            {
                s->syncPending = false;
                dmSyncSub(mgr, s);
                /* dmSyncSub may have unlocked/relocked or just taken time, refreshment of 'nowMs' might be good but not critical for cleanup */
            }

            if(s->expirationSeconds && (nowMs - s->createdAtMs) >= (uint64_t)s->expirationSeconds * 1000u)
            {
                rtVector_RemoveItem(mgr->subs, s, dmSub_Free);
                if(mgr->stats.activeSubscriptions) mgr->stats.activeSubscriptions--;
            }
        }

        /* determine next wakeup based on batching windows */
        rtTime_t now;
        rtTime_Now(&now);
        rtTime_t nextWake = now;
        bool haveWake = false;

        for(size_t i=0;i<rtVector_Size(mgr->subs);i++)
        {
            dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (int)i);
            if(!s || !s->queue || rtVector_Size(s->queue) == 0) continue;
            if(s->batching.batchWindowMs == 0) continue;

            rtTime_t due;
            rtTime_Later(&s->batchWindowStart, (int)s->batching.batchWindowMs, &due);
            if(!haveWake || rtTime_Compare(&due, &nextWake) < 0)
            {
                nextWake = due;
                haveWake = true;
            }
        }

        /* Explicitly discover and subscribe to new provider discovery signals.
           We do this instead of a wildcard subscription which is unreliable in some environments. */
        if((nowMs - mgr->lastDiscoveryRetryMs) >= 2000u)
        {
            mgr->lastDiscoveryRetryMs = nowMs;
            
            int numComps = 0;
            char** compNames = NULL;
            if(rbus_discoverWildcardDestinations(RBUS_DML_DISCOVERY_SIGNAL, &numComps, &compNames) == RBUSCORE_SUCCESS)
            {
                for(int j=0; j<numComps; j++)
                {
                    char signalName[RBUS_MAX_NAME_LENGTH];
                    char flattenedName[RBUS_MAX_NAME_LENGTH];
                    strncpy(flattenedName, compNames[j], sizeof(flattenedName)-1);
                    flattenedName[sizeof(flattenedName)-1] = '\0';
                    char* p = flattenedName;
                    while(*p) { if(*p == '.') *p = '_'; p++; }

                    snprintf(signalName, sizeof(signalName), "%s.%s", RBUS_DML_DISCOVERY_SIGNAL, flattenedName);
                    
                    /* Check if already bound or check if we can just subscribe again (rbus handles duplicates) */
                    bool alreadyBound = false;
                    for(size_t k=0; k<rtVector_Size(mgr->boundEvents); k++)
                    {
                        if(strcmp(rtVector_At(mgr->boundEvents, (int)k), signalName) == 0)
                        {
                            alreadyBound = true;
                            break;
                        }
                    }

                    if(!alreadyBound)
                    {
                        fprintf(stderr, "dmlnotify: explicitly subscribing to discovery signal %s\n", signalName);
                        if(rbusEvent_Subscribe(mgr->handle, signalName, dmOnDiscoverySignal, mgr, 0) == RBUS_ERROR_SUCCESS)
                        {
                            rtVector_PushBack(mgr->boundEvents, strdup(signalName));
                            fprintf(stderr, "dmlnotify: successfully subscribed to %s\n", signalName);
                        }
                    }
                    free(compNames[j]);
                }
                free(compNames);
            }
        }
        
        /* Periodic background sync as a fallback for push-based discovery.
           This ensures we eventually find elements even if signals were missed. */
        if((nowMs - mgr->lastSyncAllMs) >= 2000u)
        {
            mgr->lastSyncAllMs = nowMs;
            /* Use index to avoid issues if subs are added, 
               and we handle the lock properly in dmSyncSub. */
            for(size_t i=0; i<rtVector_Size(mgr->subs); i++)
            {
                dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (int)i);
                if(s)
                {
                    dmSyncSub(mgr, s);
                    /* Re-lock because dmSyncSub releases it. 
                       Wait, dmSyncSub already re-locks it before returning. */
                }
            }
        }

        if(!haveWake)
        {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 2;
            pthread_cond_timedwait(&mgr->cond, &mgr->mutex, &ts);
            continue;
        }

        struct timespec ts;
        rtTime_ToTimespec(&nextWake, &ts);
        pthread_cond_timedwait(&mgr->cond, &mgr->mutex, &ts);

        rtTime_Now(&now);
        for(size_t i=0;i<rtVector_Size(mgr->subs);i++)
        {
            dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (int)i);
            if(!s || !s->queue || rtVector_Size(s->queue) == 0) continue;

            bool flush = false;
            if(s->batching.maxBatchSize && rtVector_Size(s->queue) >= s->batching.maxBatchSize)
                flush = true;
            if(s->batching.batchWindowMs)
            {
                rtTime_t due;
                rtTime_Later(&s->batchWindowStart, (int)s->batching.batchWindowMs, &due);
                if(rtTime_Compare(&now, &due) >= 0)
                    flush = true;
            }
            if(flush)
                dmFlushSub(mgr, s);
        }
    }
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
    return NULL;
}

static void dmHandleEvent(rbusHandle_t handle, rbusEvent_t const* eventData, rbusEventSubscription_t* subscription)
{
    (void)handle;
    if(!eventData || !subscription || !subscription->userData) return;
    dmSub_t* sub = (dmSub_t*)subscription->userData;
    rbusDataModelNotificationManager_t mgr = sub->mgr;

    rbusDataModelNotificationEvent_t ev = {0};
    ev.timestampMs = dmNowMs();

    /* Provider datamodel change signal. */
    if(eventData->type == RBUS_EVENT_GENERAL && eventData->data)
    {
        rbusValue_t kindV = rbusObject_GetValue(eventData->data, "kind");
        rbusValue_t pathV = rbusObject_GetValue(eventData->data, "path");
        char const* kind = kindV ? rbusValue_GetString(kindV, NULL) : NULL;
        char const* path = pathV ? rbusValue_GetString(pathV, NULL) : NULL;
        if(kind && path)
        {
            RBUSLOG_INFO("dmlnotify received signal: kind=%s path=%s", kind, path);
            /* If a newly registered element matches this subscription, bind to it. */
            if(strcmp(kind, "ElementRegistered") == 0 || strcmp(kind, "RowRegistered") == 0)
            {
                RBUSLOG_INFO("dmlnotify matching signal path %s (pattern=%s)", path, sub->pattern.original);
                if(dmPattern_Match(&sub->pattern, path) || sub->scope == RBUS_DMLNOTIFY_SCOPE_SUBTREE)
                {
                    RBUSLOG_INFO("dmlnotify SIGNAL MATCHED! binding to %s", path);
                    (void)dmBindEvent(mgr, sub, path);

                    /* Also notify the client about the new path if they requested creation events */
                    if(sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_CREATION)
                    {
                        RBUSLOG_INFO("dmlnotify synthesizing OBJECT_CREATION for %s", path);
                        rbusDataModelNotificationEvent_t discoveryEv = {0};
                        discoveryEv.type = RBUS_DMLNOTIFY_OBJECT_CREATION;
                        discoveryEv.path = strdup(path);
                        discoveryEv.timestampMs = dmNowMs();
                        dmQueueOrDeliver(mgr, sub, &discoveryEv);
                    }

                    /* If a row was registered, discover its children to ensure they are bound. */
                    if(strcmp(kind, "RowRegistered") == 0)
                    {
                        int numElems = 0;
                        char** elemNames = NULL;
                        char rowPath[RBUS_MAX_NAME_LENGTH];
                        snprintf(rowPath, sizeof(rowPath), "%s%s", path, path[strlen(path)-1] == '.' ? "" : ".");

                        RBUSLOG_INFO("dmlnotify discovering children for new row %s", rowPath);
                        if(rbus_discoverObjectElements(rowPath, &numElems, &elemNames) == RBUSCORE_SUCCESS)
                        {
                            for(int i=0; i<numElems; i++)
                            {
                                if(dmPattern_Match(&sub->pattern, elemNames[i]))
                                {
                                    RBUSLOG_INFO("dmlnotify auto-binding row child %s", elemNames[i]);
                                    (void)dmBindEvent(mgr, sub, elemNames[i]);
                                }
                                free(elemNames[i]);
                            }
                            free(elemNames);
                        }
                    }
                }
            }
            else if(strcmp(kind, "ElementUnregistered") == 0 || strcmp(kind, "RowUnregistered") == 0)
            {
                if((dmPattern_Match(&sub->pattern, path) || sub->scope == RBUS_DMLNOTIFY_SCOPE_SUBTREE) &&
                   (sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_DELETION))
                {
                    rbusDataModelNotificationEvent_t delEv = {0};
                    delEv.type = RBUS_DMLNOTIFY_OBJECT_DELETION;
                    delEv.path = strdup(path);
                    delEv.timestampMs = dmNowMs();
                    dmQueueOrDeliver(mgr, sub, &delEv);
                }
            }
        }
        return;
    }

    if(eventData->type == RBUS_EVENT_VALUE_CHANGED && (sub->mask & RBUS_DMLNOTIFY_MASK_VALUE_CHANGE))
    {
        RBUSLOG_INFO("dmlnotify dmHandleEvent received VALUE_CHANGED for %s (sub pattern=%s)", eventData->name, sub->pattern.original);
        if(!dmPattern_Match(&sub->pattern, eventData->name) && sub->scope == RBUS_DMLNOTIFY_SCOPE_EXACT)
        {
            RBUSLOG_INFO("dmlnotify VALUE_CHANGED path %s NO MATCH", eventData->name);
            return;
        }
        RBUSLOG_INFO("dmlnotify VALUE_CHANGED path %s MATCHED", eventData->name);
        ev.type = RBUS_DMLNOTIFY_VALUE_CHANGE;
        ev.path = strdup(eventData->name);
        rbusValue_t nv = rbusObject_GetValue(eventData->data, "value");
        rbusValue_t ov = rbusObject_GetValue(eventData->data, "oldValue");
        rbusValue_t by = rbusObject_GetValue(eventData->data, "by");
        if(sub->filter && nv && !rbusFilter_Apply(sub->filter, nv))
            return;
        if(nv) { rbusValue_Retain(nv); ev.newValue = nv; }
        if(ov) { rbusValue_Retain(ov); ev.oldValue = ov; }
        if(by && rbusValue_GetType(by) == RBUS_STRING)
        {
            char const* s = rbusValue_GetString(by, NULL);
            if(s) ev.sourceComponent = strdup(s);
        }
        dmQueueOrDeliver(mgr, sub, &ev);
        return;
    }

    if(eventData->type == RBUS_EVENT_OBJECT_CREATED && (sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_CREATION))
    {
        rbusValue_t rowName = rbusObject_GetValue(eventData->data, "rowName");
        char const* path = rowName ? rbusValue_GetString(rowName, NULL) : NULL;
        if(!path) return;
        ev.type = RBUS_DMLNOTIFY_OBJECT_CREATION;
        ev.path = strdup(path);
        dmQueueOrDeliver(mgr, sub, &ev);
        return;
    }

    if(eventData->type == RBUS_EVENT_OBJECT_DELETED && (sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_DELETION))
    {
        rbusValue_t rowName = rbusObject_GetValue(eventData->data, "rowName");
        char const* path = rowName ? rbusValue_GetString(rowName, NULL) : NULL;
        if(!path) return;
        ev.type = RBUS_DMLNOTIFY_OBJECT_DELETION;
        ev.path = strdup(path);
        dmQueueOrDeliver(mgr, sub, &ev);
        return;
    }
}

static void dmRbusEventHandler2(rbusHandle_t handle, rbusEvent_t const* eventData, rbusEventSubscription_t* subscription)
{
    (void)handle;
    RBUSLOG_INFO("dmlnotify dmRbusEventHandler2 received event: %s", eventData->name);
    if(!subscription || !subscription->userData || !eventData)
        return;
    dmSub_t* sub = (dmSub_t*)subscription->userData;
    if(!sub || !sub->mgr)
        return;
    dmHandleEvent(sub->mgr->handle, eventData, subscription);
}

static rbusError_t dmBindEvent(rbusDataModelNotificationManager_t mgr, dmSub_t* sub, char const* eventName)
{
    VERIFY_NULL_RET(mgr, RBUS_ERROR_INVALID_HANDLE);
    VERIFY_NULL_RET(sub, RBUS_ERROR_INVALID_INPUT);
    VERIFY_NULL_RET(eventName, RBUS_ERROR_INVALID_INPUT);

    /* Avoid duplicate bindings. */
    if(sub->bindings)
    {
        for(size_t i=0;i<rtVector_Size(sub->bindings);i++)
        {
            dmBinding_t* existing = (dmBinding_t*)rtVector_At(sub->bindings, (int)i);
            if(existing && existing->eventName && strcmp(existing->eventName, eventName) == 0)
                return RBUS_ERROR_SUCCESS;
        }
    }

    dmBinding_t* b = (dmBinding_t*)rt_calloc(1, sizeof(dmBinding_t));
    b->eventName = strdup(eventName);

    if(!sub->bindings)
        rtVector_Create(&sub->bindings);
    rtVector_PushBack(sub->bindings, b);

    /* Subscribe with short timeout to prevent hang. RBUS retries will handle future path activation. */
    /* RBUS event system uses '*' for wildcards. If the path has '{i}', we translate to a suffix wildcard
       to ensure we catch all instances under that branch. This is more robust for rtrouted matching. */
    char rbusEventName[RBUS_MAX_NAME_LENGTH];
    char const* s = eventName;
    char* d = rbusEventName;
    bool translated = false;
    while(*s && (d < rbusEventName + sizeof(rbusEventName) - 2))
    {
        if(!translated && strncmp(s, "{i}", 3) == 0)
        {
            *d++ = '*';
            translated = true;
            break; /* Stop here and use suffix wildcard */
        }
        else
        {
            *d++ = *s++;
        }
    }
    *d = '\0';
    
    if(translated)
    {
        RBUSLOG_INFO("dmlnotify dmBindEvent: translated mid-wildcard %s to %s", eventName, rbusEventName);
    }
    *d = '\0';

    RBUSLOG_DEBUG("dmlnotify calling rbusEvent_Subscribe for %s (orig=%s)", rbusEventName, eventName);
    rbusError_t rc = rbusEvent_Subscribe(mgr->handle, rbusEventName, dmHandleEvent, sub, 0);
    RBUSLOG_DEBUG("dmlnotify rbusEvent_Subscribe for %s returned %d", rbusEventName, rc);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        RBUSLOG_WARN("dmlnotify failed to subscribe to %s rc=%d", eventName, rc);
    }
    else
    {
        b->subscribed = true;
    }
    return rc;
}

static void dmOnDiscoverySignal(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    rbusDataModelNotificationManager_t mgr = (rbusDataModelNotificationManager_t)subscription->userData;
    if(!mgr || !event->data) return;

    rbusValue_t kindV = rbusObject_GetValue(event->data, "kind");
    rbusValue_t pathV = rbusObject_GetValue(event->data, "path");
    rbusValue_t providerV = rbusObject_GetValue(event->data, "provider");
    char const* kind = kindV ? rbusValue_GetString(kindV, NULL) : NULL;
    char const* path = pathV ? rbusValue_GetString(pathV, NULL) : NULL;
    char const* provider = providerV ? rbusValue_GetString(providerV, NULL) : NULL;

    RBUSLOG_INFO("dmlnotify: manager received signal kind=%s path=%s provider=%s", kind, path, provider ? provider : "unknown");

    if(kind && path)
    {
        fprintf(stderr, "dmlnotify: manager received signal kind=%s path=%s provider=%s\n", kind, path, provider ? provider : "unknown");
        RBUSLOG_INFO("dmlnotify global discovery signal: kind=%s path=%s provider=%s", kind, path, provider ? provider : "unknown");
        
        ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
        for(size_t i=0; i<rtVector_Size(mgr->subs); i++)
        {
            dmSub_t* sub = (dmSub_t*)rtVector_At(mgr->subs, i);
            fprintf(stderr, "dmlnotify: checking sub pattern=%s\n", sub->pattern.original);
            if(strcmp(kind, "ElementRegistered") == 0 || strcmp(kind, "RowRegistered") == 0)
            {
                if(dmPattern_Match(&sub->pattern, path) || sub->scope == RBUS_DMLNOTIFY_SCOPE_SUBTREE)
                {
                    fprintf(stderr, "dmlnotify: MATCHED! binding to %s\n", path);
                    (void)dmBindEvent(mgr, sub, path);
                    if(sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_CREATION)
                    {
                        rbusDataModelNotificationEvent_t discoveryEv = {0};
                        discoveryEv.type = RBUS_DMLNOTIFY_OBJECT_CREATION;
                        discoveryEv.path = strdup(path);
                        discoveryEv.timestampMs = dmNowMs();
                        dmQueueOrDeliver(mgr, sub, &discoveryEv);
                    }
                }
            }
            else if(strcmp(kind, "ElementUnregistered") == 0 || strcmp(kind, "RowUnregistered") == 0)
            {
                if(dmPattern_Match(&sub->pattern, path) || sub->scope == RBUS_DMLNOTIFY_SCOPE_SUBTREE)
                {
                    if(sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_DELETION)
                    {
                        rbusDataModelNotificationEvent_t delEv = {0};
                        delEv.type = RBUS_DMLNOTIFY_OBJECT_DELETION;
                        delEv.path = strdup(path);
                        delEv.timestampMs = dmNowMs();
                        dmQueueOrDeliver(mgr, sub, &delEv);
                    }
                }
            }
        }
        ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
    }
}

static void dmSyncSub(rbusDataModelNotificationManager_t mgr, dmSub_t* sub)
{
    if(!mgr || !sub) return;

    /* Unlock manager mutex while doing discovery to avoid blocking other threads. */
    pthread_mutex_unlock(&mgr->mutex);

    int numComps = 0;
    char** compNames = NULL;
    char const* pattern = sub->pattern.original;
    
    fprintf(stderr, "dmlnotify sync for sub %u starting (pattern=%s)\n", sub->handle, pattern);
    
    /* Clean prefix for discovery: remove wildcards */
    char prefix[RBUS_MAX_NAME_LENGTH];
    strncpy(prefix, pattern, sizeof(prefix)-1);
    prefix[sizeof(prefix)-1] = '\0';
    char* star = strchr(prefix, '*');
    if(star) *star = '\0';

    /* Remove trailing dot for discovery if it's longer than just the dot */
    size_t len = strlen(prefix);
    if(len > 1 && prefix[len-1] == '.')
        prefix[len-1] = '\0';

    /* Use the same logic as rbus_getExt: use rbus_discoverWildcardDestinations for wildcard/partial paths */
    rbusCoreError_t rc = rbus_discoverWildcardDestinations(prefix, &numComps, &compNames);
    fprintf(stderr, "dmlnotify sync: discoverWildcardDestinations for prefix %s returned %d, numComps=%d\n", prefix, rc, numComps);
    if(rc == RBUSCORE_SUCCESS && numComps > 0)
    {
        for(int j=0; j<numComps; j++)
        {
            int numElems = 0;
            char** elemNames = NULL;
            RBUSLOG_INFO("dmlnotify sync: discovering elements for component %s", compNames[j]);
            /* Discover all elements recursively (nextLevel=false) */
            if(rbus_discoverComponentDataElements(mgr->handle, compNames[j], false, &numElems, &elemNames) == RBUS_ERROR_SUCCESS)
            {
                /* Relock for vector and bind operations */
                pthread_mutex_lock(&mgr->mutex);
                for(int k=0; k<numElems; k++)
                {
                    /* Only match if it actually starts with the pattern root to avoid binding to 
                       component names or other non-DML elements that might be returned. */
                    if(strncmp(elemNames[k], prefix, strlen(prefix)) == 0 &&
                       (dmPattern_Match(&sub->pattern, elemNames[k]) || sub->scope == RBUS_DMLNOTIFY_SCOPE_SUBTREE))
                    {
                        bool alreadyBound = false;
                        if(sub->bindings)
                        {
                            for(size_t i=0;i<rtVector_Size(sub->bindings);i++)
                            {
                                dmBinding_t* existing = (dmBinding_t*)rtVector_At(sub->bindings, (int)i);
                                if(existing && existing->eventName && strcmp(existing->eventName, elemNames[k]) == 0)
                                {
                                    alreadyBound = true;
                                    break;
                                }
                            }
                        }

                        if(!alreadyBound)
                        {
                            fprintf(stderr, "dmlnotify sync: matched and binding to %s\n", elemNames[k]);
                            (void)dmBindEvent(mgr, sub, elemNames[k]);

                            /* Explicitly notify the client about the pre-existing path if requested */
                            if(sub->mask & RBUS_DMLNOTIFY_MASK_OBJECT_CREATION)
                            {
                                rbusDataModelNotificationEvent_t discoveryEv = {0};
                                discoveryEv.type = RBUS_DMLNOTIFY_OBJECT_CREATION;
                                discoveryEv.path = strdup(elemNames[k]);
                                discoveryEv.timestampMs = dmNowMs();
                                dmQueueOrDeliver(mgr, sub, &discoveryEv);
                            }
                        }
                    }
                    free(elemNames[k]);
                }
                free(elemNames);
                pthread_mutex_unlock(&mgr->mutex);
            }
            free(compNames[j]);
        }
        free(compNames);
    }
    
    /* Relock before returning to dmThread which expects to hold it. */
    pthread_mutex_lock(&mgr->mutex);
    
    /* Ensure all structural discovery events are flushed to the client immediately */
    dmFlushSub(mgr, sub);

    RBUSLOG_INFO("dmlnotify sync for sub %u complete", sub->handle);
}

static void dmSubscribeProviderModelSignals(rbusDataModelNotificationManager_t mgr, dmSub_t* sub, char const* pattern)
{
    (void)mgr; (void)sub; (void)pattern;
    /* Discovery thread handles this periodically */
}

rbusError_t rbusDataModelNotificationManager_Create(
    rbusHandle_t handle,
    rbusDataModelNotificationManager_t* outMgr)
{
    VERIFY_NULL_RET(handle, RBUS_ERROR_INVALID_HANDLE);
    VERIFY_NULL_RET(outMgr, RBUS_ERROR_INVALID_INPUT);

    rbusDataModelNotificationManager_t mgr = (rbusDataModelNotificationManager_t)rt_calloc(1, sizeof(*mgr));
    if(!mgr)
        return RBUS_ERROR_OUT_OF_RESOURCES;

    mgr->handle = handle;
    mgr->running = 1;
    mgr->nextHandle = 1;
    rtVector_Create(&mgr->subs);
    rtVector_Create(&mgr->boundEvents);

    pthread_mutexattr_t attrib;
    ERROR_CHECK(pthread_mutexattr_init(&attrib));
    ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&mgr->mutex, &attrib));
    ERROR_CHECK(pthread_cond_init(&mgr->cond, NULL));

    /* NotifyDML manager doesn't need to register the discovery node itself.
       Each provider registers its own unique signal in rbus_open logic. */

    if(pthread_create(&mgr->thread, NULL, dmThread, mgr) != 0)
    {
        rtVector_Destroy(mgr->subs, (void (*)(void*))dmSub_Free);
        pthread_mutex_destroy(&mgr->mutex);
        pthread_cond_destroy(&mgr->cond);
        free(mgr);
        return RBUS_ERROR_BUS_ERROR;
    }

    *outMgr = mgr;
    return RBUS_ERROR_SUCCESS;
}

void rbusDataModelNotificationManager_Destroy(rbusDataModelNotificationManager_t mgr)
{
    if(!mgr) return;
    if(mgr->discoverySubscribed)
    {
        char wildcardSignal[RBUS_MAX_NAME_LENGTH];
        snprintf(wildcardSignal, sizeof(wildcardSignal), "%s.*", RBUS_DML_DISCOVERY_SIGNAL);
        rbusEvent_Unsubscribe(mgr->handle, wildcardSignal);
        mgr->discoverySubscribed = false;
    }

    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    mgr->running = 0;
    ERROR_CHECK(pthread_cond_signal(&mgr->cond));
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));

    pthread_join(mgr->thread, NULL);
    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    if(mgr->subs) rtVector_Destroy(mgr->subs, dmSub_Free);
    if(mgr->boundEvents)
    {
        for(size_t i=0; i<rtVector_Size(mgr->boundEvents); i++)
            free(rtVector_At(mgr->boundEvents, i));
        rtVector_Destroy(mgr->boundEvents, NULL);
    }
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));

    pthread_mutex_destroy(&mgr->mutex);
    pthread_cond_destroy(&mgr->cond);
    free(mgr);
}

rbusError_t rbusDataModelNotificationManager_Subscribe(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationRequest_t const* req,
    rbusDataModelNotificationHandle_t* outHandle)
{
    VERIFY_NULL_RET(mgr, RBUS_ERROR_INVALID_HANDLE);
    VERIFY_NULL_RET(req, RBUS_ERROR_INVALID_INPUT);
    VERIFY_NULL_RET(req->pattern, RBUS_ERROR_INVALID_INPUT);
    VERIFY_NULL_RET(outHandle, RBUS_ERROR_INVALID_INPUT);
    if(!req->handler && !req->batchHandler)
        return RBUS_ERROR_INVALID_INPUT;

    dmSub_t* sub = (dmSub_t*)rt_calloc(1, sizeof(dmSub_t));
    if(!sub)
        return RBUS_ERROR_OUT_OF_RESOURCES;

    if(!dmPattern_Compile(&sub->pattern, req->pattern))
    {
        dmSub_Free(sub);
        return RBUS_ERROR_INVALID_INPUT;
    }

    sub->scope = req->scope;
    sub->mask = req->eventMask;
    sub->filter = req->filter;
    if(sub->filter) rbusFilter_Retain(sub->filter);
    sub->initialState = req->initialState;
    sub->expirationSeconds = req->expirationSeconds;
    sub->batching = req->batching;
    sub->handler = req->handler;
    sub->batchHandler = req->batchHandler;
    sub->userData = req->userData;

    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    sub->handle = mgr->nextHandle++;
    sub->mgr = mgr;
    sub->createdAtMs = dmNowMs();
    rtVector_PushBack(mgr->subs, sub);
    mgr->stats.activeSubscriptions++;
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));

    /* Bind best-effort: subscribe directly to pattern (works for many RBUS cases, and retries for future paths). */
    fprintf(stderr, "dmlnotify Manager_Subscribe: calling dmBindEvent for %s\n", req->pattern);
    (void)dmBindEvent(mgr, sub, req->pattern);
    fprintf(stderr, "dmlnotify Manager_Subscribe: calling dmSubscribeProviderModelSignals\n");
    dmSubscribeProviderModelSignals(mgr, sub, req->pattern);

    /* Initial state retrieval: best-effort wildcard getExt, then synthesize ValueChange events. */
    if(req->initialState)
    {
        int numProps = 0;
        rbusProperty_t props = NULL;
        char const* names[1] = { req->pattern };
        if(rbus_getExt(mgr->handle, 1, names, &numProps, &props) == RBUS_ERROR_SUCCESS && numProps > 0 && props)
        {
            rbusProperty_t p = props;
            while(p)
            {
                char const* pname = rbusProperty_GetName(p);
                rbusValue_t pval = rbusProperty_GetValue(p);
                if(pname && pval && (!sub->filter || rbusFilter_Apply(sub->filter, pval)))
                {
                    rbusDataModelNotificationEvent_t ev = {0};
                    ev.type = RBUS_DMLNOTIFY_VALUE_CHANGE;
                    ev.path = strdup(pname);
                    ev.timestampMs = dmNowMs();
                    rbusValue_Retain(pval);
                    ev.newValue = pval;
                    dmQueueOrDeliver(mgr, sub, &ev);
                }
                p = rbusProperty_GetNext(p);
            }
            rbusProperty_Release(props);
        }
    }

    /* Discovery sync: find all pre-existing elements that match this subscription in the background */
    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    sub->syncPending = true;
    ERROR_CHECK(pthread_cond_signal(&mgr->cond));
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));

    *outHandle = sub->handle;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusDataModelNotificationManager_Unsubscribe(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationHandle_t subscriptionHandle)
{
    VERIFY_NULL_RET(mgr, RBUS_ERROR_INVALID_HANDLE);

    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    dmSub_t* sub = dmFindSubByHandle(mgr, subscriptionHandle);
    if(!sub)
    {
        ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
        return RBUS_ERROR_INVALID_INPUT;
    }

    /* Remove from vector */
    rtVector_RemoveItem(mgr->subs, sub, dmSub_Free);
    if(mgr->stats.activeSubscriptions) mgr->stats.activeSubscriptions--;
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusDataModelNotificationManager_List(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationList_t* outList)
{
    VERIFY_NULL_RET(mgr, RBUS_ERROR_INVALID_HANDLE);
    VERIFY_NULL_RET(outList, RBUS_ERROR_INVALID_INPUT);
    memset(outList, 0, sizeof(*outList));

    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    size_t n = rtVector_Size(mgr->subs);
    outList->entries = (rbusDataModelNotificationListEntry_t*)rt_calloc((int)n, sizeof(rbusDataModelNotificationListEntry_t));
    if(!outList->entries)
    {
        ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }
    outList->count = n;
    for(size_t i=0;i<n;i++)
    {
        dmSub_t* s = (dmSub_t*)rtVector_At(mgr->subs, (int)i);
        outList->entries[i].handle = s->handle;
        outList->entries[i].pattern = strdup(s->pattern.original);
        outList->entries[i].eventMask = s->mask;
        outList->entries[i].scope = s->scope;
    }
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
    return RBUS_ERROR_SUCCESS;
}

void rbusDataModelNotificationManager_FreeList(rbusDataModelNotificationList_t* list)
{
    if(!list) return;
    if(list->entries)
    {
        for(size_t i=0;i<list->count;i++)
            free((void*)list->entries[i].pattern);
        rt_free(list->entries);
    }
    memset(list, 0, sizeof(*list));
}

rbusError_t rbusDataModelNotificationManager_GetStats(
    rbusDataModelNotificationManager_t mgr,
    rbusDataModelNotificationStats_t* outStats)
{
    VERIFY_NULL_RET(mgr, RBUS_ERROR_INVALID_HANDLE);
    VERIFY_NULL_RET(outStats, RBUS_ERROR_INVALID_INPUT);
    ERROR_CHECK(pthread_mutex_lock(&mgr->mutex));
    *outStats = mgr->stats;
    ERROR_CHECK(pthread_mutex_unlock(&mgr->mutex));
    return RBUS_ERROR_SUCCESS;
}

void rbusDataModelNotificationManager_OnProviderModelChanged(
    rbusHandle_t providerHandle,
    char const* changedPath,
    bool isRegistrationEvent)
{
    (void)providerHandle;
    (void)changedPath;
    (void)isRegistrationEvent;
    /* Best-effort hook. Full activation uses underlying rbusEvent retries + existing table create/delete events.
       A richer implementation can broadcast this to interested consumer managers in-process. */
}

