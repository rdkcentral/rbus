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

#ifndef RBUS_SUBSCRIPTIONS_H
#define RBUS_SUBSCRIPTIONS_H

#include "rbus_element.h"
#include "rbus_tokenchain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rbusSubscriptions *rbusSubscriptions_t;

/* The unique 'key' for a subscription is [listener, eventName, filter]
    meaning a subscriber can subscribe to the same event with different filters
 */
typedef struct _rbusSubscription
{
    char* listener;             /* the subscriber's address to publish to*/
    char* eventName;            /* the event name subscribed to e.g. Device.WiFi.AccessPoint.1.AssociatedDevice.*.SignalStrength */
    int32_t componentId;     /* the id known by the subscriber and unique per listener/process */
    rbusFilter_t filter;        /* optional filter */
    int32_t interval;           /* optional interval */
    int32_t duration;           /* optional duration */
    bool autoPublish;           /* auto publishing */
    TokenChain* tokens;         /* tokenized eventName for pattern matching */
    elementNode* element;       /* the registation element e.g. Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength */
    rtList instances;           /* the instance elements e.g.   Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength
                                                                Device.WiFi.AccessPoint.1.AssociatedDevice.2.SignalStrength
                                                                Device.WiFi.AccessPoint.2.AssociatedDevice.1.SignalStrength */
} rbusSubscription_t;

/*create a new subscriptions registry for an rbus handle*/
void rbusSubscriptions_create(rbusSubscriptions_t* subscriptions, rbusHandle_t handle, char const* componentName, elementNode* root, char const* tmpDir);

/*destroy a subscriptions registry*/
void rbusSubscriptions_destroy(rbusSubscriptions_t subscriptions);

/*add a new subscription with unique key [listener, eventName, filter] and the corresponding*/
rbusSubscription_t* rbusSubscriptions_addSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, int32_t componentId, rbusFilter_t filter, int32_t interval, int32_t duration, bool autoPublish, elementNode* registryElem);

/*get an existing subscription by searching for its unique key [listener, eventName, filter]*/
rbusSubscription_t* rbusSubscriptions_getSubscription(rbusSubscriptions_t subscriptions, char const* listener, char const* eventName, int32_t componentId, rbusFilter_t filter, int32_t interval, int32_t duration);

/*remove an existing subscription*/
void rbusSubscriptions_removeSubscription(rbusSubscriptions_t subscriptions, rbusSubscription_t* sub);

/*call right after a new row is added*/
void rbusSubscriptions_onTableRowAdded(rbusSubscriptions_t subscriptions, elementNode* node);

/*call right before an existing row is delete*/
void rbusSubscriptions_onTableRowRemoved(rbusSubscriptions_t subscriptions, elementNode* node);

/*call when registering an event data element to resubscribe any listeners that might have been loaded from cache */
void rbusSubscriptions_resubscribeElementCache(rbusHandle_t handle, rbusSubscriptions_t subscriptions, char const* elementName, elementNode* el);

/*call when registering row of a table, to resubscribe any listeners that might have been loaded from cache */
void rbusSubscriptions_resubscribeRowElementCache(rbusHandle_t handle, rbusSubscriptions_t subscriptions, elementNode* rowNode);

/*unsubscribe any client when they disconnect from broker. handles cases where clients don't unsubscribe properly (e.g. because they crashed)*/
void rbusSubscriptions_handleClientDisconnect(rbusHandle_t handle, rbusSubscriptions_t subscriptions, char const* listener);

#ifdef __cplusplus
}
#endif
#endif
