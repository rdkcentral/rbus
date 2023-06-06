/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#define _GNU_SOURCE

#include "rtRoutingTree.h"
#include "rtLog.h"
#include "rtList.h"
#include "rtMemory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

typedef struct Token
{
    char const* name;
    int length;
} Token;

typedef struct rtRoutingTreeStats
{
    size_t numTopics;                      /*number of rtTreeTopic in the tree, including intermediate routes*/
    size_t numTopicsWithRoutes;            /*number of rtTreeTopic that have 1 or more routes directly assigned*/
    size_t numTopicsWithMultipleRoutes;    /*number of rtTreeTopic that have more then 1 route directly assigned*/
    size_t numRoutes;                      /*number of unique routes registered in the tree*/
} rtRoutingTreeStats;

static char workBuffer[512];
static Token workTokens[32];
static int workTokenCount = 0;

static int rtList_ComparePointer(const void *left, const void *right)
{
  return !(left == right);
}

static rtTreeTopic* createTreeTopic(const char* name, rtTreeTopic* parent)
{
    rtTreeTopic* treeTopic = rt_calloc(1, sizeof(struct rtTreeTopic));
    treeTopic->parent = parent;
    treeTopic->name = strdup(name);
    if(parent->fullName)
    {
        treeTopic->fullName = rt_malloc(strlen(parent->fullName) + 1 + strlen(name) + 1);
        sprintf(treeTopic->fullName, "%s.%s", parent->fullName, name);
    }
    else
    {
        treeTopic->fullName = strdup(treeTopic->name);
    }
    if(!parent->childList)
        rtList_Create(&parent->childList);
    rtList_PushBack(parent->childList, treeTopic, NULL);

    /*Flag the parent as table if this is a '{i}' child.
      This assume that { is only used for tables.
      Although techinically 'Foo.{i}' is the table,
      rbus allows you to query the table with just 'Foo',
      so we need the isTable flag to avoid having to 
      verify {i} exists as a child inside rtRoutingTree_GetTopicRoutes
    */    
    if(name[0] == '{')
      parent->isTable = 1;

    return treeTopic;
}

static void freeTreeTopic(void* p)
{
    rtTreeTopic* treeTopic = p;
    if(treeTopic->childList)
        rtList_Destroy(treeTopic->childList, freeTreeTopic);
    if(treeTopic->routeList)
        rtList_Destroy(treeTopic->routeList, NULL);
    if(treeTopic->routeList2)
        rtList_Destroy(treeTopic->routeList2, NULL);
    if(treeTopic->name)
        free(treeTopic->name);
    if(treeTopic->fullName)
        free(treeTopic->fullName);
    free(treeTopic);
}

static void freeTreeRoute(void* p)
{
    rtTreeRoute* route = p;
    if(route->topicList)
        rtList_Destroy(route->topicList, NULL);
    free(route);
}

static void tokenizeExpression(const char* expression)
{
    int i = 0;
    const char* from = expression;
    char* to = workBuffer;
    workTokenCount = 0;
    while(*from)
    {
        workTokens[i].name = to;
        
        while(*from && *from != '.')
            *to++ = *from++;
        workTokens[i].length = to - workTokens[i].name;
        *to++ = 0;
        if(*from == '.')
            from++;
        i++;
    }
    workTokenCount = i;
}

static void removeTopicFromRoutes(rtRoutingTree rt, rtTreeTopic* topic)
{
    rtListItem childItem;
    rtTreeTopic* childTopic;
    rtListItem routeItem;
    rtTreeRoute* route;
    size_t size;
    rtList_GetFront(topic->routeList, &routeItem);
    while(routeItem)
    {
        rtListItem_GetData(routeItem, (void**)&route);

        rtList_RemoveItemByCompare(route->topicList, topic, rtList_ComparePointer, NULL);
        /*if the route's topicList has become empty, then remove the whole route*/
        rtList_GetSize(route->topicList, &size);
        if(size == 0)
            rtList_RemoveItemByCompare(rt->routeList, route, rtList_ComparePointer, freeTreeRoute);

        rtListItem_GetNext(routeItem, &routeItem);
    }

    rtList_GetFront(topic->childList, &childItem);
    while(childItem)
    {
        rtListItem_GetData(childItem, (void**)&childTopic);
        removeTopicFromRoutes(rt, childTopic);
        rtListItem_GetNext(childItem, &childItem);
    }
}

