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

#include "rbus_subscriptions.h"
#include "rbus_buffer.h"
#include "rbus_handle.h"
#include <rtMemory.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <signal.h>
#include <unistd.h>

#define VERIFY_NULL(T)         if(NULL == T){ return; }
#define CACHE_FILE_PATH_FORMAT "%s/rbus_subs_%s"

struct _rbusSubscriptions
{
    rbusHandle_t handle;
    elementNode* root;
    char* componentName;
    char* tmpDir;
    rtList subList;
};

static void rbusSubscriptions_loadCache(rbusSubscriptions_t subscriptions);
static void rbusSubscriptions_saveCache(rbusSubscriptions_t subscriptions);

int subscribeHandlerImpl(rbusHandle_t handle, bool added, elementNode* el, char const* eventName, char const* listener, int32_t componentId, int32_t interval, int32_t duration, rbusFilter_t filter);

static int subscriptionKeyCompare(rbusSubscription_t* subscription, char const* listener, int32_t componentId,  char const* eventName, rbusFilter_t filter, int32_t interval, int32_t duration)
{
    int rc;
    if((rc = strcmp(subscription->listener, listener)) == 0)
    {
        if(subscription->componentId == componentId)
        {
            if((rc = strcmp(subscription->eventName, eventName)) == 0)
            {
                if ((rc = rbusFilter_Compare(subscription->filter, filter)) == 0)
                {
                    rc = ((subscription->interval == interval) &&
                            (subscription->duration == duration)) ? 0 : 1;
                }
            }
        }
        else if(subscription->componentId < componentId)
            rc = -1;
        else
            rc = 1;
    }
    return rc;
}

static void subscriptionFree(void* p)
{
    rbusSubscription_t* sub = p;
    rtListItem item;
    VERIFY_NULL(sub);

    rtList_GetFront(sub->instances, &item);
    while(item)
    {
        elementNode* node;
        rtListItem_GetData(item, (void**)&node);
        removeElementSubscription(node, sub);
        rtListItem_GetNext(item, &item);
    }
    if(sub->tokens)
        TokenChain_destroy(sub->tokens);
    if(sub->instances)
        rtList_Destroy(sub->instances, NULL);
    free(sub->eventName);
    free(sub->listener);
    if(sub->filter)
        rbusFilter_Release(sub->filter);
    free(sub);
}

void rbusSubscriptions_create(rbusSubscriptions_t* subscriptions, rbusHandle_t handle, char const* componentName, elementNode* root, const char* tmpDir)
{
    *subscriptions = rt_malloc(sizeof(struct _rbusSubscriptions));
    (*subscriptions)->handle = handle;
    (*subscriptions)->root = root;
    (*subscriptions)->componentName = strdup(componentName);
    (*subscriptions)->tmpDir = strdup(tmpDir);
    rtList_Create(&(*subscriptions)->subList);
    rbusSubscriptions_loadCache(*subscriptions);
}

/*destroy a subscriptions registry*/
void rbusSubscriptions_destroy(rbusSubscriptions_t subscriptions)
{
    VERIFY_NULL(subscriptions);
    rtList_Destroy(subscriptions->subList, subscriptionFree);
    free(subscriptions->componentName);
    free(subscriptions->tmpDir);
    free(subscriptions);
}

static void rbusSubscriptions_onSubscriptionCreated(rbusSubscription_t* sub, elementNode* node);

/*add a new subscription*/
rbusSubscription_t* rbusSubscriptions_addSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, int32_t componentId, rbusFilter_t filter, int32_t interval, int32_t duration, bool autoPublish, elementNode* registryElem)
{
    rbusSubscription_t* sub;
    TokenChain* tokens;
  
    RBUSLOG_DEBUG("adding %s %s", listener, eventName);

    tokens = TokenChain_create(eventName, registryElem);
    if(tokens == NULL)
    {
        RBUSLOG_ERROR("invalid token chain for %s", eventName);
        return NULL;
    }
    if(!subscriptions)
        return NULL;

    sub = rt_malloc(sizeof(rbusSubscription_t));

    sub->listener = strdup(listener);
    sub->eventName = strdup(eventName);
    sub->componentId = componentId;
    sub->filter = filter;
    if(sub->filter)
        rbusFilter_Retain(sub->filter);
    sub->interval = interval;
    sub->duration = duration;
    sub->autoPublish = autoPublish;
    sub->element = registryElem;
    sub->tokens = tokens;
    rtList_Create(&sub->instances);
    rtList_PushBack(subscriptions->subList, sub, NULL);

    rbusSubscriptions_onSubscriptionCreated(sub, subscriptions->root);

    rbusSubscriptions_saveCache(subscriptions);

    return sub;
}

