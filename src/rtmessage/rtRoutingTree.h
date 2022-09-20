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
#ifndef __RT_ROUTETREE_H__
#define __RT_ROUTETREE_H__

#include "rtList.h"

/*

rtRoutingTree: 
    Provides support for vanilla pub/sub with multiple routes assigned to the same topic.
    Provides backward compatibility for rbus which restricts one route per topic.
    Provides support for tr-069 styles expressions containing wildcard('*') or 
    instance ids (including aliases "[alias]") in place of the topic name "{i}".

    Replace rtree API as follows:
        API name replacements:
            rtRoutingTree_Create                <= rtree_initialize
            rtRoutingTree_AddTopicRoute         <= rtree_set_value
            rtRoutingTree_GetRouteTopics        <= rtree_get_all_nodes_matching_value 
            rtRoutingTree_GetTopicRoutes        <= rtree_lookup_value
            rtRoutingTree_ResolvePartialPath    <= rtree_get_uniquely_resolvable_endpoints_for_expression
            rtRoutingTree_RemoveRoute           <= rtree_remove_nodes_matching_value
            rtRoutingTree_RemoveTopic           <= rtree_remove_value
            rtRoutingTree_LogStats              <= rtree_get_stats
            rtRoutingTree_LogTopicTree          <= rtree_traverse_and_log
        API new functions:
            rtRoutingTree_Destroy
            rtRoutingTree_LogRouteList
        API removed functions
            rtree_dump_quick_match_expressions  : new tree does lookups differently
            rtree_set_routing_strategy          : new tree uses one strategy

    Test result showing difference in API results:

        rtRoutingTree_ResolvePartialPath    <= rtree_get_uniquely_resolvable_endpoints_for_expression
            rtRoutingTree_ResolvePartialPath will return the first topic registered for each route, which 
                for the rbus case will be the 'component id' registerd during rbus_open.
            rtree_get_uniquely_resolvable_endpoints_for_expression will return a different topic then the first one
                registered.
            This should not break anything as the topic being returned is only used to route a request and not
                used after the route reaches the provider.
        
        rtRoutingTree_GetRouteTopics        <= rtree_get_all_nodes_matching_value 
            rtRoutingTree_GetRouteTopics will include table topic which have routes registered for them.
            rtree_get_all_nodes_matching_value doesn't include table topics.

    Comments:
        rtree_get_uniquely_resolvable_endpoints_for_expression
            given a partial path expression where the expression is simply a topic followed by a dot,
            find that topic and then identify all routes that have been assigned
            either to that topic or to any descendent topics (e.g. the sub tree branching from topic).
            For each route, find 1 topic name which is owned exclusively by that route.
            Return the list of topic names, 1 per route.
            The topic names can be any one, so long as the topic is a part of expressions subtree
            and the topic (and its subtree) is exclusively owned by the route.
 
            One issue with rtree_get_uniquely_resolvable_endpoints_for_expression is that does not have
            any concept of a tr-069 style table, where {i} represents a table row placeholder.
            So a provider might register a topic like "Device.Table.{i}" and the leaf topic node for that table
            will be a literal "{i}".  Any expression that has either a wildcard (Device.Table.*.Foo) or
            and instance ID (Device.Table.1.Foo or Device.Table.[alias].Foo) will fail because
            the rtree does a strcmp on each token in the expression with each topic it walks through.
            So it will get to the {i} node and do a strcmp(node->name(e.g."{i}") , token(e.g. (*, or 1, or [alias])) 
            and fail and then return 0 results.  This type of issue is something we'd like to fix.

        To be backward compatible with ccsp we must support how rtree_get_uniquely_resolvable_endpoints_for_expression worked.
            Here is how rtree_get_uniquely_resolvable_endpoints_for_expression is used currently.
            rbus_getExt is called with parameter = "Device.".   This is a partial path query.
            rbus_resolveWildcardDestinations ("Device.") is called which leads to rtrouted calling 
            rtree_get_uniquely_resolvable_endpoints_for_expression("Device.")
            Assume Component A has Device.WiFi and a bunch of stuff under it.
            Assume Component B has Device.Moca and a bunch of stuff under it.
            rbus_resolveWildcardDestinations should return 2 topic names.  
            One topic name for Component A, which could be Device.WiFi or any topic name under Device.WiFi (e.g. Device.WiFi.A.B.C) as it 
            doesn't matter.  And a second topic for component B being Device.Moca or something inside that.
            Lets call these returned topics, destinationTopics.

            Next for each destinationTopics topic, rbus_getExt will create an rtMessage containing the actual get parameter ("Device.")
            and call rbus_invokeRemoveMethod(destinationTopics[i], METHOD_GETPARAMETERVALUES, message...)
            Note that that destinationTopics can be any alias that can route me to each component.  

        Current API confussion:
            This style and more specifially the naming syntax can be a little confusing. 
            rbus_resolveWildcardDestinations is really rbus_getRoutesForQuery and it ought to just return a list of routes.
            Instead it returns topics which can be used to find the route when sent to rtrouted.
            If it wants to return aliases/topics then why not just the first one the component registers during rbus_open.
            The fact that the topic returned is seamingly random is confussing until you understand thats its only used
            to route the rbus_invokeRemoteMethod call. But its still confusing because on the provider side, the message header topic
            will be this random topic name owned by that component. 
            To be continue ...

        This new tree must support multiple listeners on the same topic.
            Rbus restricted one listener per topic.  The listener is the provider, which is listening for requests to come in with messages that
            when read covery the various rbus api calls a consumer can make.  So the provider, instead of publishing is actually listening and
            consumers are publishing that they want to get/set some parameter or add/rm a table row ect...  rbus provider can publish
            events back to consumers but the provider gets informed of subscribers through its api and essentially knows about each subscriber
            This couples providers/consumers and is something we'd like to explore decoupling.  It is unclear currently
            how to decouple because we want to allow consumer to subscribe with filters that should only be triggered when matched.
            This means the provider needs to know about a subscribers filter.
            To be continue ...

        Rbus one listener per topic design will work with this new API because in rbus providers won't register topics they don't 'own'.
        If 2 providers did happen to register the same topic, then consumer wouldn't be able to call both.  It is unclear exactly what
        would happen but presumably, only the first route to have registered for that topic would receive a message.
        This api does not restrict anything for the rbus use case so it is up to rbus providers to be careful not to share topics.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtTreeRoute
{
    void* route;  
    rtList topicList;       /*list of rtTreeTopic. list of all topics this route is directly assigned to*/
} rtTreeRoute;