static rtTreeTopic* getChildByName(rtRoutingTree rt, rtTreeTopic* parent, const char* name, int create, int* created, int *error, int remove)
{
    rtListItem item;

    if(created)
        *created = 0;
    rtList_GetFront(parent->childList, &item);

    while(item)
    {
        rtTreeTopic* treeTopic;
        rtListItem_GetData(item, (void**)&treeTopic);
        if(strcmp(treeTopic->name, name) == 0)
        {
            /* we can remove if its an exact match*/
            if(remove)
            {
              removeTopicFromRoutes(rt, treeTopic);
              rtList_RemoveItem(parent->childList, item, freeTreeTopic);
              return NULL;
            }
            return treeTopic;
        }
        /* if a query contains a row instance id, a square bracket alias, or wildcard * 
           we need to match any curly brace i table name.  
           E.G. '1' or '2' or '[someAlias]' or '*' all match {i}
           This assumes that instance paths (paths with instance id or alias) is never registered
           with the broker.  Only the non-instance path (with {i}) gets registered.
           And the next line makes an assumption that '{' is only ever used to register tables with {i},
           and that if the first character of the name passed in is a number, then it must be a instance Id,
           and that if the first character of the name is square bracket, then is must be an alias
           and that * is only used as a wildcard.
           If a user ever tries to use { or [ or * as anything other then table register, alias, wildcard, then
           the following line will have to be modified.
        */
        else if(treeTopic->name[0] == '{' && (isdigit(name[0]) || name[0] == '[' || name[0] == '*'))
        {
            /* note that we don't handle the remove flag here because you should never create or remove instance ids themselves in the tree
               however, as a safeguard we only do remove in the above branch when name is an exact match*/
            return treeTopic;
        }
        /* Registration of property element as sibling to a table and vice versa not supported as per TR369/TR181 standard.*/
        else if (treeTopic->name[0] == '{' || name[0] == '{')
        {
            if (error)
            {
                *error = 1;
            }
            return treeTopic;
        }
        rtListItem_GetNext(item, &item);
    }
    if(create)
    {
        if(created)
            *created = 1;
        return createTreeTopic(name, parent);
    }
    else
        return NULL;
}

static rtTreeRoute* getTreeRoute(rtRoutingTree rt, const void* route, rtListItem* routeItem)
{
    rtListItem item;

    rtList_GetFront(rt->routeList, &item);
    while(item)
    {
        rtTreeRoute* entry;
        rtListItem_GetData(item, (void**)&entry);
        if(entry->route == route)
        {
            if(routeItem)
                *routeItem = item;
            return entry;
        }
        rtListItem_GetNext(item, &item);
    }

    return NULL;
}

static void addPointerToListOnce(rtList list, void* ptr)
{
    int found = 0;
    rtListItem item;
    rtList_GetFront(list, &item);
    while(item)
    {
        void* ptr2;
        rtListItem_GetData(item, (void**)&ptr2);
        if(ptr == ptr2)
        {
            found = 1;
            break;
        }
        rtListItem_GetNext(item, &item);
    }
    if(!found)
        rtList_PushBack(list, ptr, NULL);
}

static void optimizeTopicsBackpropagate(rtTreeTopic* topic, rtTreeRoute* route)
{
    if(!topic->routeList2)
        rtList_Create(&topic->routeList2);
    addPointerToListOnce(topic->routeList2, route);
    if(topic->parent->parent)
        optimizeTopicsBackpropagate(topic->parent, route);
}

/* 
    This does a depth first search for treeTopics having the route.
    If a treeTopic has the route, the route is removed from the treeTopic.
    If a treeTopic's route list is empty and the treeTopic's child list is empty we remove the treeTopic.
    We use a depth first search so we can remove treeTopic whose child and route lists
    become empty, in the correct order (deep up).
 */
static void removeRouteFromTopicTree(rtRoutingTree rt, rtTreeTopic* treeTopic, rtTreeRoute* route)
{
    rtListItem childItem;

    if(treeTopic->childList == NULL)
      return;

    rtList_GetFront(treeTopic->childList, &childItem);

    while(childItem)
    {
        rtTreeTopic* child;
        rtListItem next;
        size_t numChildren=0;
        size_t numRoutes=0;
        size_t numRoutes2=0;

        rtListItem_GetData(childItem, (void**)&child);

        removeRouteFromTopicTree(rt, child, route);

        if(child->routeList)
        {
          rtList_RemoveItemByCompare(child->routeList, route, rtList_ComparePointer, NULL);
          rtList_GetSize(child->routeList, &numRoutes);
        }
        if(child->routeList2)
        {
          rtList_RemoveItemByCompare(child->routeList2, route, rtList_ComparePointer, NULL);
          rtList_GetSize(child->routeList2, &numRoutes2);
        }
        rtList_GetSize(child->childList, &numChildren);
        rtListItem_GetNext(childItem, &next);
        if(numChildren == 0 && numRoutes == 0)
        {
            rtList_RemoveItem(treeTopic->childList, childItem, freeTreeTopic);
        }/*
        else if(numRoutes == 0 && numRoutes2 > 0)
        {
            rtList_GetFront(child->routeList2, &routeItem);
            while(routeItem)
            {
                rtListItem_GetNext(routeItem, &next);
                rtList_RemoveItem(child->routeList2, routeItem, NULL);
                routeItem = next;
            }
        }*/
        childItem = next;
    }
}