/*get an existing subscription by searching for its unique key [eventName, listener, filter]*/
rbusSubscription_t* rbusSubscriptions_getSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, int32_t componentId, rbusFilter_t filter, int32_t interval, int32_t duration)
{
    rtListItem item;
    rbusSubscription_t* sub;

    RBUSLOG_DEBUG("searching for %s %s", listener, eventName);

    if(!subscriptions)
        return NULL;

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);

        if(!sub)
            return NULL;
        RBUSLOG_DEBUG("comparing to %s %s", sub->listener, sub->eventName);

        if(subscriptionKeyCompare(sub, listener, componentId, eventName, filter, interval, duration) == 0)
        {
            RBUSLOG_DEBUG("found sub %s %s %d", listener, eventName, componentId);
            return sub;
        }
        rtListItem_GetNext(item, &item);
    }
    RBUSLOG_DEBUG("no sub found for %s %s", listener, eventName);

    return NULL;
}

/*remove an existing subscription*/
void rbusSubscriptions_removeSubscription(rbusSubscriptions_t subscriptions, rbusSubscription_t* sub)
{
    rtListItem item;
    rbusSubscription_t* sub2;

    VERIFY_NULL(subscriptions);
    VERIFY_NULL(sub);
    RBUSLOG_DEBUG("%s %s", sub->listener, sub->eventName);

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub2);
        if(sub == sub2)
        {
            RBUSLOG_DEBUG("removing %s %s", sub->listener, sub->eventName);
            rtList_RemoveItem(subscriptions->subList, item, subscriptionFree);
            break;
        }
        rtListItem_GetNext(item, &item);
    }    
    rbusSubscriptions_saveCache(subscriptions);
}

/*  called after a new subscription is created 
 *  we go through the element tree and check to see if the 
 *  new subscription matches any existing instance nodes
 *  e.g. if subscribing to Foo.*.Prop, this will find all instances of Prop 
 */
static void rbusSubscriptions_onSubscriptionCreated(rbusSubscription_t* sub, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            if(child->type != 0)
            {
                if(TokenChain_match(sub->tokens, child))
                {
                    rtList_PushBack(sub->instances, child, NULL);
                    addElementSubscription(child, sub, false);
                }
            }

            /*recurse into children except for row templates {i}*/
            if( child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0) )
            {
                
                rbusSubscriptions_onSubscriptionCreated(sub, child);
            }

            child = child->nextSibling;
        }
    }
}

/*  called after a new node instance is created 
 *  we go through the list of subscriptions and check to see if the 
 *  new node is picked up by any subscription eventName
 */
static void rbusSubscriptions_onElementCreated(rbusSubscriptions_t subscriptions, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            if(child->type == 0)
            {
                rbusSubscriptions_onElementCreated(subscriptions, child);
            }
            else
            {
                rtListItem item;
                rbusSubscription_t* sub;

                rtList_GetFront(subscriptions->subList, &item);

                while(item)
                {
                    rtListItem_GetData(item, (void**)&sub);

                    if(sub->tokens/*tokens can NULL when loaded from cache*/ && 
                       TokenChain_match(sub->tokens, child))
                    {
                        rtList_PushBack(sub->instances, child, NULL);
                        addElementSubscription(child, sub, false);
                    }
                    rtListItem_GetNext(item, &item);
                }

                /*we dont recurse into child because either child is a leaf (e.g. property/method/event)
                  or child is a table that doesn't have any rows yet, since its brand new */
            }

            child = child->nextSibling;
        }
    }
}

