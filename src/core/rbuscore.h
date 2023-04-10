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
#ifndef __RBUS_CORE_H__
#define __RBUS_CORE_H__

#include <rtMessageHeader.h>
#include <rtConnection.h>
#include "rbuscore_types.h"
#include "rbuscore_message.h"
#include <rtm_discovery_api.h>

#define MAX_OBJECT_NAME_LENGTH RTMSG_HEADER_MAX_TOPIC_LENGTH
#define MAX_METHOD_NAME_LENGTH 64
#define MAX_EVENT_NAME_LENGTH MAX_OBJECT_NAME_LENGTH
#define MAX_SUPPORTED_METHODS 32
#define MAX_REGISTERED_OBJECTS 64
#define RBUS_OPEN_TELEMETRY_DATA_MAX 512

void rbus_getOpenTelemetryContext(const char **s, const char **t);
void rbus_setOpenTelemetryContext(const char *s, const char *t);
void rbus_clearOpenTelemetryContext();

#ifdef __cplusplus
extern "C" {
#endif
typedef int (*rbus_callback_t)(const char * destination, const char * method, rbusMessage in, void * user_data, rbusMessage *out, const rtMessageHeader* hdr);
typedef int (*rbus_async_callback_t)(rbusMessage message, void * user_data);
typedef int (*rbus_event_callback_t)(const char * object_name,  const char * event_name, rbusMessage message, void * user_data);
typedef int (*rbus_timed_update_event_callback_t)(rbusMessage *message);
typedef int (*rbus_event_subscribe_callback_t)(const char * object_name,  const char * event_name, const char * listener, int added, const rbusMessage payload, void * user_data);
typedef void (*rbus_client_disconnect_callback_t)(const char * name);

typedef struct
{
    const char *method;
    void * user_data;
    rbus_callback_t callback;
} rbus_method_table_entry_t;

/*------ Common bus access APIs. ------*/

/* Establish a connection with the daemon/broker and register on the bus with a component name. You can send/receive messages after this.*/
rbusCoreError_t rbus_openBrokerConnection(const char * component_name);
rbusCoreError_t rbus_openBrokerConnection2(const char * component_name, const char * broker_address);

/* Close connection with daemon. Messaging services will cease. */
rbusCoreError_t rbus_closeBrokerConnection(void);

/* Get the underlying rtMessage connection handle */
rtConnection rbus_getConnection();

/* Register an object with the bus. An object is an addressable endpoint that other elements on the bus can send messages to. Any messages sent
 * on the bus with destination = object_name will be routed to this component and invoke the handler. */
rbusCoreError_t rbus_registerObj(const char * object_name, rbus_callback_t handler, void * user_data);

/* Unregister the given object. This makes this particular address non-reachable on the bus. */
rbusCoreError_t rbus_unregisterObj(const char * object_name);


/* Add an element to a registered object. An element is a public attribute of an object identified by a unique name. Applications using rbus can send messages to 
 * an element by marking its unique name as the recipient. rbus will deliver such messages to one of the callbacks installed for this object.
 * 
 * Elements are useful in scenarios where you want to populate the bus in such a way that a single object presents multiple public identities. For instance, if an 
 * application has an object that implements feature A and is registered as "foo", but the same object also implements features B and C known to the outside world 
 * by names "xyz" and "abc". In order to achieve this structure, the application would register "foo" as an rbus object, and follow it up by adding "xyz" and "abc"
 * as elements to this object. All messages sent to "foo", "xyz" and "abc" will be processed by callbacks installed for rbus object "foo". Inside those callbacks, 
 * you have the option to look at the intended recipient (foo vs xyz vs abc) and act accordingly. Elements don't get their own methods/callbacks. They rely on the 
 * callbacks installed for the parent object to get the job done. In other words, elements are an alias for the object.*/
rbusCoreError_t rbus_addElement(const char * object_name, const char* element);
rbusCoreError_t rbus_removeElement(const char * object_name, const char * element);

/* Register a remote procedure call for an object. Any messages on the bus sent to this object, bearing the registered method, will lead to the installed handler being invoked. */
rbusCoreError_t rbus_registerMethod(const char * object_name, const char *method, rbus_callback_t handler, void * user_data);

/* Unregister a remote procedure call for an object.*/ 
rbusCoreError_t rbus_unregisterMethod(const char * object_name, const char *method);

/* Convenience function that allows components to register remote procedure calls handlers in bulk (function tables).*/
rbusCoreError_t rbus_registerMethodTable(const char * object_name, rbus_method_table_entry_t *table, unsigned int num_entries);

/* Convenience function that allows components to unregister remote procedure calls handlers in bulk. */
rbusCoreError_t rbus_unregisterMethodTable(const char * object_name, rbus_method_table_entry_t *table, unsigned int num_entries);

/* Invoke a remote procedure call 'method' on a destination/object object_name. 'out' has the input arguments necessary for the RPC, 'in' carries the result
 * of the operation when it's complete. Marshalling of input arguments and output response is the responsibility of the caller. This function blocks until it receives a response 
 * from the remote recipient, or times out after 'timeout_milliseconds'. rbus will release 'out' internally. If call is successful, it's caller's responsibility
 * to release 'in'. */
rbusCoreError_t rbus_invokeRemoteMethod(const char * object_name, const char *method, rbusMessage out, int timeout_millisecs, rbusMessage *in);
rbusCoreError_t rbus_invokeRemoteMethod2(rtConnection conn, const char * object_name, const char *method, rbusMessage out, int timeout_millisecs, rbusMessage *in);

/* Invoke a remote procedure call 'method' on a destination/object object_name. 'out' has the input arguments necessary for the RPC. This function does not block for response
 * from the remote end. It returns immediately after the outbound message is dispatched. 'callback' is invoked when it receives the response to the RPC call, or if it times out 
 * waiting for a response. The callback will contain the response from the remote end. Marshalling of input arguments and output response is the responsibility of the caller.*/
rbusCoreError_t rbus_invokeRemoteMethodAsync(const char * object_name, const char *method, rbusMessage out, int timeout_millisecs, rbus_async_callback_t callback);
/* Notes on using event APIs:
 * An object_name is a discoverable entity on the bus that is the source of some events. An event source can issue multiple types of events, where each unique type is identified
 * by event_name. A simpler object_name can issue just one type of event, with event_name = 0 always. To receive events, clients have to subscribe to an object_name. 
 * If the object_name supports multiple event types, the clients have to subscribe for each event_name of interest individually. If the object_name doesn't support multiple 
 * event_names, the function calls publishEvent(), subcribeToEvent(), unsubscribeFromEvent(), subscribeToTimedUpdateEvent(), unsubscribeFromTimedUpdateEvent() have to 
 * use event_name = 0. */


/* Send an event message to all subscribers for this particular event. 'event_name' can be NULL if the object publishes only one event. 'out' is the payload of the event message. 
 * Caller is responsible for releasing 'out' after the function returns.*/
rbusCoreError_t rbus_publishEvent(const char* object_name,  const char * event_name, rbusMessage out);

/* Register an event with an object on the bus so that others can subscribe to it. 
 * 'callback' and 'user_data' can be NULL.  If set, 'callback' is invoked with 'user_data' when any client subscribes or unsubscribes to this event. */ 
rbusCoreError_t rbus_registerEvent(const char* object_name, const char * event, rbus_event_subscribe_callback_t callback, void * user_data);

/* Unregister an event from an object on the bus */
rbusCoreError_t rbus_unregisterEvent(const char* object_name, const char * event);

/* Add the event as an element to a registered object. */
rbusCoreError_t rbus_addElementEvent(const char * object_name, const char* event);

/* Register a callback that the framework will invoke to generate an event message that gets dispatched as a timed update event. If there are subscribers that require this event
 * to be issued at N second intervals, the installed callback will be invoked every N seconds to generate an event message. The message thus generated will be dispatched internally
 * to all subscribers.*/
rbusCoreError_t rbus_registerTimedUpdateEventCallback(const char* object_name,  const char * event_name, rbus_timed_update_event_callback_t callback);

/* Subscribe to 'event_name' events from 'object_name' object. If the object supports only one event, event_name can be NULL. If the event_name is an alias for the object, then object_name can be NULL. The installed callback will be invoked every time 
 * a matching event is received. */
rbusCoreError_t rbus_subscribeToEvent(const char * object_name,  const char * event_name, rbus_event_callback_t callback, const rbusMessage payload, void * user_data, int* providerError);

/* Subscribe to 'event_name' events from 'object_name' object, with the specified timeout. If the timeout is less than or equal to zero, timeout will be set to 1000.
 * If the object supports only one event, event_name can be NULL. If the event_name is an alias for the object, then object_name can be NULL. The installed callback will be invoked every time 
 * a matching event is received. */
rbusCoreError_t rbus_subscribeToEventTimeout(const char * object_name,  const char * event_name, rbus_event_callback_t callback, const rbusMessage payload, void * user_data, int* providerError, int timeout_ms, bool publishOnSubscribe, rbusMessage *response);

/* Unsubscribe from receiving 'event_name' events from 'object_name' object. If the object supports only one event, event_name can be NULL. */
rbusCoreError_t rbus_unsubscribeFromEvent(const char * object_name,  const char * event_name, const rbusMessage payload);

/* Register a on-subscribe callback which will be called when any subscriber subscribes to any event.
   This disables the rbuscore built-in server-side event subscription handling. Used by rbus 2.0*/
rbusCoreError_t rbus_registerSubscribeHandler(const char* object_name, rbus_event_subscribe_callback_t callback, void * user_data);

/* Register a on-event callback which will be called when any event is received. */
rbusCoreError_t rbus_registerMasterEventHandler(rbus_event_callback_t callback, void * user_data);

/* Register a callback which will be called when any client on the bus, disconnects */
rbusCoreError_t rbus_registerClientDisconnectHandler(rbus_client_disconnect_callback_t callback);
rbusCoreError_t rbus_unregisterClientDisconnectHandler();

/* Send an event message directly to a specific subscribe(e.g. listener) */
rbusCoreError_t rbus_publishSubscriberEvent(const char* object_name,  const char * event_name, const char* listener, rbusMessage out);

/*------ Convenience functions built on top of base functions above. ------*/


/* Set remote object 'object_name' to a value encoded in 'message'. This function will block (up to 'timeout_milliseconds') until remote end responds with a success or failure. 
 * Returns RBUSCORE_SUCCESS if operation is a success. rbus will release 'message' internally. */
rbusCoreError_t rbus_pushObj(const char * object_name, rbusMessage message, int timeout_millisecs);

/* Set remote object 'object_name' to a value encoded in 'message'. This function does not block for remote response and will return as soon as the outbound message is sent. 
 * 'callback', if not NULL, will be invoked when the remote end responds (or when the timeout is up).*/
rbusCoreError_t rbus_pushObjNoAck(const char * object_name, rbusMessage message);

/* Get the value of 'object_name'. This call blocks (up to 'timeout_millisecs') until it receives a response from the remote recipient. Returns RTMESSAGE_BUS_SUCESS if operation 
 * is a success.*/
rbusCoreError_t rbus_pullObj(const char * object_name, int timeout_millisecs, rbusMessage *response);

/* Subscribe to timed updates of nature specified by 'event_name'. You will receive 'event_name' events every 'interval_milliseconds'. The server that implements 'event_name' will 
 * generate and send recurring event messages to us at the interval specified here. */
rbusCoreError_t rbus_subscribeToTimedUpdateEvents(const char * object_name,  const char * event_name, unsigned int interval_milliseconds, rbus_event_callback_t callback, void * user_data);

/* Unsubscribe from timed updates.*/
rbusCoreError_t rbus_unsubscribeFromTimedUpdateEvents(const char * object_name,  const char * event_name);

/* Discover what end-points a wildcard expression resolves to.*/
rbusCoreError_t rbus_discoverWildcardDestinations(const char * expression, int * count, char *** destinations);

rbusCoreError_t rbus_discoverObjectElements(const char * object, int * count, char *** elements);

/* Discovers the given element to their corresponding objects. The look-up results are returned in an array 'objects' of size that is represented in count;
 Usually the length is 1 build if the given element is WildCard, it is possible to have multiple objects in return.
 It's caller's responsibility to free the individual strings in 'objects' array as well as 'objects' itself. If look-up fails for any of the elements, it will be 
 represented by an empty string in the objects array. If this function returns error, do not try to dereference/free 'objects'.*/
rbusCoreError_t rbus_discoverElementObjects(const char* element, int * count, char *** objects);

/*same as above but for multiple elements*/
rbusCoreError_t rbus_discoverElementsObjects(int numElements, const char** elements, int * count, char *** objects);

/* Finds out all the registered application that is running with RBUS at any given point in time */
rbusCoreError_t rbus_discoverRegisteredComponents(int * count, char *** components);

/* Get the rbus status; to find out whether the rbus is enabled or not. The application can take action (ex: registration of events) based on this return value. */
rbuscore_bus_status_t rbuscore_checkBusStatus(void);

/* Sends reply response to the GET/SET and other methods */
rbusCoreError_t rbus_sendResponse(const rtMessageHeader* hdr, rbusMessage response);



/* The Consumer application to identify whether any private connection exist for the given DML */
rtConnection rbuscore_FindClientPrivateConnection(const char *pParameterName);

/* The Consumer application to request to create a private connection to the Provider for the given DML */
rbusCoreError_t rbuscore_createPrivateConnection(const char *pParameterName, rtConnection *pPrivateConn);

/* The Consumer application to request to close the private connection to the Provider for the given DML */
rbusCoreError_t rbuscore_closePrivateConnection(const char *pParameterName);

/* The Consumer application to request to terminate the private connection to the Provider for ALL the DMLs of the provider */
rbusCoreError_t rbuscore_terminatePrivateConnection(const char *pProviderName);

/* The Provider application to request to start the instance of rtrouted in the given socket name; for the given DML */
rbusCoreError_t rbuscore_startPrivateListener(const char* pPrivateConnAddress, const char* pConsumerName, const char *pDMLName, rbus_callback_t handler, void * user_data);

/* The Provider application to request to remove/close the instance of rtrouted in the given DML */
rbusCoreError_t rbuscore_updatePrivateListener(const char* pConsumerName, const char *pDMLName);


#ifdef __cplusplus
}
#endif
#endif 