static void getTreeTopicStats(rtTreeTopic* topic, rtRoutingTreeStats* stats)
{
    rtListItem childItem;
    rtTreeTopic* child;

    if(topic->parent)
    {
        stats->numTopics++;
        if(topic->routeList)
        {
            size_t numRoutes;
            rtList_GetSize(topic->routeList, &numRoutes);
            if(numRoutes > 0)
            {
                stats->numTopicsWithRoutes++;
                if(numRoutes > 1)
                    stats->numTopicsWithMultipleRoutes++;
            }
        }
    }
    if(topic->childList)
    {
      rtList_GetFront(topic->childList, &childItem);
      while(childItem)
      {
          rtListItem_GetData(childItem, (void**)&child);
          getTreeTopicStats(child, stats);
          rtListItem_GetNext(childItem, &childItem);
      }
  }
}

static void appendLogMessage(char* buffer, size_t len, const char* format, ...)
{
    size_t len2 = strlen(buffer);
    va_list vl;
    va_start(vl,format);
    vsnprintf(buffer + len2, len -len2 -1, format, vl);
    va_end(vl);
}

static void logTreeTopic(rtTreeTopic* topic, int depth)
{
    rtListItem childItem;
    rtListItem routeItem;
    rtTreeTopic* child;
    rtTreeRoute* route;
    size_t sz1 = 0, sz2 = 0;
    char logLine[1024] = {0};

    if(topic->routeList)
        rtList_GetSize(topic->routeList, &sz1);
    if(topic->routeList2)
        rtList_GetSize(topic->routeList2, &sz2);
    appendLogMessage(logLine, 1024, "%s %d,%d [", topic->parent == NULL ? "root" : topic->fullName, (int)sz1, (int)sz2);

    if(topic->routeList)
    {
        rtList_GetFront(topic->routeList, &routeItem);
        while(routeItem)
        {
            rtListItem_GetData(routeItem, (void**)&route);
            appendLogMessage(logLine, 1024, "%p", route->route);
            rtListItem_GetNext(routeItem, &routeItem);
            if(routeItem)
                appendLogMessage(logLine, 1024, ",");
        }
    }
    appendLogMessage(logLine, 1024, "] (");
    if(topic->routeList2)
    {
        rtList_GetFront(topic->routeList2, &routeItem);
        while(routeItem)
        {
            rtListItem_GetData(routeItem, (void**)&route);
            rtListItem_GetNext(routeItem, &routeItem);
            appendLogMessage(logLine, 1024, "%p", route->route);
            if(routeItem)
                appendLogMessage(logLine, 1024, ",");
        }
    }
    appendLogMessage(logLine, 1024, ")");

    rtLog_Info("%s", logLine);

    if(topic->childList)
    {
        rtList_GetFront(topic->childList, &childItem);
        while(childItem)
        {
            rtListItem_GetData(childItem, (void**)&child);
            logTreeTopic(child, depth+1);
            rtListItem_GetNext(childItem, &childItem);
        }
    }
}

void rtRoutingTree_Create(rtRoutingTree* rt)
{
    rtLog_Debug("%s", __FUNCTION__);
    *rt = rt_malloc(sizeof(struct _rtRoutingTree));
    (*rt)->topicRoot = rt_calloc(1, sizeof(struct rtTreeTopic));/*root's name and fullName stay NULL*/
    rtList_Create(&(*rt)->routeList);
}

void rtRoutingTree_Destroy(rtRoutingTree rt)
{
    rtLog_Debug("%s", __FUNCTION__);
    rtList_Destroy(rt->routeList, freeTreeRoute);
    freeTreeTopic(rt->topicRoot);
    free(rt);
}