/*called before a node instance is deleted */
void rbusSubscriptions_onElementDeleted(rbusSubscriptions_t subscriptions, elementNode* node)
{
    if(node)
    {
        elementNode* child = node->child;

        while(child)
        {
            /*if child's type is a subscribable type*/
            if(child->type != 0)
            {
                rtListItem item, tempitem;
                rbusSubscription_t* sub;
                bool val=false;
                rbusSubscription_t* subscription = NULL;

                rtList_GetFront(subscriptions->subList, &item);

                /* go through all subscriptions */
                while(item)
                {
                    rtListItem item2;
                    elementNode* inst;

                    rtListItem_GetNext(item, &tempitem);
                    rtListItem_GetData(item, (void**)&sub);

                    rtList_GetFront(sub->instances, &item2);

                    /* go through all instances in this subscription */
                    while(item2)
                    {
                        rtListItem_GetData(item2, (void**)&inst);

                        if(child == inst)
                        {
                            rtList_RemoveItem(sub->instances, item2, NULL);
                            removeElementSubscription(child, sub);
                            val = true;
                            break;
                        }
                        rtListItem_GetNext(item2, &item2);
                    }

                    /* RDKB-38389 : Removing the instance of the row to be removed from the subscriptions->subList linked list */
                    if(val)
                    {
                        subscription = rbusSubscriptions_getSubscription(subscriptions, sub->listener, sub->eventName,
                                sub->componentId, sub->filter, sub->interval, sub->duration);
                        if(!subscription)
                        {
                            RBUSLOG_INFO("unsubscribing from event which isn't currectly subscribed to event=%s listener=%s", sub->eventName, sub->listener);
                        }
                        else
                        {
                            rbusSubscriptions_removeSubscription(subscriptions, subscription);
                        }
                    }
                    item = tempitem;
                }
            }

            /*recurse into children except for row templates {i}*/
            if( child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0) )
            {
                rbusSubscriptions_onElementDeleted(subscriptions, child);
            }

            child = child->nextSibling;
        }
    }
}

void rbusSubscriptions_onTableRowAdded(rbusSubscriptions_t subscriptions, elementNode* node)
{
    VERIFY_NULL(subscriptions);
    rbusSubscriptions_onElementCreated(subscriptions, node);
}

void rbusSubscriptions_onTableRowRemoved(rbusSubscriptions_t subscriptions, elementNode* node)
{
    VERIFY_NULL(subscriptions);
    rbusSubscriptions_onElementDeleted(subscriptions, node);
}

static pid_t rbusSubscriptions_getListenerPid(char const* listener)
{
    pid_t pid;
    const char* p = listener + strlen(listener) - 1;
    while(*p != '.' && p != listener)
    {
        p--;
    }
    pid = atoi(p+1);
    if(pid == 0)
    {
        RBUSLOG_ERROR("pid not found in listener name %s", listener);
    }
    return pid;
}

static bool rbusSubscriptions_isProcessRunning(pid_t pid)
{
    /*sending 0 signal to kill is used to check if process running*/
    int rc = kill(pid, 0);
    RBUSLOG_DEBUG("kill check for pid %d returned %d", pid, rc);
    return rc == 0;
}

static bool rbusSubscriptions_isListenerRunning(char const* listener)
{
    /*this assumes rtMessage is appending the pid to the inbox name
      get pid and verify a process with that pid is actually running.
    */
    pid_t pid = rbusSubscriptions_getListenerPid(listener);
    if(pid > 0)
    {
        return rbusSubscriptions_isProcessRunning(pid);
    }
    /* if there's no pid appended to listener then rtMessage was updated to not append the pid
       which makes it impossible to know what a particular component's process actually is.
       so, we'll have to figure out another  solution if this every happens.
       assume its running by returning true*/
    return true;
}