typedef struct rtTreeTopic
{
    struct rtTreeTopic* parent;
    rtList childList;       /*list of rtTreeTopic: list of child topics*/
    rtList routeList;       /*list of rtTreeRoute: list of routes directly assigned to this topic*/
    rtList routeList2;      /*list of rtTreeRoute: list of all routes either assigned to this topic or assigned to a descendents topic (except leaf nodes)*/
    char* name;
    char* fullName;
    int isTable;
} rtTreeTopic;

typedef struct _rtRoutingTree
{
    rtTreeTopic* topicRoot; /*tree of rtTreeTopic: topic tree built from all topics assigned to this tree*/
    rtList routeList;       /*list of rtTreeRoute: list of all routes assigned in this tree*/
} *rtRoutingTree;

void rtRoutingTree_Create(rtRoutingTree* rt);
void rtRoutingTree_Destroy(rtRoutingTree rt);
rtError rtRoutingTree_AddTopicRoute(rtRoutingTree rt, const char* topic, const void* route, int err_on_dup);
void rtRoutingTree_GetTopicRoutes(rtRoutingTree rt, const char* topic, rtList* routes);
void rtRoutingTree_GetRouteTopics(rtRoutingTree rt, const void* route, rtList* topics);
void rtRoutingTree_RemoveRoute(rtRoutingTree rt, const void* route);
void rtRoutingTree_RemoveTopic(rtRoutingTree rt, const char* topic);
void rtRoutingTree_ResolvePartialPath(rtRoutingTree rt, const char* partialPath, rtList topics);
void rtRoutingTree_LogStats(rtRoutingTree rt);
void rtRoutingTree_LogTopicTree(rtRoutingTree rt);
void rtRoutingTree_LogRouteList(rtRoutingTree rt);

#ifdef __cplusplus
}
#endif
#endif