rtError rtRoutingTree_AddTopicRoute(rtRoutingTree rt, const char* topicPath, const void* routeData, int err_on_dup)
{
    rtError rc = RT_OK;
    int i;
    rtTreeTopic* topic = rt->topicRoot;
    rtTreeRoute* route;

    rtLog_Debug("%s: %s", __FUNCTION__, topicPath);

    tokenizeExpression(topicPath);

    if(workTokenCount == 0)
        return rc;

    route = getTreeRoute(rt, routeData, NULL);
    if(!route)
    {
        route = rt_malloc(sizeof(rtTreeRoute));
        route->route = (void*)routeData;
        rtList_Create(&route->topicList);
        rtList_PushBack(rt->routeList, route, NULL);
    }

    int isCreated = 0;
    int error = 0;
    for(i = 0; i < workTokenCount; ++i)
    {
        topic = getChildByName(rt, topic, workTokens[i].name, 1/*create missing topic*/, &isCreated, &error, 0);
        if (error)
        {
            rtLog_Debug("Rejecting data registraion as per standard");
            return RT_ERROR_PROTOCOL_ERROR;
        }
    }

    if (err_on_dup && (0 == isCreated) && (topic->routeList) && (0 != strcmp (topicPath, "_RTROUTED.ADVISORY")))
    {
        /* Controlled Error log will be printed in rtrouted */
        rtLog_Debug("Rejecting a duplicate registraion");
        return RT_ERROR_DUPLICATE_ENTRY;
    }

    if(!topic->routeList)
        rtList_Create(&topic->routeList);

    rtList_PushBack(topic->routeList, route, NULL);

    addPointerToListOnce(route->topicList, topic);

    if(topic->parent->parent)
        optimizeTopicsBackpropagate(topic->parent, route);
    return RT_OK;
}

void rtRoutingTree_GetTopicRoutes(rtRoutingTree rt, const char* topic, rtList* routes)
{
    int i;
    *routes = NULL;
    rtTreeTopic* treeTopic = rt->topicRoot;

    rtLog_Debug("%s: %s", __FUNCTION__, topic);

    tokenizeExpression(topic);
    if(workTokenCount == 0)
        return;

    for(i = 0; i < workTokenCount; ++i)
    {
        treeTopic = getChildByName(rt, treeTopic, workTokens[i].name, 0, NULL, NULL, 0);

        if(!treeTopic)
            return;

#ifdef ENABLE_ROUTE2_QUICK_GET_OPTIMIZATION
        /*
          As we walk into the tree token by token, we can end early if the treeTopic
          has 1 route in its routeList2 list, as this means that all sub topics
          share that same route.  If we cannot end early and reach the last token then
          we need further checks below to determine what to return.  
          A = [] (r1,r2)
          A.B.{i} = [r1] (r2,r3)
          A.B.{i}.C = [] (r2)
          A.B.{i}.C.D = [r2] ()
          A.B.{i}.D.E.{i} = [r3](r3)
          A.B.{i}.D.E.{i}.F = [r3]
        */
        /*This optimization works for rbus where all subnode use the same route
          but fails for rdkc where multiple routes are assigned in the subnodes.
          It fails in the use case where 2 routes service the subnodes and one
          route gets removed, leaving only 1 route in the route2 list. 
          If a client tries to send a message to the route that was removed, the
          message will go to wrong route.  Instead of getting an error that the 
          topic doesn't exist, they will get whatever the other provider gives
          them (maybe and error or invalid data). 
          Example:
            A.B.C = route1
            A.B.D = route2
            A.B routeList2 contains route1, route2
          if A.B.D/route2 is removed (or never created)
            A.B routeList2 contains route1 only
          if consumer sends message to A.B.C, they will
            get route1 when walking to A.B sinces routeList2 has 1 entry
          and so the message goes to the wrong provider.
          So for RDKC use case, if A.B.C doesn't exist, the consumer should get
          an error indicating this.
        */
        if(treeTopic->routeList2 && i < workTokenCount - 1)
        {
            size_t size = 0;
            rtList_GetSize(treeTopic->routeList2, &size);
            if(size == 1)
            {
                *routes = treeTopic->routeList2;
                return;
            }
        }
#endif
    }
    if(topic[strlen(topic)-1] == '.')
    {
        /* If its a partial path or a table send all routes listening to sub topics */
        *routes = treeTopic->routeList2;
    }
    else
    if(treeTopic->isTable)
    {
        /* If we ended on a table, then we need an additional check.
          We allow querying a table without adding the ".{i}" suffix.
          You can query "Foo.{i}" table with just "Foo".
          However, "Foo" doesn't have the route.  "Foo.{i}" has the route we want.
          So we need to search for the "{i}" child here and use its route.
        */
        rtTreeTopic* curlyChild = getChildByName(rt, treeTopic, "{i}", 0, NULL, NULL, 0);
        if(curlyChild)
        {
          *routes = curlyChild->routeList;
        }
        /*if we didn't find one then we are in some error scenario so just leave route null*/
    }
    else
    {
        /*If its not a partial path then return the routes listening to the topic*/
        *routes = treeTopic->routeList;
    }
}