static void rbusSubscriptions_loadCache(rbusSubscriptions_t subscriptions)
{
    struct stat st;
    long size;
    uint16_t type, length;
    int32_t hasFilter;
    FILE* file = NULL;
    rbusBuffer_t buff = NULL;
    rbusSubscription_t* sub = NULL;
    char filePath[256];
    bool needSave = false;
    VERIFY_NULL(subscriptions);

    snprintf(filePath, 256, CACHE_FILE_PATH_FORMAT, subscriptions->tmpDir, subscriptions->componentName);

    RBUSLOG_INFO("file %s", filePath);

    if(stat(filePath, &st) != 0)
    {
        RBUSLOG_DEBUG("file doesn't exist %s", filePath);
        return;
    }

    file = fopen(filePath, "rb");
    if(!file)
    {
        RBUSLOG_ERROR("failed to open file %s", filePath);
        goto remove_bad_file;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if(size <= 0)
    {
        RBUSLOG_DEBUG("file is empty %s", filePath);
        goto remove_bad_file;
    }

    rbusBuffer_Create(&buff);
    VERIFY_NULL(buff);
    rbusBuffer_Reserve(buff, size);

    fseek(file, 0, SEEK_SET);
    if(fread(buff->data, 1, size, file) != (size_t)size)
    {
        RBUSLOG_ERROR("failed to read entire file %s", filePath);
        goto remove_bad_file;
    }

    fclose(file);
    file = NULL;

    buff->posWrite += size;

    while(buff->posRead < buff->posWrite)
    {
        sub = (rbusSubscription_t*)rt_try_calloc(1, sizeof(struct _rbusSubscription));
        if(!sub)
        {
            RBUSLOG_ERROR("failed to malloc sub");
            goto remove_bad_file;
        }
        //read listener
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_STRING || length >= RBUS_MAX_NAME_LENGTH) goto remove_bad_file;

        sub->listener = rt_try_malloc(length);
        if(!sub->listener)
        {
            RBUSLOG_ERROR("failed to malloc %d bytes for listener", length);
            goto remove_bad_file;
        }
        memcpy(sub->listener, buff->data + buff->posRead, length);
        buff->posRead += length;

        //read eventName
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_STRING || length >= RBUS_MAX_NAME_LENGTH) goto remove_bad_file;

        sub->eventName = rt_try_malloc(length);
        if(!sub->eventName)
        {
            RBUSLOG_ERROR("failed to malloc %d bytes for eventName", length);
            goto remove_bad_file;
        }
        memcpy(sub->eventName, buff->data + buff->posRead, length);
        buff->posRead += length;

        //read componentId
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_INT32 && length != sizeof(int32_t)) goto remove_bad_file;
        if(rbusBuffer_ReadInt32(buff, &sub->componentId) < 0) goto remove_bad_file;

        //read interval
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_INT32 && length != sizeof(int32_t)) goto remove_bad_file;
        if(rbusBuffer_ReadInt32(buff, &sub->interval) < 0) goto remove_bad_file;

        //read duration        
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_INT32 && length != sizeof(int32_t)) goto remove_bad_file;
        if(rbusBuffer_ReadInt32(buff, &sub->duration) < 0) goto remove_bad_file;

        //read autoPublish
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_INT32 && length != sizeof(int32_t)) goto remove_bad_file;
        if(rbusBuffer_ReadInt32(buff, (int*)&sub->autoPublish) < 0) goto remove_bad_file;

        //read hasFilter
        if(rbusBuffer_ReadUInt16(buff, &type) < 0) goto remove_bad_file;
        if(rbusBuffer_ReadUInt16(buff, &length) < 0) goto remove_bad_file;
        if(type != RBUS_INT32 && length != sizeof(int32_t)) goto remove_bad_file;
        if(rbusBuffer_ReadInt32(buff, &hasFilter) < 0) goto remove_bad_file;

        //read filter
        if(hasFilter)
        {
            if(rbusFilter_Decode(&sub->filter, buff) < 0) goto remove_bad_file;
        }
        else
        {
            sub->filter = NULL;
        }
        /*
            It's possible that we can load a sub from the cache for a listener whose process is no longer running.
            Example, this provider exited with active subscribers and thus still had those subs in its cache.
            Later those listener processes exit/crash.
            Then after that, this provider restarts and reads those now obsolete listeners. 
         */
        if(!rbusSubscriptions_isListenerRunning(sub->listener))
        {
            RBUSLOG_INFO("process no longer running for listener %s", sub->listener);
            subscriptionFree(sub);
            needSave = true;
            continue;
        }
        else
        {
            char filename[RTMSG_HEADER_MAX_TOPIC_LENGTH];
            snprintf(filename, RTMSG_HEADER_MAX_TOPIC_LENGTH-1, "%s%d_%d", "/tmp/.rbus/",
                    rbusSubscriptions_getListenerPid(sub->listener), sub->componentId);
            if(access(filename, F_OK) != 0)
            {
                RBUSLOG_ERROR("file doesn't exist %s", filename);
                subscriptionFree(sub);
                needSave = true;
                continue;
            }
        }

        rtList_Create(&sub->instances);
        rtList_PushBack(subscriptions->subList, sub, NULL);

        RBUSLOG_INFO("loaded %s %s", sub->listener, sub->eventName);
    }

    rbusBuffer_Destroy(buff);

    if(needSave)
        rbusSubscriptions_saveCache(subscriptions);

    return;

remove_bad_file:

    RBUSLOG_WARN("removing corrupted file %s", filePath);

    if(file)
        fclose(file);

    if(buff)
        rbusBuffer_Destroy(buff);

    if(sub)
        free(sub);

    if(remove(filePath) != 0)
        RBUSLOG_ERROR("failed to remove %s", filePath);
}

static void rbusSubscriptions_saveCache(rbusSubscriptions_t subscriptions)
{
    FILE* file;
    rbusBuffer_t buff;
    rtListItem item;
    rbusSubscription_t* sub;
    char filePath[256];

    snprintf(filePath, 256, CACHE_FILE_PATH_FORMAT, subscriptions->tmpDir, subscriptions->componentName);

    RBUSLOG_DEBUG("saving Cache %s", filePath);

    rtList_GetFront(subscriptions->subList, &item);

    if(!item)
    {
        RBUSLOG_DEBUG("no subs so removing file %s", filePath);

        if(remove(filePath) != 0)
            RBUSLOG_ERROR("failed to remove %s", filePath);

        return;
    }

    file = fopen(filePath, "wb");

    if(!file)
    {
        RBUSLOG_ERROR("failed to open %s", filePath);
        return;
    }

    rbusBuffer_Create(&buff);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);
        if(!sub)
            return;
        rbusBuffer_WriteStringTLV(buff, sub->listener, strlen(sub->listener)+1);
        rbusBuffer_WriteStringTLV(buff, sub->eventName, strlen(sub->eventName)+1);
        rbusBuffer_WriteInt32TLV(buff, sub->componentId);
        rbusBuffer_WriteInt32TLV(buff, sub->interval);
        rbusBuffer_WriteInt32TLV(buff, sub->duration);
        rbusBuffer_WriteInt32TLV(buff, sub->autoPublish);
        rbusBuffer_WriteInt32TLV(buff, sub->filter ? 1 : 0);
        if(sub->filter)
          rbusFilter_Encode(sub->filter, buff);

        RBUSLOG_DEBUG("saved %s %s", sub->listener, sub->eventName);

        rtListItem_GetNext(item, &item);
    }

    fwrite(buff->data, 1, buff->posWrite, file);

    rbusBuffer_Destroy(buff);

    fclose(file);
}

/*this is basicially a strcmp with the addition that it will ignore any wildcard (e.g. "*") in the event name
  if there's a corresponding table row tag (e.g. "{i}") in the element name */
static int _compareEventNameToElemName(char const* event, char const* elem)
{
    const char* p1 = event;
    const char* p2 = elem;
    for(;;)
    {
        if(*p1)
        {
            if(*p2)
            {
                if(*p1 == *p2)
                {
                    p1++;
                    p2++;
                }
                else
                {
                    if(*p1 == '*')
                    {
                        if(strncmp(p2, "{i}", 3) == 0)
                        {
                            p1++;
                            p2+=3;
                        }
                        else
                            return 1;
                    }
                    else
                        return 1;
                }
            }
            else
                return 1;
        }
        else
        {
            if(*p2)
                return 1;
            else
                return 0;
        }
    }
}