void rtRoutingTree_GetRouteTopics(rtRoutingTree rt, const void* route, rtList* topics)
{
    rtTreeRoute* entry = NULL;

    rtLog_Debug("%s route=%p", __FUNCTION__, route);

    entry = getTreeRoute(rt, route, NULL);

    if(entry)
        *topics = entry->topicList;
    else
        *topics = NULL;
}

void rtRoutingTree_RemoveRoute(rtRoutingTree rt, const void* route)
{
    rtListItem routeItem = NULL;

    rtLog_Debug("%s route=%p", __FUNCTION__, route);

    rtTreeRoute* treeRoute = getTreeRoute(rt, route, &routeItem);

    if(!treeRoute)
        return;

    removeRouteFromTopicTree(rt, rt->topicRoot, treeRoute);

    rtList_RemoveItem(rt->routeList, routeItem, freeTreeRoute);
}

void rtRoutingTree_RemoveTopic(rtRoutingTree rt, const char* topic)
{
    int i;
    rtTreeTopic* treeTopic = rt->topicRoot;

    rtLog_Debug("%s topic=%s", __FUNCTION__, topic);

    tokenizeExpression(topic);
    if(workTokenCount == 0)
        return;

    for(i = 0; i < workTokenCount; ++i)
    {
        treeTopic = getChildByName(rt, treeTopic, workTokens[i].name, 0, NULL, NULL, i == workTokenCount-1);

        if(!treeTopic)
            return;
    }
}

void rtRoutingTree_ResolvePartialPath(rtRoutingTree rt, const char* partialPath, rtList topics)
{
    int i;
    rtTreeTopic* treeTopic = rt->topicRoot;

    tokenizeExpression(partialPath);
    if(workTokenCount == 0)
        return;

    for(i = 0; i < workTokenCount; ++i)
    {
        treeTopic = getChildByName(rt, treeTopic, workTokens[i].name, 0, NULL, NULL, 0);
        
        if(!treeTopic)
            return;
    }

    if(treeTopic->routeList2)
    {
        rtListItem routeItem;
        rtList_GetFront(treeTopic->routeList2, &routeItem);
        while(routeItem)
        {
            rtTreeRoute* route;
            rtListItem_GetData(routeItem, (void**)&route); 
            rtListItem_GetNext(routeItem, &routeItem);

            rtListItem topicItem;
            rtTreeTopic* topic;
            if (route->topicList)
            {
                rtList_GetFront(route->topicList, &topicItem);
                rtListItem_GetData(topicItem, (void**)&topic);
            
                rtList_PushBack(topics, topic->fullName, NULL);
            }
        }
    }
}

void rtRoutingTree_LogStats(rtRoutingTree rt)
{
    rtRoutingTreeStats stats = {0,0,0,0};
    getTreeTopicStats(rt->topicRoot, &stats);
    rtList_GetSize(rt->routeList, (size_t*)&stats.numRoutes);
    rtLog_Info("RoutingTree stats: numTopics=%d numTopicsWithRoutes=%d numTopicsWithMultipleRoutes=%d numRoutes=%d", 
        (int)stats.numTopics, (int)stats.numTopicsWithRoutes, (int)stats.numTopicsWithMultipleRoutes, (int)stats.numRoutes );
}

void rtRoutingTree_LogTopicTree(rtRoutingTree rt)
{
    rtLog_Info("RoutingTree Topic Tree:");
    logTreeTopic(rt->topicRoot, 0);
}

void rtRoutingTree_LogRouteList(rtRoutingTree rt)
{
    rtListItem routeItem;
    int i = 0;
    size_t numTopics = 0;

    rtLog_Info("RoutingTree Route List:");
    rtList_GetFront(rt->routeList, &routeItem);
    while(routeItem)
    {
        rtTreeRoute* route;
        rtListItem topicItem;
        int j = 0;

        rtListItem_GetData(routeItem, (void**)&route);
        rtListItem_GetNext(routeItem, &routeItem);
        rtList_GetSize(route->topicList, &numTopics);
        rtLog_Info("\t%d: route=%p numTopics=%d [", i++, route->route, (int)numTopics);

        rtList_GetFront(route->topicList, &topicItem);
        while(topicItem)
        {
            rtTreeTopic* topic;

            rtListItem_GetData(topicItem, (void**)&topic);
            rtListItem_GetNext(topicItem, &topicItem);
            rtLog_Info("\t\t%d: %s", j++, topic->fullName);
        }
    }
 
}