void rbusSubscriptions_resubscribeElementCache(rbusHandle_t handle, rbusSubscriptions_t subscriptions, char const* elementName, elementNode* el)
{
    rtListItem item;
    rbusSubscription_t* sub;

    VERIFY_NULL(subscriptions);
    VERIFY_NULL(el);
    RBUSLOG_DEBUG("event %s", elementName);

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);

        VERIFY_NULL(sub);

        if(sub->element == NULL && sub->tokens == NULL &&/*not already subscribed*/
           _compareEventNameToElemName(sub->eventName, elementName) == 0)
        {
            rtListItem next;
            rbusError_t err;
            RBUSLOG_INFO("resubscribing %s for %s", sub->eventName, sub->listener);
            rtListItem_GetNext(item, &next);
            rtList_RemoveItem(subscriptions->subList, item, NULL);/*remove before calling subscribeHandlerImpl to avoid dupes in cache file*/
            err = subscribeHandlerImpl(handle, true, el, sub->eventName, sub->listener, sub->componentId, sub->interval, sub->duration, sub->filter);
            /*TODO figure out what to do if we get an error resubscribing
            It's conceivable that a provider might not like the sub due to some state change between this and the previous process run
            */
            (void)err;
            subscriptionFree(sub);
            item = next;
        }
        else
        {
            rtListItem_GetNext(item, &item);
        }
    }
}

void rbusSubscriptions_resubscribeRowElementCache(rbusHandle_t handle, rbusSubscriptions_t subscriptions, elementNode* rowNode)
{
    rtListItem item = NULL;
    rbusSubscription_t* sub = NULL;
    elementNode* el = NULL;
    rbusError_t err = RBUS_ERROR_SUCCESS;
    size_t size = 0;
    unsigned int count = 0;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    if(subscriptions)
    {
        rtList_GetFront(subscriptions->subList, &item);
        rtList_GetSize(subscriptions->subList, &size);
    }

    while(item && (count < size))
    {
        /* This count matters otherwise, the sublist will go on forever as we push the new subscription at the end of the list */
        count++;
        rtListItem_GetData(item, (void**)&sub);
        if(sub)
        {
            rtListItem next = NULL;
            rtListItem_GetNext(item, &next);
            if(strncmp(sub->eventName, rowNode->fullName, strlen(rowNode->fullName)) == 0)
            {
                el = retrieveInstanceElement(handleInfo->elementRoot, sub->eventName);
                if(el)
                {
                    RBUSLOG_INFO("resubscribing %s for %s", sub->eventName, sub->listener);
                    rtList_RemoveItem(subscriptions->subList, item, NULL);
                    err = subscribeHandlerImpl(handle, true, el, sub->eventName, sub->listener, sub->componentId, sub->interval, sub->duration, sub->filter);
                    (void)err;
                    subscriptionFree(sub);
                }
            }

            /* Move to Next */
            item = next;
        }
        else
            break;
    }
}

void rbusSubscriptions_handleClientDisconnect(rbusHandle_t handle, rbusSubscriptions_t subscriptions, char const* listener)
{
    rtListItem item;
    rbusSubscription_t* sub;
    elementNode* el = NULL;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    VERIFY_NULL(subscriptions);
    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, listener);

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);
        rtListItem_GetNext(item, &item);
        if(strcmp(sub->listener, listener) == 0)
        {
            /* RDKB-38389 : Checking for elementnode existence for which the eventname is subscribed */
            el = retrieveInstanceElement(handleInfo->elementRoot, sub->eventName);
            if(el)
            {
                subscribeHandlerImpl(handle, false, sub->element, sub->eventName, sub->listener, sub->componentId, 0, 0, 0);
            }
            else
            {
                RBUSLOG_WARN("rbusSubscriptions_handleClientDisconnect: unexpected! element not found");
            }
        }
    }
}

#if 0
/*removes subscriptions for listeners whose processes are no longer running (e.g. listener crash handling)
  we are using _client_disconnect_callback_handler to detect when listener crashes and that may be good enough
  but i'm leaving this function in case we need a way to guarantee any dead listener gets removed */
static void rbusSubscriptions_cleanupDeadListeners(rbusHandle_t handle, rbusSubscriptions_t subscriptions, void(*unsubscribe)(rbusHandle_t, rbusSubscription_t*)
{
    rtListItem item;
    rbusSubscription_t* sub;

    RBUSLOG_DEBUG("%s", __FUNCTION__);

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        const char* p;
        pid_t pid;

        rtListItem_GetData(item, (void**)&sub);
        rtListItem_GetNext(item, &item);

        pid = rbusSubscriptions_getListenerPid(sub->listener);
        if(pid > 0)
        {
            if(!rbusSubscriptions_isProcessRunning(pid))
            {
                subscribeHandlerImpl(handle, false, sub->el, sub->eventName, sub->listener, 0, 0, 0);
            }
        }
    }
}

/*similar to rbusSubscriptions_getSubscription, checks for existing subscription by searching for its unique key [listener, eventName, filter]
  additionally, if an existing is found, its listner name is updated to the new listener name (which may have a different pid appended)*/
rbusSubscription_t* rbusSubscriptions_updateExisting(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, rbusFilter_t filter)
{
    rtListItem item;
    rbusSubscription_t* sub;

    RBUSLOG_DEBUG("%s: update %s %s", __FUNCTION__, listener, eventName);

    rtList_GetFront(subscriptions->subList, &item);

    while(item)
    {
        rtListItem_GetData(item, (void**)&sub);

#if 1 //IGNORE_PID_WHEN_CHECKING_DUPLICATES
        if(strcmp(sub->eventName, eventName) == 0)
        {
            /* This tries to handle the case where a subscriber crashes and 
               restarts with a new pid appended to its inbox name.
               The rtMessage inbox name is what we call listener here.  
               If subscriber Foo with inbox=Foo.1234 subscribes but then crashes, 
               we don't know about the crash and continue to keep Foo.1234 in our list.
               If Foo restarts with a new inbox=Foo.4567 and subscribes
               we want to reuse the pre-existing subscription, updating only the name.
               We have to check that the pre-existing listener's process is actually
               no longer running, as there could be cases where multiple components
               with the same name are actually intentially being run (e.g. t2 and dmcli).
             */
            const char* p1 = sub->listener + strlen(sub->listener) - 1;
            const char* p2 = listener + strlen(listener) - 1;
            pid_t pid1, pid2;
            while(*p1 != '.' && p1 != listener)
                p1--;
            while(*p2 != '.' && p2 != listener)
                p2--;
            pid1 = atoi(p1+1);
            pid2 = atoi(p2+1)
            if( pid1 > 0 && pid2 > 0 )
            {
                int l1 = (int)(p1 - sub->listener);
                int l2 = (int)(p2 - listener);

                if( strncmp(sub->listener, listener, l1 >= l2 ? l1 : l2) == 0 && 
                    rbusFilter_Compare(sub->filter, filter) == 0)
                {   
                    /*don't allow the the same component from the same process with the same filter
                      to subscribe to the same event more then once*/
                    if(pid1 == pid2) 
                    {
                        return sub;
                    }
                    /*if the preexisting listener process is not running then its a good
                      chance this new item is from the restart of a crashed process.
                      of course its possible that we could have multiple crashes of processes
                      hosting the same component, and in this case we might have several
                      pre-existing items that could match the current one, but its fine
                      to take the first one we come across in this list.
                     */
                    if(rbusSubscriptions_isProcessRunning(p1) != 0)
                    {
                        /*update name which has new pid*/
                        RBUSLOG_INFO("%s: updating duplicate name from %s to %s", __FUNCTION__, sub->listener, listener);

                        free(sub->listener);
                        sub->listener = strdup(listener);

                        /*must save with new name in case this provider crashes*/
                        rbusSubscriptions_saveCache(subscriptions);
                        return sub;
                    }
                }
            }
            else
            {
                /*rtMessage should be appending pids, if not then we are in some error state*/
                RBUSLOG_ERROR("%s: pid not found in a listener name %s %s", __FUNCTION__, listener, sub->listener);
            }
        }
#else
        if(subscriptionKeyCompare(sub, listener, eventName, filter) == 0)
        {
            return sub;
        }
#endif
        rtListItem_GetNext(item, &item);
    }
    return NULL;
}
#endif

