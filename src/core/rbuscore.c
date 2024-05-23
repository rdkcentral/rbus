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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rbuscore.h"
#include "rbuscore_logger.h"
#include "rtVector.h"
#include "rtAdvisory.h"
#include "rtMemory.h"

#include "rtrouteBase.h"
void rbusMessage_BeginMetaSectionWrite(rbusMessage message);
void rbusMessage_EndMetaSectionWrite(rbusMessage message);
void rbusMessage_BeginMetaSectionRead(rbusMessage message);
void rbusMessage_EndMetaSectionRead(rbusMessage message);

/* Begin constant definitions.*/
static const unsigned int TIMEOUT_VALUE_FIRE_AND_FORGET = 1000;
static const unsigned int MAX_SUBSCRIBER_NAME_LENGTH = MAX_OBJECT_NAME_LENGTH;
static const char * DEFAULT_EVENT = "";
/* End constant definitions.*/

/* Begin type definitions.*/
typedef struct _rbusOpenTelemetryContext
{
  char otTraceParent[RBUS_OPEN_TELEMETRY_DATA_MAX];
  char otTraceState[RBUS_OPEN_TELEMETRY_DATA_MAX];
} rbusOpenTelemetryContext;

static pthread_once_t _open_telemetry_once = PTHREAD_ONCE_INIT;
static pthread_key_t  _open_telemetry_key;
static rbusOpenTelemetryContext* rbus_getOpenTelemetryContextFromThreadLocal();

static void rbus_init_open_telemeetry_thread_specific_key()
{
  pthread_key_create(&_open_telemetry_key, free);
}

static void rbus_releaseOpenTelemetryContext();
/* Begin rbus_server */

struct _server_object;
typedef struct _server_object* server_object_t;

typedef struct _server_method
{
    char name[MAX_METHOD_NAME_LENGTH+1];
    rbus_callback_t callback;
    void * data;
} *server_method_t;


typedef struct _server_event
{
    char name[MAX_EVENT_NAME_LENGTH+1];
    server_object_t object;
    rtVector listeners /*list of strings*/;
    rbus_event_subscribe_callback_t sub_callback;
    void * sub_data;
} *server_event_t;

typedef struct _server_object
{
    char name[MAX_OBJECT_NAME_LENGTH+1];
    void* data;
    rbus_callback_t callback;
    bool process_event_subscriptions;
    rtVector methods; /*list of server_method_t*/
    rtVector subscriptions; /*list of server_event_t*/
    rbus_event_subscribe_callback_t subscribe_handler_override;
    void* subscribe_handler_data;
} *server_object_t;

/////////////
extern char* __progname;
static void _freePrivateServer(void* p);
static void _rbuscore_directconnection_load_from_cache();
static void _rbuscore_destroy_clientPrivate_connections();
const rtPrivateClientInfo* _rbuscore_find_server_privateconnection(const char *pParameterName, const char *pConsumerName);
/////

void server_method_create(server_method_t* meth, char const* name, rbus_callback_t callback, void* data)
{
    (*meth) = rt_malloc(sizeof(struct _server_method));
    strcpy((*meth)->name, name);
    (*meth)->callback = callback;
    (*meth)->data = data;
}

int server_method_compare(const void* left, const void* right)
{
    return strncmp(((server_event_t)left)->name, (char*)right, MAX_METHOD_NAME_LENGTH);
}

int server_event_compare(const void* left, const void* right)
{
    return strncmp(((server_event_t)left)->name, (char*)right, MAX_EVENT_NAME_LENGTH);
}

void server_event_create(server_event_t* event, const char * event_name, server_object_t obj, rbus_event_subscribe_callback_t sub_callback, void* sub_data)
{
    (*event) = rt_malloc(sizeof(struct _server_event));
    rtVector_Create(&(*event)->listeners);
    strcpy((*event)->name, event_name);
    (*event)->object = obj;
    (*event)->sub_callback = sub_callback;
    (*event)->sub_data = sub_data;
}

void server_event_destroy(void* p)
{
    server_event_t event = p;
    rtVector_Destroy(event->listeners, rtVector_Cleanup_Free);
    free(event);
}

void server_event_addListener(server_event_t event, char const* listener)
{
    if(!listener)
    {
        RBUSCORELOG_ERROR("Listener is empty.");
    }
    else if(!rtVector_HasItem(event->listeners, listener, rtVector_Compare_String))
    {
        rtVector_PushBack(event->listeners, strdup(listener));

        if(event->sub_callback)
        {
            event->sub_callback(event->object->name, event->name, listener, 1, NULL, event->sub_data);
        }

        RBUSCORELOG_INFO("Listener %s added for event %s.", listener, event->name);
    }
    else
    {
        RBUSCORELOG_WARN("Listener %s is already registered for event %s.", listener, event->name);
    }
}

void server_event_removeListener(server_event_t event, char const* listener)
{
    if(!listener)
    {
        RBUSCORELOG_ERROR("Listener is empty.");
    }
    else if(rtVector_HasItem(event->listeners, listener, rtVector_Compare_String))
    {
        RBUSCORELOG_WARN("Removing listener %s for event %s.", listener, event->name);

        rtVector_RemoveItemByCompare(event->listeners, listener, rtVector_Compare_String, rtVector_Cleanup_Free);

        if(event->sub_callback)
        {
            event->sub_callback(event->object->name, event->name, listener, 0, NULL, event->sub_data);
        }
    }
    else
    {
        RBUSCORELOG_ERROR("Listener %s not found for event %s.", listener, event->name);
    }
}

int server_object_compare(const void* left, const void* right)
{
    return strncmp(((server_object_t)left)->name, (char*)right, MAX_OBJECT_NAME_LENGTH);
}

void server_object_create(server_object_t* obj, char const* name, rbus_callback_t callback, void* data)
{
    (*obj) = rt_malloc(sizeof(struct _server_object));
    strcpy((*obj)->name, name);
    (*obj)->callback = callback;
    (*obj)->data = data;
    (*obj)->process_event_subscriptions = false;
    (*obj)->subscribe_handler_override = NULL;
    (*obj)->subscribe_handler_data = NULL;
    rtVector_Create(&(*obj)->methods);
    rtVector_Create(&(*obj)->subscriptions);
}

void server_object_destroy(void* p)
{
    server_object_t obj = p;
    rtVector_Destroy(obj->methods, rtVector_Cleanup_Free);
    rtVector_Destroy(obj->subscriptions, server_event_destroy);
    free(obj);
}

rbusCoreError_t server_object_subscription_handler(server_object_t obj, const char * event, char const* subscriber, int added, rbusMessage payload)
{
    rbusCoreError_t ret;

    if((NULL == event) || (NULL == subscriber) ||
       (MAX_SUBSCRIBER_NAME_LENGTH <= strlen(subscriber)) || 
       (MAX_EVENT_NAME_LENGTH <= strlen(event)))
    {
        RBUSCORELOG_ERROR("Cannot %s subscriber %s to event %s. Length exceeds limits.", added ? "add":"remove", subscriber, event);
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    
    if(obj->subscribe_handler_override)
    {
        ret = (rbusCoreError_t)obj->subscribe_handler_override(obj->name, event, subscriber, added, payload, obj->subscribe_handler_data);
        if(ret != RBUSCORE_ERROR_SUBSCRIBE_NOT_HANDLED)
            return ret;
    }

    server_event_t server_event = rtVector_Find(obj->subscriptions, event, server_event_compare);

    if(server_event)
    {
        if(added)
        {
            server_event_addListener(server_event, subscriber);
        }
        else
        {
            server_event_removeListener(server_event, subscriber);
        }
        return RBUSCORE_SUCCESS;
    }
    else
    {
        RBUSCORELOG_ERROR("Object %s doesn't support event %s. Cannot %s listener.", obj->name, event, added ? "add":"remove");
        return RBUSCORE_ERROR_UNSUPPORTED_EVENT;
    }
}

typedef struct _queued_request
{
    rtMessageHeader hdr;
    rbusMessage msg;
    server_object_t obj;
} *queued_request_t;

void queued_request_create(queued_request_t* req, rtMessageHeader hdr, rbusMessage msg, server_object_t obj)
{
    (*req) = rt_malloc(sizeof(struct _queued_request));
    (*req)->hdr = hdr;
    (*req)->msg = msg;
    (*req)->obj = obj;
}

/* End rbus_server */

/* Begin rbus_client */
typedef struct _client_event
{
    char name[MAX_EVENT_NAME_LENGTH+1];
    rbus_event_callback_t callback;
    void* data;
} *client_event_t;

typedef struct _client_subscription
{
    char object[MAX_OBJECT_NAME_LENGTH+1];
    rtVector events; /*list of client_event_t*/
} *client_subscription_t;

void client_event_create(client_event_t* event, const char* name, rbus_event_callback_t callback, void* data)
{
    (*event) = rt_malloc(sizeof(struct _client_event));
    (*event)->callback = callback;
    (*event)->data = data;
    strcpy((*event)->name, name);
}

int client_event_compare(const void* left, const void* right)
{
    return strncmp(((const client_event_t)left)->name, (char const*)right, MAX_EVENT_NAME_LENGTH);
}

int client_subscription_compare(const void* left, const void* right)
{
    return strncmp(((const client_subscription_t)left)->object, (char const*)right, MAX_OBJECT_NAME_LENGTH);
}

void client_subscription_create(client_subscription_t* sub, const char * object_name)
{
    (*sub) = rt_malloc(sizeof(struct _client_subscription));
    strcpy((*sub)->object, object_name);
    rtVector_Create(&(*sub)->events); 
}

void client_subscription_destroy(void* p)
{
    client_subscription_t sub = p;
    rtVector_Destroy(sub->events, rtVector_Cleanup_Free);
    free(sub);
}

/* End rbus_client */

/* End type definitions.*/

/* Begin global variables*/
#define MAX_DAEMON_ADDRESS_LEN 256
static char g_daemon_address[MAX_DAEMON_ADDRESS_LEN] = "unix:///tmp/rtrouted";
static rtConnection g_connection = NULL;
static rtVector g_server_objects; /*server_object_t list*/
static pthread_mutex_t g_mutex;
static pthread_mutex_t g_directCliMutex;
static pthread_mutex_t g_directServMutex;
static int g_mutex_init = 0;
static bool g_run_event_client_dispatch = false;
static rtVector g_event_subscriptions_for_client; /*client_subscription_t list. Used by the subscriber to track all active subscriptions. */
static rtVector g_queued_requests; /*list of queued_request */

/*client disconnect detection*/
static bool g_advisory_listener_installed = false;
static rbus_client_disconnect_callback_t g_client_disconnect_callback = NULL;

static rtVector gListOfServerDirectDMLs;
static rtVector gListOfClientDirectDMLs;

rbus_event_callback_t g_master_event_callback = NULL;
void* g_master_event_user_data = NULL;

/* End global variables*/
static int directServerLock()
{
	return pthread_mutex_lock(&g_directServMutex);
}

static int directServerUnlock()
{
	return pthread_mutex_unlock(&g_directServMutex);
}

static int directClientLock()
{
	return pthread_mutex_lock(&g_directCliMutex);
}

static int directClientUnlock()
{
	return pthread_mutex_unlock(&g_directCliMutex);
}
static int lock()
{
	return pthread_mutex_lock(&g_mutex);
}

static int unlock()
{
	return pthread_mutex_unlock(&g_mutex);
}

static rbusCoreError_t send_subscription_request(const char * object_name, const char * event_name, bool activate, const rbusMessage payload, int* providerError, int timeout, bool publishOnSubscribe, rbusMessage *response, bool rawData);

static void perform_init()
{
    RBUSCORELOG_DEBUG("Performing init");
    rtVector_Create(&g_server_objects);
    rtVector_Create(&g_event_subscriptions_for_client);
    rtVector_Create(&gListOfServerDirectDMLs);
    rtVector_Create(&gListOfClientDirectDMLs);
    _rbuscore_directconnection_load_from_cache();
}

static void perform_cleanup()
{
    size_t i, sz;

    RBUSCORELOG_DEBUG("Performing cleanup");

    lock();

    rtVector_Destroy(g_server_objects, server_object_destroy);

    rtVector_Destroy(gListOfServerDirectDMLs, _freePrivateServer);
    _rbuscore_destroy_clientPrivate_connections();
    rtVector_Destroy(gListOfClientDirectDMLs, rtVector_Cleanup_Free);

    sz = rtVector_Size(g_event_subscriptions_for_client);
    if(sz>0)
    {
        RBUSCORELOG_INFO("Cancelling active event subscriptions.");
        unlock();
        for(i = 0; i < sz; ++i)
        {
            size_t i2, sz2;
            client_subscription_t sub = rtVector_At(g_event_subscriptions_for_client, i);

            sz2 = rtVector_Size(sub->events);
            for(i2 = 0; i2 < sz2; i2++)
            {   
                client_event_t event = rtVector_At(sub->events, i2);
                send_subscription_request(sub->object, event->name, false, NULL, NULL, 0, false, NULL, false);
            }
        }
        lock();
    }
    rtVector_Destroy(g_event_subscriptions_for_client, client_subscription_destroy);

    unlock();
}


void _rbusMessage_SetMetaInfo(rbusMessage m,
  char const *method_name,
  char const *ot_parent,
  char const *ot_state)
{
  rbusMessage_BeginMetaSectionWrite(m);
  rbusMessage_SetString(m, method_name);
  rbusMessage_SetString(m, ot_parent);
  rbusMessage_SetString(m, ot_state);
  rbusMessage_EndMetaSectionWrite(m);

  #if 0
  printf("set metainfo\n");
  printf("\tmethod_name:%s\n", method_name);
  printf("\tot_parent  :%s\n", ot_parent);
  printf("\tot_state   :%s\n", ot_state);
  #endif
}

void _rbusMessage_GetMetaInfo(rbusMessage m,
  char const **method_name,
  char const **ot_parent,
  char const **ot_state)
{
  rbusMessage_BeginMetaSectionRead(m);
  rbusMessage_GetString(m, method_name);
  rbusMessage_GetString(m, ot_parent);
  rbusMessage_GetString(m, ot_state);
  rbusMessage_EndMetaSectionRead(m);

  #if 0
  printf("get metainfo\n");
  printf("\tmethod_name:%s\n", *method_name);
  printf("\tot_parent  :%s\n", *ot_parent);
  printf("\tot_state   :%s\n", *ot_state);
  #endif
}

static server_object_t get_object(const char * object_name)
{
    return rtVector_Find(g_server_objects, object_name, server_object_compare);
}

static rbusCoreError_t translate_rt_error(rtError err)
{
    if(RT_OK == err)
        return RBUSCORE_SUCCESS;
    else
        return RBUSCORE_ERROR_GENERAL;
}

static void dispatch_method_call(rbusMessage msg, const rtMessageHeader *hdr, server_object_t obj)
{
    rtError err = RT_OK;
    const char* method_name = NULL;
    const char* traceParent = NULL;
    const char* traceState = NULL;
    rbusMessage response = NULL;
    bool handler_invoked = false;

    /* Fetch the context that was sent with the message */
    _rbusMessage_GetMetaInfo(msg, &method_name, &traceParent, &traceState);
    rbus_setOpenTelemetryContext(traceParent, traceState);

    lock();
    if( rtVector_Size(obj->methods) > 0 && RT_OK == err)
    {
        server_method_t method = rtVector_Find(obj->methods, method_name, server_method_compare);

        if(method)
        {
            unlock();
            method->callback(hdr->topic, method_name, msg, method->data, &response, hdr); //FIXME: potential for race.
            handler_invoked = true;
        }
    }
    if(false == handler_invoked)
    {
        unlock();
        if(obj->callback(hdr->topic, method_name, msg, obj->data, &response, hdr) == RBUSCORE_SUCCESS_ASYNC) //FIXME: potential for race
            return;/*provider will send response async later on*/
    }
    /* Clear the context for next method */
    rbus_clearOpenTelemetryContext();

    rbus_sendResponse(hdr, response);
}

static void onMessage(rtMessageHeader const* hdr, uint8_t const* data, uint32_t dataLen, void* closure)
{
    rbusMessage msg;
    rbusMessage_FromBytes(&msg, data, dataLen);

    /*using namespace rbus_server;*/
    static int stack_counter = 0;
    stack_counter++;
    server_object_t obj = (server_object_t)closure;

    if(1 != stack_counter)
    {
        //We're in the midst of handling another request. Queue this one for later.
        queued_request_t req;
        queued_request_create(&req, *hdr, msg, obj);
        rtVector_PushBack(g_queued_requests, req);
    }
    else
        dispatch_method_call(msg, hdr, obj);

    if((1 == stack_counter) && rtVector_Size(g_queued_requests) > 0)
    {
        //Consume the request queue now that the earlier request has been fully handled.
        while(rtVector_Size(g_queued_requests) > 0)
        {
            queued_request_t req = rtVector_At(g_queued_requests, 0);
            dispatch_method_call(req->msg, &req->hdr, req->obj);
            rtVector_RemoveItem(g_queued_requests, req, rtVector_Cleanup_Free);
        }
    }
    stack_counter--;

    rbusMessage_Release(msg);
    return;
}

static void configure_router_address()
{
    FILE* fconfig = fopen("/etc/rbus_client.conf", "r");
    if(fconfig)
    {
        size_t len;
        char buff[MAX_DAEMON_ADDRESS_LEN] = {0};

        /*locate the first word(block of printable text)*/
        while(fgets(buff, MAX_DAEMON_ADDRESS_LEN, fconfig))
        {
            len = strlen(buff);
            if(len > 0)
            {
                size_t idx1 = 0;

                /*move past any leading space*/
                while(idx1 < len && isspace(buff[idx1]))
                    idx1++;

                if(idx1 < len)
                {
                    size_t idx2 = idx1+1;

                    /*move to end of word*/
                    while(idx2 < len && !isspace(buff[idx2]))
                        idx2++;

                    if(idx2-idx1 > 0)
                    {
                        buff[idx2] = 0;
                        strcpy(g_daemon_address, &buff[idx1]);
                        break;
                    }
                }
            }
        }
        fclose(fconfig);
    }
}

rbusCoreError_t rbus_openBrokerConnection(const char * component_name)
{
    if(RBUSCORE_DISABLED == rbuscore_checkBusStatus())
    {
        RBUSCORELOG_ERROR("RBUS is disabled");
        return RBUSCORE_ERROR_GENERAL;
    }
    else
    {
        return rbus_openBrokerConnection2(component_name, NULL);
    }
}

rbusCoreError_t rbus_openBrokerConnection2(const char * component_name, const char* broker_address)
{
	rbusCoreError_t ret = RBUSCORE_SUCCESS;
	rtError result = RT_OK;

	if(!component_name)
	{
		RBUSCORELOG_ERROR("Invalid parameter: component name null");
		return RBUSCORE_ERROR_INVALID_PARAM;
	}

	/*TODO we really need a 1 call per process init function to initialize the global variables
	and it might not be safe to switch from PTHREAD_MUTEX_ERRORCHECK and use static initializer PTHREAD_MUTEX_INITIALIZER*/
	if(!g_mutex_init)
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(&g_mutex, &attr);
		pthread_mutex_init(&g_directServMutex, &attr);
		pthread_mutex_init(&g_directCliMutex, &attr);
		g_mutex_init = 1;
	}

	lock();

	/*only 1 connection per process*/
	if(g_connection)
	{
		RBUSCORELOG_DEBUG("using previously opened connection for %s", component_name);
		unlock();
		return RBUSCORE_SUCCESS;
	}

	/*nobody calls rbus_openBrokerConnection2 directly (except maybe some unit tests) so broker_address is always NULL*/
	if(broker_address == NULL)
	{
		configure_router_address();/*this allows devices with split cpu environment to connect to rtrouted over tcp*/
		broker_address = g_daemon_address;
	}

	RBUSCORELOG_INFO("Broker address: %s", broker_address);

	perform_init();
	result = rtConnection_Create(&g_connection, "rbus", broker_address);
	if(RT_OK != result)
	{
		RBUSCORELOG_ERROR("Failed to create a connection for %s: Error: %d", component_name, result);
		g_connection = NULL;
		perform_cleanup();
		unlock();
		return RBUSCORE_ERROR_GENERAL;
	}

	RBUSCORELOG_DEBUG("Successfully created connection for %s", component_name );
	unlock();
	return ret;
}

rbusCoreError_t rbus_closeBrokerConnection()
{
    rtError err = RT_OK;
    lock();
    if(NULL == g_connection)
    {
        RBUSCORELOG_INFO("No connection exist to close.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }
    if(1 < rtVector_Size(g_server_objects))
    {
        RBUSCORELOG_INFO("Should not destroy/close connection when one or more active connection exits.");
        unlock();
        return RBUSCORE_SUCCESS;
    }
    rbus_releaseOpenTelemetryContext();
    perform_cleanup();
    err = rtConnection_Destroy(g_connection);
    if(RT_OK != err)
    {
        RBUSCORELOG_ERROR("Could not destroy connection. Error: 0x%x.", err);
        return RBUSCORE_ERROR_GENERAL;
    }
    g_connection = NULL;
    unlock();

    pthread_mutex_destroy(&g_mutex);
    pthread_mutex_destroy(&g_directServMutex);
    pthread_mutex_destroy(&g_directCliMutex);
    g_mutex_init = 0;

    RBUSCORELOG_INFO("Destroyed connection.");
    return RBUSCORE_SUCCESS;
}

rtConnection rbus_getConnection()
{
    return g_connection;
}

static rbusCoreError_t send_subscription_request(const char * object_name, const char * event_name, bool activate, const rbusMessage payload, int* providerError, int timeout_ms, bool publishOnSubscribe, rbusMessage *response, bool rawData)
{
    /* Method definition to add new event subscription: 
     * method name: METHOD_ADD_EVENT_SUBSCRIPTION / METHOD_REMOVE_EVENT_SUBSCRIPTION.
     * argument 1: event_name, mapped to key MESSAGE_FIELD_PAYLOAD 
     * Expected resut:
     * integer, mapped to key MESSAGE_FIELD_RESULT. 0 is success. Anything else is a failure. */
    rbusCoreError_t ret;

    rbusMessage request, internal_response;
    rbusMessage_Init(&request);

    rbusMessage_SetString(request, event_name);
    rbusMessage_SetString(request, rtConnection_GetReturnAddress(g_connection));
    rbusMessage_SetInt32(request, payload ? 1 : 0);
    if(payload)
        rbusMessage_SetMessage(request, payload);
    if(publishOnSubscribe)
        rbusMessage_SetInt32(request, 1); /*for publishOnSubscribe */
    else
        rbusMessage_SetInt32(request, 0);
    if(rawData)
        rbusMessage_SetInt32(request, 1); /*for rawDataSubscription */
    else
        rbusMessage_SetInt32(request, 0);

    if(timeout_ms <= 0)
        timeout_ms = TIMEOUT_VALUE_FIRE_AND_FORGET;
    ret = rbus_invokeRemoteMethod(object_name, (activate? METHOD_SUBSCRIBE : METHOD_UNSUBSCRIBE),
            request, timeout_ms, &internal_response);
    if(RBUSCORE_SUCCESS == ret)
    {
        rtError extract_ret;
        int result;

        extract_ret = rbusMessage_GetInt32(internal_response, &result);
        if(RT_OK == extract_ret)
        {
            if(RBUSCORE_SUCCESS == result)
            {
                /*Event registration was successful.*/
                RBUSCORELOG_INFO("Subscription for %s::%s is now %s.", object_name, event_name, (activate? "active" : "cancelled"));
                ret = RBUSCORE_SUCCESS;
            }
            else
            {
                /*For some reason, event publisher couldnt' handle the request.*/
                //TODO: Expand to troubleshoot causes of a failed subscription.
                RBUSCORELOG_ERROR("Error %s subscription for %s::%s. Server returned error %d.", (activate? "adding" : "removing"), object_name, event_name, result);
                ret = RBUSCORE_ERROR_GENERAL;
                if(providerError)
                    *providerError = result;
            }
        }
        else
        {
            RBUSCORELOG_ERROR("Error adding subscription for %s::%s. Received unexpected response.", object_name, event_name);
            ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
        }
        if(response != NULL)
            *response = internal_response;
        else
            rbusMessage_Release(internal_response);
    }
    else if(RBUSCORE_ERROR_ENTRY_NOT_FOUND == ret)
    {
        RBUSCORELOG_DEBUG("Error %s subscription for %s::%s. Provider not found. %d", (activate? "adding" : "removing"), object_name, event_name, ret);
        //keep ret as RBUSCORE_ERROR_DESTINATION_UNREACHABLE
    }
    else
    {
        RBUSCORELOG_ERROR("Error %s subscription for %s::%s. Communication issues. %d", (activate? "adding" : "removing"), object_name, event_name, ret);
        ret = RBUSCORE_ERROR_REMOTE_END_FAILED_TO_RESPOND;
    }

    return ret;
}

rbusCoreError_t rbus_registerObj(const char * object_name, rbus_callback_t handler, void * user_data)
{
    rtError err = RT_OK;
    server_object_t obj = NULL;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected. Cannot register objects yet.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if(NULL == object_name)
    {
        RBUSCORELOG_ERROR("Object name is NULL");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    int object_name_len = strlen(object_name);
    if((MAX_OBJECT_NAME_LENGTH <= object_name_len) || (0 == object_name_len))
    {
        RBUSCORELOG_ERROR("object_name name is too long/short.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    lock();
    obj = rtVector_Find(g_server_objects, object_name, server_object_compare);
    unlock();
    if(obj)
    {
        RBUSCORELOG_ERROR("%s is already registered. Rejecting duplicate registration.", object_name);
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    server_object_create(&obj, object_name, handler, user_data);

    //TODO: callback signature translation. rbusMessage uses a significantly wider signature for callbacks. Translate to something simpler.
    err = rtConnection_AddListener(g_connection, object_name, onMessage, obj); /* Avoiding rtConnection_AddListenerWithId call here as ccsp code
                                                                                    uses this rbus_registerObj() function call directly and usage
                                                                                    of rtConnection_AddListenerWithId() function will result in
                                                                                    conflict with subscriptionId */

    if(RT_OK == err)
    {
        size_t sz;

        lock();
        rtVector_PushBack(g_server_objects, obj);
        sz = rtVector_Size(g_server_objects);
        unlock();
        RBUSCORELOG_DEBUG("Registered object %s", object_name);
        if(sz >= MAX_REGISTERED_OBJECTS)
        {
            RBUSCORELOG_WARN("Number of registered objects is %zu", sz);
        }
        return RBUSCORE_SUCCESS;
    }
    else
    {
        RBUSCORELOG_ERROR("Failed to register object. Error: 0x%x", err);
        server_object_destroy(obj);
        return RBUSCORE_ERROR_GENERAL;
    }
}

rbusCoreError_t rbus_registerMethod(const char * object_name, const char *method_name, rbus_callback_t handler, void * user_data)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    if(MAX_METHOD_NAME_LENGTH <= strlen(method_name))
        return RBUSCORE_ERROR_INVALID_PARAM;

    lock();
    //TODO: Check that method name length is within limits and search for duplicates
    server_object_t obj = get_object(object_name);
    if(obj)
    {
        if(MAX_SUPPORTED_METHODS <= rtVector_Size(obj->methods))
        {
            RBUSCORELOG_ERROR("Too many methods registered with object %s. Cannot register more.", object_name);
            ret = RBUSCORE_ERROR_OUT_OF_RESOURCES;
        }
        else
        {
            server_method_t method = rtVector_Find(obj->methods, method_name, server_method_compare);

            if(method)
            {
               unlock();
               RBUSCORELOG_ERROR("Method %s is already registered,Rejecting duplicate registration.", method_name);
               return RBUSCORE_ERROR_INVALID_PARAM;
            }
            else
            {
                server_method_create(&method, method_name, handler, user_data);
                rtVector_PushBack(obj->methods, method);
                RBUSCORELOG_DEBUG("Successfully registered method %s with object %s", method_name, object_name);
            }
        }
    }
    else
    {
        RBUSCORELOG_ERROR("Couldn't locate object %s.", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();
    return ret;
}


rbusCoreError_t rbus_unregisterMethod(const char * object_name, const char *method_name)
{
    /*using namespace rbus_server;*/
	rbusCoreError_t ret = RBUSCORE_SUCCESS;
	lock();

    server_object_t obj = get_object(object_name);
    if(obj)
    {
        server_method_t method = rtVector_Find(obj->methods, method_name, server_method_compare);
        if(method)
        {
            rtVector_RemoveItem(obj->methods, method, rtVector_Cleanup_Free);
            RBUSCORELOG_INFO("Successfully unregistered method %s from object %s", method_name, object_name);
        }
        else
        {
            RBUSCORELOG_ERROR("Couldn't find a method %s registered with object %s.", method_name, object_name);
            ret = RBUSCORE_ERROR_GENERAL;
        }
    }
    else	
    {
        RBUSCORELOG_ERROR("Couldn't locate object %s.", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();
    return ret;
}

rbusCoreError_t rbus_addElementEvent(const char * object_name, const char* event)
{
    return rbus_addElement(object_name, event);
}

rbusCoreError_t rbus_registerMethodTable(const char * object_name, rbus_method_table_entry_t *table, unsigned int num_entries)
{
    rbusCoreError_t ret= RBUSCORE_SUCCESS;
    RBUSCORELOG_INFO("Registering method table for object %s", object_name);
    for(unsigned int i = 0; i < num_entries; i++)
    {
        if((ret = rbus_registerMethod(object_name, table[i].method, table[i].callback, table[i].user_data)) != RBUSCORE_SUCCESS)
        {
            RBUSCORELOG_ERROR("Failed to register table with object %s. Method: %s. Aborting remaining method registrations.", object_name, table[i].method);
            break;
        }
    }
    return ret;
}

rbusCoreError_t rbus_unregisterMethodTable(const char * object_name, rbus_method_table_entry_t *table, unsigned int num_entries)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    RBUSCORELOG_INFO("Unregistering method table for object %s", object_name);
    for(unsigned int i = 0; i < num_entries; i++)
    {
        if((ret = rbus_unregisterMethod(object_name, table[i].method)) != RBUSCORE_SUCCESS)
        {
            RBUSCORELOG_ERROR("Failed to unregister table with object %s. Method: %s. Aborting remaining method unregistrations.", object_name, table[i].method);
            break;
        }
    }
    return ret;
}

rbusCoreError_t rbus_unregisterObj(const char * object_name)
{
    /*using namespace rbus_server;*/
    rtError err = RT_OK;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    if((NULL == object_name) || ('\0' == object_name[0]) || (MAX_OBJECT_NAME_LENGTH <= strlen(object_name)))
    {
        RBUSCORELOG_ERROR("object_name is invalid.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    err = rtConnection_RemoveListener(g_connection, object_name);
    if(RT_OK != err)
    {
        RBUSCORELOG_ERROR("rtConnection_RemoveListener %s failed: Err=%d", object_name, err);
        return RBUSCORE_ERROR_GENERAL;
    }

    lock();
    server_object_t obj = get_object(object_name);
    if(NULL != obj)
    {
        rtVector_RemoveItem(g_server_objects, obj, server_object_destroy);
        RBUSCORELOG_INFO("Unregistered object %s.", object_name);
    }
    else
    {
        RBUSCORELOG_ERROR("No matching entry for object %s.", object_name);
        ret = RBUSCORE_ERROR_GENERAL;
    }
    unlock();

    return ret;
}

rbusCoreError_t rbus_addElement(const char * object_name, const char * element)
{
    rtError err = RT_OK;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if((NULL == object_name) || (NULL == element))
    {
        RBUSCORELOG_ERROR("Object/element name is NULL");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    int object_name_len = strlen(object_name);
    int element_name_len = strlen(element);
    if((MAX_OBJECT_NAME_LENGTH <= object_name_len) || (0 == object_name_len) ||
            (MAX_OBJECT_NAME_LENGTH <= element_name_len) || (0 == element_name_len))
    {
        RBUSCORELOG_ERROR("object/element name is too long/short.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    err = rtConnection_AddAlias(g_connection, object_name, element);
    if(RT_OK != err)
    {
        RBUSCORELOG_ERROR("Failed to add element. Error: 0x%x", err);
        if (RT_ERROR_DUPLICATE_ENTRY == err)
            return RBUSCORE_ERROR_DUPLICATE_ENTRY;
        else if (RT_ERROR_PROTOCOL_ERROR == err)
            return RBUSCORE_ERROR_UNSUPPORTED_ENTRY;
        else
            return RBUSCORE_ERROR_GENERAL;
    }

    RBUSCORELOG_DEBUG("Added alias %s for object %s.", element, object_name);
    return RBUSCORE_SUCCESS;
}

rbusCoreError_t rbus_removeElement(const char * object, const char * element)
{
    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if((NULL == object) || (NULL == element))
    {
        RBUSCORELOG_ERROR("Object/element name is NULL");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    int object_name_len = strlen(object);
    int element_name_len = strlen(element);
    if((MAX_OBJECT_NAME_LENGTH <= object_name_len) || (0 == object_name_len) ||
            (MAX_OBJECT_NAME_LENGTH <= element_name_len) || (0 == element_name_len))
    {
        RBUSCORELOG_ERROR("object/element name is too long/short.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    rtError err = rtConnection_RemoveAlias(g_connection, object, element);
    if(RT_OK != err)
        return RBUSCORE_ERROR_GENERAL;
    return RBUSCORE_SUCCESS;
}

rbusCoreError_t rbus_pushObj(const char * object_name, rbusMessage message, int timeout_millisecs)
{
    rtError err = RT_OK;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rbusMessage response = NULL;
    if((ret = rbus_invokeRemoteMethod(object_name, METHOD_SETPARAMETERVALUES, message, timeout_millisecs, &response)) != RBUSCORE_SUCCESS)
    {
        RBUSCORELOG_ERROR("Failed to send message. Error code: 0x%x", err);
        return ret;
    }
    else
    {
        int result = RBUSCORE_SUCCESS;
        if((err = rbusMessage_GetInt32(response, &result) == RT_OK))
        {
            ret = (rbusCoreError_t)result;
        }
        else
        {
            RBUSCORELOG_ERROR("%s.", stringify(RBUSCORE_ERROR_MALFORMED_RESPONSE));
            ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
        }
        rbusMessage_Release(response);
    }
    return ret;
}

static rtError rbus_sendRequest(rtConnection con, rbusMessage req, char const* topic, rbusMessage* res, int32_t timeout)
{
    rtError err = RT_OK;
    uint8_t* data = NULL;
    uint32_t dataLength = 0;
    uint8_t* rspData = NULL;
    uint32_t rspDataLength = 0;

    rbusMessage_ToBytes(req, &data, &dataLength);

    err = rtConnection_SendBinaryRequest(con, data, dataLength, topic, &rspData, &rspDataLength, timeout);

    if(err == RT_OK)
    {
        rbusMessage_FromBytes(res, rspData, rspDataLength);
    }

    rtMessage_FreeByteArray(rspData);

    return err;
}

rbusCoreError_t rbus_invokeRemoteMethod(const char * object_name, const char *method, rbusMessage out, int timeout_millisecs, rbusMessage *in)
{
    return rbus_invokeRemoteMethod2(g_connection, object_name, method, out, timeout_millisecs, in);
}

rbusCoreError_t rbus_invokeRemoteMethod2(rtConnection myConn, const char * object_name, const char *method, rbusMessage out, int timeout_millisecs, rbusMessage *in)
{
    rtError err = RT_OK;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    char const *traceParent = NULL;
    char const *traceState = NULL;

    rbus_getOpenTelemetryContext(&traceParent, &traceState);

    if(NULL == myConn)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if(MAX_OBJECT_NAME_LENGTH <= strnlen(object_name, MAX_OBJECT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Object name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    *in = NULL;
    if(NULL == out)
        rbusMessage_Init(&out);

    _rbusMessage_SetMetaInfo(out, method, traceParent, traceState);

    err = rbus_sendRequest(myConn, out, object_name, in, timeout_millisecs);
    if(RT_OK != err)
    {
        if(RT_OBJECT_NO_LONGER_AVAILABLE == err)
        {
            RBUSCORELOG_DEBUG("Cannot reach object %s.", object_name);
            ret = RBUSCORE_ERROR_ENTRY_NOT_FOUND;
        }
        else if(RT_ERROR_TIMEOUT == err)
        {
            RBUSCORELOG_ERROR("Request timed out. Error code: 0x%x", err);
            ret = RBUSCORE_ERROR_REMOTE_TIMED_OUT;
        }
        else
        {
            RBUSCORELOG_ERROR("Failed to send message. Error code: 0x%x", err);
            ret = RBUSCORE_ERROR_GENERAL;
        }
    }
    else
    {
        method = NULL;
        rbusMessage_BeginMetaSectionRead(*in);
		    rbusMessage_GetString(*in, &method);
        rbusMessage_EndMetaSectionRead(*in);
        if(NULL != method)
        {
            if(0 != strncmp(METHOD_RESPONSE, method, MAX_METHOD_NAME_LENGTH))
            {
                RBUSCORELOG_ERROR("%s.", stringify(RBUSCORE_ERROR_MALFORMED_RESPONSE));
                ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
            }
        }
        else
        {
            RBUSCORELOG_ERROR("%s.", stringify(RBUSCORE_ERROR_MALFORMED_RESPONSE));
            ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
        }
    }

    rbusMessage_Release(out);
    if((RBUSCORE_SUCCESS != ret) && (NULL != *in))
    {
        rbusMessage_Release(*in);
        *in = NULL;
    }
    return ret;
}


/*TODO: make this really fire and forget.*/
rbusCoreError_t rbus_pushObjNoAck(const char * object_name, rbusMessage message)
{
	return rbus_pushObj(object_name, message, TIMEOUT_VALUE_FIRE_AND_FORGET);
}

rbusCoreError_t rbus_pullObj(const char * object_name, int timeout_millisecs, rbusMessage *response)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtError err = RT_OK;
    if((ret = rbus_invokeRemoteMethod(object_name, METHOD_GETPARAMETERVALUES, NULL, timeout_millisecs, response)) != RBUSCORE_SUCCESS)
    {
        RBUSCORELOG_ERROR("Failed to send message. Error code: 0x%x", ret);
    }
    else
    {
        int result = RBUSCORE_SUCCESS;
        if((err = rbusMessage_GetInt32(*response, &result) == RT_OK))
        {
            ret = (rbusCoreError_t)result;
        }
        else
        {
            RBUSCORELOG_ERROR("%s.", stringify(RBUSCORE_ERROR_MALFORMED_RESPONSE));
            ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
        }
        if(RBUSCORE_SUCCESS != ret) 
        {
            rbusMessage_Release(*response);
            *response = NULL;
        }
    }
    return ret;
}

rbusCoreError_t rbus_sendData(const void* data, uint32_t dataLength, const char * topic)
{
    rtError ret;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    ret = rtConnection_SendBinaryDirect(g_connection, data, dataLength, topic, NULL);
    return translate_rt_error(ret);
}

static rbusCoreError_t rbus_sendMessage(rbusMessage msg, const char * destination, const char * sender)
{
    rtError ret;
    uint8_t* data = NULL;
    uint32_t dataLength = 0;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    rbusMessage_ToBytes(msg, &data, &dataLength);
    ret = rtConnection_SendBinaryDirect(g_connection, data, dataLength, destination, sender);
    return translate_rt_error(ret);
}

static int subscription_handler(const char *not_used, const char * method_name, rbusMessage in, void * user_data, rbusMessage *out, const rtMessageHeader* hdr)
{
    (void) hdr;
    /*using namespace rbus_server;*/
    const char * sender = NULL;
    const char * event_name = NULL;
    int has_payload = 0;
    rbusMessage payload = NULL;
    server_object_t obj = (server_object_t)user_data;
    (void)not_used;

    rbusMessage_Init(out);

    if((RT_OK == rbusMessage_GetString(in, &event_name)) &&
        (RT_OK == rbusMessage_GetString(in, &sender))) 
    {
        /*Extract arguments*/
        if((NULL == sender) || (NULL == event_name))
        {
            RBUSCORELOG_ERROR("Malformed subscription request. Sender: %s. Event: %s.", sender, event_name);
            rbusMessage_SetInt32(*out, RBUSCORE_ERROR_INVALID_PARAM);
        }
        else
        {
            rbusMessage_GetInt32(in, &has_payload);
            if(has_payload)
                rbusMessage_GetMessage(in, &payload);
            int added = strncmp(method_name, METHOD_SUBSCRIBE, MAX_METHOD_NAME_LENGTH) == 0 ? 1 : 0;
            rbusCoreError_t ret = server_object_subscription_handler(obj, event_name, sender, added, payload);
            if(payload)
                rbusMessage_Release(payload);
            rbusMessage_SetInt32(*out, ret);
        }
    }
    
    return 0;
}

static void rtrouted_advisory_callback(rtMessageHeader const* hdr, uint8_t const* data, uint32_t dataLen, void* closure)
{
    rtMessage msg;
    (void)hdr;
    (void)closure;
    int32_t advisory_event;

    if(!g_client_disconnect_callback)
        return;

    rtMessage_FromBytes(&msg, data, dataLen);
    if(rtMessage_GetInt32(msg, RTMSG_ADVISE_EVENT, &advisory_event) == RT_OK)
    {
        if(advisory_event == rtAdviseClientDisconnect)
        {
            const char* listener;
            if(rtMessage_GetString(msg, RTMSG_ADVISE_INBOX, &listener) == RT_OK)
            {
                RBUSCORELOG_DEBUG("Advisory event: client disconnect %s", listener);
                g_client_disconnect_callback(listener);
            }
            else
            {
                RBUSCORELOG_ERROR("Failed to get inbox from advisory msg");
            }
        }
    }
    else
    {
        RBUSCORELOG_ERROR("Failed to get event from advisory msg");
    }

    rtMessage_Release(msg);

    return;
}

static rbusCoreError_t install_subscription_handlers(server_object_t object)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS; 

    server_method_t method = rtVector_Find(object->methods, METHOD_SUBSCRIBE, server_method_compare);

    if(method)
    {
        RBUSCORELOG_INFO("Object already accepts subscription requests.");
        return ret;
    }

    /*No subscription handlers present. Add them.*/
    RBUSCORELOG_DEBUG("Adding handler for subscription requests for %s.", object->name);
    if((ret = rbus_registerMethod(object->name, METHOD_SUBSCRIBE, subscription_handler, object)) != RBUSCORE_SUCCESS)
    {
        RBUSCORELOG_ERROR("Could not register add_subscription_handler.");
    }
    else
    {
        if((ret = rbus_registerMethod(object->name, METHOD_UNSUBSCRIBE, subscription_handler, object)) != RBUSCORE_SUCCESS)
        {
            RBUSCORELOG_ERROR("Could not register remove_subscription_handler.");
        }
        else
        {
            RBUSCORELOG_DEBUG("Successfully registered subscription handlers for %s.", object->name);
            object->process_event_subscriptions = true;
        }
    }
    return ret;
}

rbusCoreError_t rbus_registerEvent(const char* object_name, const char * event_name, rbus_event_subscribe_callback_t callback, void * user_data)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    server_object_t obj;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if(NULL == event_name)
        event_name = DEFAULT_EVENT;
    if(NULL == object_name)
    {
        RBUSCORELOG_ERROR("Invalid parameter(s)");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    if(MAX_EVENT_NAME_LENGTH <= strnlen(event_name, MAX_EVENT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Event name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    lock();
    obj = get_object(object_name);
    if(obj)
    {
        server_event_t evt = rtVector_Find(obj->subscriptions, event_name, server_event_compare);

        if(evt)
        {
            RBUSCORELOG_INFO("Event %s already exists in subscription table.", event_name);
        }
        else
        {
            server_event_create(&evt, event_name, obj, callback, user_data);
            rtVector_PushBack(obj->subscriptions, evt);
            RBUSCORELOG_INFO("Registered event %s::%s.", object_name, event_name);
        }
        if(!obj->process_event_subscriptions)
            ret = install_subscription_handlers(obj);
    }
    else
    {
        RBUSCORELOG_ERROR("Could not find object %s", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();
    return ret;
}

rbusCoreError_t rbus_unregisterEvent(const char* object_name, const char * event_name)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    if(NULL == event_name)
        event_name = DEFAULT_EVENT;

    lock();

    server_object_t obj = get_object(object_name);
    if(obj)
    {
        server_event_t evt = rtVector_Find(obj->subscriptions, event_name, server_event_compare);

        if(evt)
        {
            rtVector_RemoveItem(obj->subscriptions, evt, server_event_destroy);
            RBUSCORELOG_INFO("Event %s::%s has been unregistered.", object_name, event_name);
            /* If we've removed all events and RPC registrations, delete the object itself.*/
        }
        else
        {
            RBUSCORELOG_INFO("Event %s could not be found in subscription table of object %s.", event_name, object_name);
            ret = RBUSCORE_ERROR_INVALID_PARAM;
        }
    }
    else
    {
    
        RBUSCORELOG_ERROR("Could not find object %s", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();
    return ret;
}

static void master_event_callback(rtMessageHeader const* hdr, uint8_t const* data, uint32_t dataLen, void* closure)
{
    /*using namespace rbus_client;*/
    rbusMessage msg = NULL;
    const char * sender = hdr->reply_topic;
    const char * event_name = NULL;
    const char * object_name = NULL;
    int32_t is_rbus_flag = 1;
    rtError err;
    size_t subs_len;
    size_t i;
    (void)closure;

   /*Sanitize the incoming data.*/
    if(MAX_OBJECT_NAME_LENGTH <= strlen(sender))
    {
        RBUSCORELOG_ERROR("Object name length exceeds limits.");
        return;
    }

    rbusMessage_FromBytes(&msg, data, dataLen);

    rbusMessage_BeginMetaSectionRead(msg);
    err = rbusMessage_GetString(msg, &event_name);
    err = rbusMessage_GetString(msg, &object_name);
    err = rbusMessage_GetInt32(msg, &is_rbus_flag);
    rbusMessage_EndMetaSectionRead(msg);
    if(RT_OK != err)
    {
        RBUSCORELOG_ERROR("Event message doesn't contain an event name.");
        rbusMessage_Release(msg);
        return;
    }

    if(is_rbus_flag)
    {
        if(g_master_event_callback)
        {
            err = g_master_event_callback(sender, event_name, msg, g_master_event_user_data);
            if(err != RBUSCORE_ERROR_EVENT_NOT_HANDLED)
            {
                rbusMessage_Release(msg);
                return;
            }
        }
        else
        {
            RBUSCORELOG_ERROR("Received rbus event but no master callback registered yet.");
        }
    }

    lock();
    subs_len = rtVector_Size(g_event_subscriptions_for_client);
    for(i = 0; i < subs_len; ++i)
    {
        client_subscription_t sub = rtVector_At(g_event_subscriptions_for_client, i);

        if( strncmp(sub->object, sender, MAX_OBJECT_NAME_LENGTH) == 0 ||
            strncmp(sub->object, event_name, MAX_OBJECT_NAME_LENGTH) == 0 ) /* support rbus events being elements : the object name will be the event name */
        {
            client_event_t evt = rtVector_Find(sub->events, event_name, client_event_compare);

            if(evt)
            {
                unlock();
                evt->callback(sender, event_name, msg, evt->data);
                rbusMessage_Release(msg);
                return;
            }
            /* support rbus events being elements : keep searching */
        }
    }
    /* If no matching objects exist in records. Create a new entry.*/
    unlock();
    RBUSCORELOG_WARN("Received event %s::%s for which no subscription exists.", sender, event_name);
    rbusMessage_Release(msg);
    return;
}

static rbusCoreError_t remove_subscription_callback(const char * object_name,  const char * event_name)
{
    /*using namespace rbus_client;*/
    client_subscription_t sub;
    rbusCoreError_t ret = RBUSCORE_ERROR_INVALID_PARAM;

    lock();
    sub = rtVector_Find(g_event_subscriptions_for_client, object_name, client_subscription_compare);
    if(sub)
    {
        client_event_t evt = rtVector_Find(sub->events, event_name, client_event_compare);
        if(evt)
        {
            rtVector_RemoveItem(sub->events, evt, rtVector_Cleanup_Free);
            RBUSCORELOG_DEBUG("Subscription removed for event %s::%s.", object_name, event_name);
            ret = RBUSCORE_SUCCESS;

            if(rtVector_Size(sub->events) == 0)
            {
                RBUSCORELOG_DEBUG("Zero event subscriptions remaining for object %s. Cleaning up.", object_name);
                rtVector_RemoveItem(g_event_subscriptions_for_client, sub, client_subscription_destroy);
            }
        }
        else
        {
            RBUSCORELOG_WARN("Subscription for event %s::%s not found.", object_name, event_name);
        }
    }
    unlock();
    return ret;
}

static rbusCoreError_t rbus_subscribeToEventInternal(const char * object_name,  const char * event_name, rbus_event_callback_t callback, const rbusMessage payload, void * user_data, int* providerError, int timeout, bool publishOnSubscribe, rbusMessage *response, bool rawData)
{
    /*using namespace rbus_client;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    client_subscription_t sub;
    client_event_t evt;

    /* support rbus events being elements : use the event_name as the object_name because event_name is alias to object */
    if(object_name == NULL && event_name != NULL) 
        object_name = event_name;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if((NULL == object_name) || (NULL == callback))
    {
        RBUSCORELOG_ERROR("Invalid parameter(s)");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    if(MAX_OBJECT_NAME_LENGTH <= strnlen(object_name, MAX_OBJECT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Object name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    if(MAX_EVENT_NAME_LENGTH <= strnlen(event_name, MAX_EVENT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Event name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    lock();
   
    if(NULL == event_name)
        event_name = DEFAULT_EVENT;

    if(false == g_run_event_client_dispatch)
    {
        RBUSCORELOG_DEBUG("Starting event dispatching.");
        rtConnection_AddDefaultListener(g_connection, master_event_callback, NULL);
        g_run_event_client_dispatch = true;
    }

    if(g_master_event_callback == NULL)
    {
        sub = rtVector_Find(g_event_subscriptions_for_client, object_name, client_subscription_compare);
        if(sub)
        {
            if(rtVector_Find(sub->events, event_name, client_event_compare))
            {
                /*sub already exist and event already registered so do nothing*/
                RBUSCORELOG_WARN("Subscription exists for event %s::%s.", object_name, event_name);
                unlock();
                return RBUSCORE_SUCCESS;
            }
        }
        else
        {
            /*sub didn't exist so create it*/
            client_subscription_create(&sub, object_name);
            rtVector_PushBack(g_event_subscriptions_for_client, sub);
        }

        /*create event and add to sub*/
        client_event_create(&evt, event_name, callback, user_data);
        rtVector_PushBack(sub->events, evt);
    }
    RBUSCORELOG_DEBUG("Added subscription for event %s::%s.", object_name, event_name);

    unlock();

    if((ret = send_subscription_request(object_name, event_name, true, payload, providerError, timeout, publishOnSubscribe, response, rawData)) != RBUSCORE_SUCCESS)
    {
        if(g_master_event_callback == NULL)
        {
            /*Something went wrong in the RPC. Undo what we did so far and report error.*/
            lock();
            remove_subscription_callback(object_name, event_name);
            unlock();
        }
    }
    return ret;
}

rbusCoreError_t rbus_subscribeToEvent(const char * object_name,  const char * event_name, rbus_event_callback_t callback, const rbusMessage payload, void * user_data, int* providerError)
{
    return rbus_subscribeToEventInternal(object_name, event_name, callback, payload, user_data, providerError, 0, false, NULL, false);
}

rbusCoreError_t rbus_subscribeToEventTimeout(const char * object_name,  const char * event_name, rbus_event_callback_t callback, const rbusMessage payload, void * user_data, int* providerError, int timeout, bool publishOnSubscribe, rbusMessage *response, bool rawData)
{
    return rbus_subscribeToEventInternal(object_name, event_name, callback, payload, user_data, providerError, timeout, publishOnSubscribe, response, rawData);
}

rbusCoreError_t rbus_unsubscribeFromEvent(const char * object_name,  const char * event_name, const rbusMessage payload, bool rawData)
{
    rbusCoreError_t ret = RBUSCORE_ERROR_INVALID_PARAM;

    /* support rbus events being elements */
    if(object_name == NULL && event_name != NULL) 
        object_name = event_name;

    if(NULL == object_name)
    {
        RBUSCORELOG_ERROR("Invalid parameter(s)");
        return ret;
    }
    if(MAX_OBJECT_NAME_LENGTH <= strnlen(object_name, MAX_OBJECT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Object name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    if(NULL == event_name)
        event_name = DEFAULT_EVENT;

    if(!g_master_event_callback)
        remove_subscription_callback(object_name, event_name);
    ret = send_subscription_request(object_name, event_name, false, payload, NULL, 0, false, NULL, rawData);
    return ret;
}

rbusCoreError_t rbus_publishEvent(const char* object_name,  const char * event_name, rbusMessage out)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if(NULL == event_name)
        event_name = DEFAULT_EVENT;
    if(MAX_OBJECT_NAME_LENGTH <= strnlen(object_name, MAX_OBJECT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Object name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    rbusMessage_BeginMetaSectionWrite(out);
    rbusMessage_SetString(out, event_name);
    rbusMessage_SetString(out, object_name); 
    rbusMessage_SetInt32(out, 0); /*is ccsp and not rbus 2.0*/
    rbusMessage_EndMetaSectionWrite(out);

    lock();
    server_object_t obj = get_object(object_name);
    if(obj)
    {
        server_event_t evt = rtVector_Find(obj->subscriptions, event_name, server_event_compare);

        if(evt)
        {
            size_t nlistener, i;

            nlistener = rtVector_Size(evt->listeners);
            RBUSCORELOG_DEBUG("Event %s exists in subscription table. Dispatching to %zu subscribers.", event_name, nlistener);
            for(i=0; i < nlistener; ++i)
            {
                char const* listener = (char const*)rtVector_At(evt->listeners, i);
                if(RBUSCORE_SUCCESS != rbus_sendMessage(out, listener, object_name))
                {
                    RBUSCORELOG_ERROR("Couldn't send event %s::%s to %s.", object_name, event_name, listener);
                }
            }
        }
        else
        {
            RBUSCORELOG_ERROR("Could not find event %s", event_name);
            ret = RBUSCORE_ERROR_INVALID_PARAM;
        }
    }
    else 
    {
        /*Object not present yet. Register it now.*/
        RBUSCORELOG_ERROR("Could not find object %s", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();

    return ret;
}

rbusCoreError_t rbus_registerSubscribeHandler(const char* object_name, rbus_event_subscribe_callback_t callback, void * user_data)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    if((NULL == object_name) || (NULL == callback))
    {
        RBUSCORELOG_ERROR("Invalid parameter(s)");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    lock();
    server_object_t obj = get_object(object_name);
    if(obj)
    {
        obj->subscribe_handler_override = callback;
        obj->subscribe_handler_data = user_data;
        if(!obj->process_event_subscriptions)
            ret = install_subscription_handlers(obj);
    }
    else
    {
        RBUSCORELOG_ERROR("Could not find object %s", object_name);
        ret = RBUSCORE_ERROR_INVALID_PARAM;
    }
    unlock();
    return ret;
}

rbusCoreError_t rbus_registerMasterEventHandler(rbus_event_callback_t callback, void * user_data)
{
    g_master_event_callback = callback;
    g_master_event_user_data = user_data;
    return RBUSCORE_SUCCESS;
}
rbusCoreError_t rbus_registerClientDisconnectHandler(rbus_client_disconnect_callback_t callback)
{
    lock();
    if(!g_advisory_listener_installed)
    {
        rtError err = rtConnection_AddListenerWithId(g_connection, RTMSG_ADVISORY_TOPIC, RBUS_ADVISORY_EXPRESSION_ID, &rtrouted_advisory_callback, g_connection);
        if(err == RT_OK)
        {
            RBUSCORELOG_DEBUG("Listening for advisory messages");
        }
        else
        {
            RBUSCORELOG_ERROR("Failed to add advisory listener: %d", err);
            unlock();
            return RBUSCORE_ERROR_GENERAL;
        }
        g_advisory_listener_installed = true;
        g_client_disconnect_callback = callback;
    }
    unlock();
    return RBUSCORE_SUCCESS;
}

rbusCoreError_t rbus_unregisterClientDisconnectHandler()
{
    lock();
    if(g_advisory_listener_installed)
    {
        rtConnection_RemoveListenerWithId(g_connection, RTMSG_ADVISORY_TOPIC, RBUS_ADVISORY_EXPRESSION_ID);
        g_advisory_listener_installed = false;
    }
    unlock();
    return RBUSCORE_SUCCESS;
}

rbusCoreError_t rbuscore_publishDirectSubscriberEvent(const char * event_name, const char* listener, const void* data, uint32_t dataLength, uint32_t subscriptionId, bool rawData)
{
    rtError err = RT_OK;

    directServerLock();
    const rtPrivateClientInfo *pPrivCliInfo = _rbuscore_find_server_privateconnection (event_name, listener);
    if(pPrivCliInfo)
    {
        err = rtRouteDirect_SendMessage (pPrivCliInfo, data, dataLength, subscriptionId, rawData);
    }
    directServerUnlock();
    return translate_rt_error(err);
}

rbusCoreError_t rbus_publishSubscriberEvent(const char* object_name,  const char * event_name, const char* listener, rbusMessage out, uint32_t subscriptionId, bool rawData)
{
    /*using namespace rbus_server;*/
    rbusCoreError_t ret = RBUSCORE_SUCCESS;

    if(NULL == event_name)
        event_name = DEFAULT_EVENT;
    if(MAX_OBJECT_NAME_LENGTH <= strnlen(object_name, MAX_OBJECT_NAME_LENGTH))
    {
        RBUSCORELOG_ERROR("Object name is too long.");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }
    rbusMessage_BeginMetaSectionWrite(out);
    rbusMessage_SetString(out, event_name);
    rbusMessage_SetString(out, object_name); 
    rbusMessage_SetInt32(out, 1);/*is rbus 2.0*/ 
    rbusMessage_EndMetaSectionWrite(out);

    directServerLock();
    const rtPrivateClientInfo *pPrivCliInfo = _rbuscore_find_server_privateconnection (event_name, listener);
    if(pPrivCliInfo)
    {
        uint8_t* data;
        uint32_t dataLength;
        rbusMessage_ToBytes(out, &data, &dataLength);
        rtRouteDirect_SendMessage (pPrivCliInfo, data, dataLength, subscriptionId, rawData);
    }
    directServerUnlock();
    if (!pPrivCliInfo)
    {
        lock();
        server_object_t obj = get_object(object_name);
        if(NULL == obj)
        {
            /*Object not present yet. Register it now.*/
            RBUSCORELOG_ERROR("Could not find object %s", object_name);
            ret = RBUSCORE_ERROR_INVALID_PARAM;
        }

        if(rbus_sendMessage(out, listener, object_name) != RBUSCORE_SUCCESS)
        {
           RBUSCORELOG_ERROR("Couldn't send event %s::%s to %s.", object_name, event_name, listener);
        }
        unlock();
    }
    return ret;
}

rbusCoreError_t rbus_discoverWildcardDestinations(const char * expression, int * count, char *** destinations)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtError err = RT_OK;
    rtMessage msg, rsp;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if((NULL == expression) || (NULL == count) || (NULL == destinations))
    {
        RBUSCORELOG_ERROR("expression/count/destinations pointer is NULL");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    rtMessage_Create(&msg);
    rtMessage_SetString(msg, RTM_DISCOVERY_EXPRESSION, expression);

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_WILDCARD_DESTINATIONS, &rsp, TIMEOUT_VALUE_FIRE_AND_FORGET);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int result;
        const char * value = NULL;

        if((RT_OK == rtMessage_GetInt32(msg, RTM_DISCOVERY_RESULT, &result)) && (RT_OK == result))
        {
            int32_t size, length, i;

            rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
            rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

            if(size != length)
            {
                RBUSCORELOG_ERROR("rbus_resolveWildcardDestination size missmatch");
            }

            if(size && length)
            {
                char **array_ptr = (char **)rt_try_malloc(size * sizeof(char *));
                *count = size;
                if (NULL != array_ptr)
                {
                    *destinations = array_ptr;
                    memset(array_ptr, 0, (length * sizeof(char *)));
                    for (i = 0; i < length; i++)
                    {
                        if ((RT_OK != rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value)) || (NULL == (array_ptr[i] = strndup(value, MAX_OBJECT_NAME_LENGTH))))
                        {
                            for (int j = 0; j < i; j++)
                                free(array_ptr[j]);
                            free(array_ptr);
                            RBUSCORELOG_ERROR("Read/Memory allocation failure");
                            ret = RBUSCORE_ERROR_GENERAL;
                            break;
                        }
                    }
                }
                else
                {
                    RBUSCORELOG_ERROR("Memory allocation failure");
                    ret = RBUSCORE_ERROR_INSUFFICIENT_MEMORY;
                }
            }

            rtMessage_Release(msg);

            ret = RBUSCORE_SUCCESS;

        }
        else
        {
            ret = RBUSCORE_ERROR_GENERAL;
            rtMessage_Release(msg);
        }
    }
    else
    {
        ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
    }
    return ret;
}

rbusCoreError_t rbus_discoverObjectElements(const char * object, int * count, char *** elements)
{
    rtError err = RT_OK;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtMessage msg, rsp;

    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    if((NULL == object) || (NULL == elements) || (NULL == count))
    {
        RBUSCORELOG_ERROR("Object/elements/count is NULL");
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    rtMessage_Create(&msg);
    rtMessage_SetString(msg, RTM_DISCOVERY_EXPRESSION, object);

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_OBJECT_ELEMENTS, &rsp, TIMEOUT_VALUE_FIRE_AND_FORGET);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int32_t size, length, i;
        const char * value = NULL;
        char **array_ptr = NULL;

        *elements = NULL;

        rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
        rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

        if(size != length)
        {
            RBUSCORELOG_ERROR("rbus_GetElementsAddedByObject size missmatch");
        }

        *count = size;
        if(size && length)
        {
            array_ptr = (char **)rt_try_malloc(size * sizeof(char *));
            if (NULL != array_ptr)
            {
                *elements = array_ptr;
                memset(array_ptr, 0, (length * sizeof(char *)));
                for (i = 0; i < length; i++)
                {
                    if ((RT_OK != rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value)) || (NULL == (array_ptr[i] = strndup(value, MAX_OBJECT_NAME_LENGTH))))
                    {
                        for (int j = 0; j < i; j++)
                            free(array_ptr[j]);
                        free(array_ptr);
                        array_ptr=NULL;
			*elements = NULL;
                        RBUSCORELOG_ERROR("Read/Memory allocation failure");
                        ret = RBUSCORE_ERROR_GENERAL;
                        break;
                    }
                }
            }
            else
            {
                RBUSCORELOG_ERROR("Memory allocation failure");
                ret = RBUSCORE_ERROR_INSUFFICIENT_MEMORY;
            }
        }

        rtMessage_Release(msg);

        ret = RBUSCORE_SUCCESS;
    }
    else
    {
        ret = RBUSCORE_ERROR_GENERAL;
    }

    return ret;
}

rbusCoreError_t rbus_discoverElementObjects(const char* element, int * count, char *** objects)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtError err = RT_OK;
    rtMessage msg, rsp;

    rtMessage_Create(&msg);
    if(NULL != element)
    {
        rtMessage_SetInt32(msg, RTM_DISCOVERY_COUNT, 1);
        rtMessage_AddString(msg, RTM_DISCOVERY_ITEMS, element);
    }
    else
    {
        RBUSCORELOG_ERROR("Null entries in element list.");
        rtMessage_Release(msg);
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_ELEMENT_OBJECTS, &rsp, TIMEOUT_VALUE_FIRE_AND_FORGET);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int result;
        const char * value = NULL;

        if((RT_OK == rtMessage_GetInt32(msg, RTM_DISCOVERY_RESULT, &result)) && (RT_OK == result))
        {
            int num_elements = 0;
            rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &num_elements);
            *count = num_elements;

            if(num_elements)
            {
                char **array_ptr = (char **)rt_try_malloc(num_elements * sizeof(char *));
                if (NULL != array_ptr)
                {
                    *objects = array_ptr;
                    memset(array_ptr, 0, (num_elements * sizeof(char *)));
                    for (int i = 0; i < num_elements; i++)
                    {
                        if ((RT_OK != rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value)) || (NULL == (array_ptr[i] = strndup(value, MAX_OBJECT_NAME_LENGTH))))
                        {
                            for (int j = 0; j < i; j++)
                                free(array_ptr[j]);
                            free(array_ptr);
                            RBUSCORELOG_ERROR("Read/Memory allocation failure");
                            ret = RBUSCORE_ERROR_GENERAL;
                            break;
                        }
                    }
                }
                else
                {
                    RBUSCORELOG_ERROR("Memory allocation failure");
                    ret = RBUSCORE_ERROR_INSUFFICIENT_MEMORY;
                }
            }
        }
        else
        {
            ret = RBUSCORE_ERROR_GENERAL;
        }
        rtMessage_Release(msg);
    }
    else
    {
        ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
    }
    
    return ret;    
}

rbusCoreError_t rbus_discoverElementsObjects(int numElements, const char** elements, int * count, char *** objects)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtError err = RT_OK;
    rtMessage msg, rsp;
    char** array_ptr = NULL;
    int array_count = 0;

    *count = 0;

    rtMessage_Create(&msg);
    if(NULL != elements)
    {
        int i;
        rtMessage_SetInt32(msg, RTM_DISCOVERY_COUNT, numElements);
        for(i = 0; i < numElements; ++i)
            rtMessage_AddString(msg, RTM_DISCOVERY_ITEMS, elements[i]);
    }
    else
    {
        RBUSCORELOG_ERROR("Null entries in element list.");
        rtMessage_Release(msg);
        return RBUSCORE_ERROR_INVALID_PARAM;
    }

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_ELEMENT_OBJECTS, &rsp, TIMEOUT_VALUE_FIRE_AND_FORGET);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int result;

        if((RT_OK == rtMessage_GetInt32(msg, RTM_DISCOVERY_RESULT, &result)) && (RT_OK == result))
        {
            int i;

            for(i = 0; i < numElements && ret == RBUSCORE_SUCCESS; ++i)
            {
                int numComponents = 0;
                const char* component = NULL;

                if(rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &numComponents) == RT_OK)
                {
                    char **next = NULL;
                    if(numComponents)
                    {
                        if(!array_ptr)
                            next = (char **)rt_try_malloc(numComponents * sizeof(char *));
                        else
                            next = (char **)rt_try_realloc(array_ptr, (array_count + numComponents) * sizeof(char *));
                        if (!next)
                        {
                            RBUSCORELOG_ERROR("Memory allocation failure");
                            ret = RBUSCORE_ERROR_GENERAL;
                            break;
                        }
                        array_ptr = next;
                        for (int j = 0; j < numComponents; j++)
                        {
                            if (RT_OK != rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, array_count, &component))
                            {
                                RBUSCORELOG_ERROR("Read item failure");
                                ret = RBUSCORE_ERROR_GENERAL;
                                break;
                            }
                            if(component[0]) /*rtrouted will put a 0 len string if no route found*/
                            {
                                if (NULL == (array_ptr[array_count++] = strndup(component, MAX_OBJECT_NAME_LENGTH)))
                                {
                                    RBUSCORELOG_ERROR("Memory allocation failure");
                                    ret = RBUSCORE_ERROR_GENERAL;
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    RBUSCORELOG_ERROR("rbus_discoverElementsObjects: failed at %s", elements[i]);
                    ret = RBUSCORE_ERROR_GENERAL;
                    break;
                }
            }
        }
        else
        {
            ret = RBUSCORE_ERROR_GENERAL;
        }
        rtMessage_Release(msg);
    }
    else
    {
        ret = RBUSCORE_ERROR_MALFORMED_RESPONSE;
    }

    if (ret == RBUSCORE_SUCCESS)
    {
        *count = array_count;
        *objects = array_ptr;
    }
    else
    {
        if(array_ptr)
        {
            for (int i = 0; i < array_count; i++)
                free(array_ptr[i]);
            free(array_ptr);
        }
    }
    return ret;    
}

rbusCoreError_t rbus_discoverRegisteredComponents(int * count, char *** components)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtError err = RT_OK;
    rtMessage msg;
    rtMessage out;
    rtMessage_Create(&out);
    rtMessage_SetInt32(out, "dummy", 0);
    
    if(NULL == g_connection)
    {
        RBUSCORELOG_ERROR("Not connected.");
        rtMessage_Release(out);
        return RBUSCORE_ERROR_INVALID_STATE;
    }

    err = rtConnection_SendRequest(g_connection, out, RTM_DISCOVER_REGISTERED_COMPONENTS, &msg, TIMEOUT_VALUE_FIRE_AND_FORGET);

    if(RT_OK == err)
    {
        int32_t size, length, i;
        const char * value = NULL;

        rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
        rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

        if(size != length)
        {
            RBUSCORELOG_ERROR("rbus_registeredComponents size missmatch");
        }

        char **array_ptr = (char **)rt_try_malloc(size * sizeof(char *));
        *count = size;
        if (NULL != array_ptr)
        {
            *components = array_ptr;
            memset(array_ptr, 0, (length * sizeof(char *)));
            for (i = 0; i < length; i++)
            {
                if ((RT_OK != rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value)) || (NULL == (array_ptr[i] = strndup(value, MAX_OBJECT_NAME_LENGTH))))
                {
                    for (int j = 0; j < i; j++)
                        free(array_ptr[j]);
                    free(array_ptr);
                    RBUSCORELOG_ERROR("Read/Memory allocation failure");
                    ret = RBUSCORE_ERROR_GENERAL;
                    break;
                }
            }
        }
        else
        {
            RBUSCORELOG_ERROR("Memory allocation failure");
            ret = RBUSCORE_ERROR_INSUFFICIENT_MEMORY;
        }

        rtMessage_Release(msg);
        ret = RBUSCORE_SUCCESS;
    }
    else
    {
        RBUSCORELOG_ERROR("Failed with error code %d", err);
        ret = RBUSCORE_ERROR_GENERAL;
    }

    rtMessage_Release(out);
    return ret;
}

rbuscore_bus_status_t rbuscore_checkBusStatus(void)
{
#ifdef RBUS_SUPPORT_DISABLING
    if(0 != access("/nvram/rbus_disable", F_OK))
    {
        RBUSCORELOG_INFO ("Currently RBus Enabled");
        return RBUSCORE_ENABLED;
    }
    else
    {
        RBUSCORELOG_INFO ("Currently RBus Disabled");
        return RBUSCORE_DISABLED;
    }
#else
    RBUSCORELOG_INFO ("RBus Enabled");
    return RBUSCORE_ENABLED;
#endif /* RBUS_SUPPORT_DISABLING */
}

rbusCoreError_t rbus_sendResponse(const rtMessageHeader* hdr, rbusMessage response)
{
    rtError err = RT_OK;
    uint8_t* data;
    uint32_t dataLength;

    if(rtMessageHeader_IsRequest(hdr))
    {
        /* The origin of this message expects a response.*/
        if(NULL == response)
        {
            /* App declined to issue a response. Make one up ourselves. */
            rbusMessage_Init(&response);
            rbusMessage_SetInt32(response, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
        }

        _rbusMessage_SetMetaInfo(response, METHOD_RESPONSE, NULL, NULL);

        rbusMessage_ToBytes(response, &data, &dataLength);

        if((err= rtConnection_SendBinaryResponse(g_connection, hdr, data, dataLength, TIMEOUT_VALUE_FIRE_AND_FORGET)) != RT_OK)
        {
            RBUSCORELOG_ERROR("Failed to send async response. Error code: 0x%x", err);
        }
        rbusMessage_Release(response);
    }
    return err == RT_OK ? RBUSCORE_SUCCESS : RBUSCORE_ERROR_GENERAL;
}

rbusOpenTelemetryContext*
rbus_getOpenTelemetryContextFromThreadLocal()
{
    pthread_once(&_open_telemetry_once, &rbus_init_open_telemeetry_thread_specific_key);

    rbusOpenTelemetryContext* ot_ctx = (rbusOpenTelemetryContext *) pthread_getspecific(_open_telemetry_key);
    if (!ot_ctx)
    {
        ot_ctx = malloc(sizeof(rbusOpenTelemetryContext));
        if (ot_ctx)
        {
            memset(ot_ctx->otTraceParent, 0, sizeof(ot_ctx->otTraceParent));
            memset(ot_ctx->otTraceState, 0, sizeof(ot_ctx->otTraceState));
            pthread_setspecific(_open_telemetry_key, ot_ctx);
        }
    }

    return ot_ctx;
}

void rbus_getOpenTelemetryContext(const char **traceParent, const char **traceState)
{
    rbusOpenTelemetryContext* ot_ctx = rbus_getOpenTelemetryContextFromThreadLocal();

    *traceParent = &ot_ctx->otTraceParent[0];
    *traceState = &ot_ctx->otTraceState[0];
}

void rbus_clearOpenTelemetryContext()
{
    rbusOpenTelemetryContext *ot_ctx = rbus_getOpenTelemetryContextFromThreadLocal();
    ot_ctx->otTraceParent[0] = '\0';
    ot_ctx->otTraceState[0] = '\0';
}

static void rbus_releaseOpenTelemetryContext()
{
    rbusOpenTelemetryContext *ot_ctx = rbus_getOpenTelemetryContextFromThreadLocal();
    if (ot_ctx)
    {
        pthread_setspecific(_open_telemetry_key, NULL);
        free (ot_ctx);
    }
}

void rbus_setOpenTelemetryContext(const char *traceParent, const char *traceState)
{
    rbusOpenTelemetryContext *ot_ctx = rbus_getOpenTelemetryContextFromThreadLocal();

    if (traceParent)
    {
	size_t tpLen = strlen(traceParent);
	if ((tpLen > 0) && (tpLen < (RBUS_OPEN_TELEMETRY_DATA_MAX - 1)))
	{
            memset(ot_ctx->otTraceParent, '\0', sizeof(ot_ctx->otTraceParent));
	    strncpy(ot_ctx->otTraceParent, traceParent, tpLen);
            ot_ctx->otTraceParent[tpLen + 1] = '\0';
	}
	else
            ot_ctx->otTraceParent[0] = '\0';
    }
    else
        ot_ctx->otTraceParent[0] = '\0';

    if (traceState)
    {
        size_t tsLen = strlen(traceState);
	if ((tsLen > 0) && (tsLen < (RBUS_OPEN_TELEMETRY_DATA_MAX - 1)))
	{
            memset(ot_ctx->otTraceState, '\0', sizeof(ot_ctx->otTraceState));
	    strncpy(ot_ctx->otTraceState, traceState, tsLen);
            ot_ctx->otTraceState[tsLen + 1] = '\0';
	}
	else
            ot_ctx->otTraceState[0] = '\0';
    }
    else
        ot_ctx->otTraceState[0] = '\0';
}


typedef struct _rbusServerDMLList 
{
    char                    m_privConnAddress[MAX_OBJECT_NAME_LENGTH+1];
    char                    m_consumerName[MAX_OBJECT_NAME_LENGTH+1];
    char                    m_privateDML [MAX_OBJECT_NAME_LENGTH+1];
    rtPrivateClientInfo     m_consumerInfo;
    pthread_t               m_pid;
    rbus_callback_t         m_fnCallbackHandler;
    void*                   m_fnCallbackuserData;
} rbusServerDMLList_t;

typedef struct _server_directHandler
{
    char                  m_privConnAddress[MAX_OBJECT_NAME_LENGTH+1];
    rtDriectClientHandler m_fnRouteCallback;
} rbusServerDirectHandler_t;

typedef struct _rbusClientDMLList
{
    char          m_privateDML[MAX_OBJECT_NAME_LENGTH+1];
    char          m_providerName[MAX_OBJECT_NAME_LENGTH+1];
    char          m_consumerName[MAX_OBJECT_NAME_LENGTH+1];
    rtConnection  m_privConn;
} rbusClientDMLList_t;




/////// Server Side ///////
static int _findPrivateServer(const void* left, const void* right)
{
    return strncmp(((rbusServerDMLList_t*)left)->m_privConnAddress, (char*)right, MAX_OBJECT_NAME_LENGTH);
}

static int _findPrivateServerDML(const void* left, const void* right)
{
    return strncmp(((rbusServerDMLList_t*)left)->m_privateDML, (char*)right, MAX_OBJECT_NAME_LENGTH);
}

static void _freePrivateServer(void* p)
{
    rbusServerDMLList_t *pTmp = p;
    if (pTmp->m_pid)
    {
        pthread_cancel(pTmp->m_pid);
        pthread_join(pTmp->m_pid, NULL);
        printf ("Cancel the thread..\n"); //FIXME
    }
    free(pTmp);
}

#define RBUS_DIRECT_FILE_CACHE "/tmp/.rbus_%s_direct.cache"
#define RBUS_DIRECT_ROW_CACHE_LENGTH 512
static void _rbuscore_directconnection_save_to_cache()
{
    FILE* file;
    size_t sz = 0, i = 0;
    char cacheFileName[256] = "";
    snprintf(cacheFileName, 256, RBUS_DIRECT_FILE_CACHE, __progname);

    sz = rtVector_Size(gListOfServerDirectDMLs);
    if(0 == sz)
    {
        RBUSCORELOG_DEBUG("no direct connection exist, so removing cache file");
        remove(cacheFileName);
    }
    else
    {
        rbusMessage tmp;
        uint8_t* pBuff = NULL;
        uint32_t length = 0;
        rbusServerDMLList_t* pDmlObj = NULL;

        file = fopen(cacheFileName, "wb");
        if(!file)
        {
            RBUSCORELOG_ERROR("failed to open %s", cacheFileName);
            return;
        }

        rbusMessage_Init(&tmp);
        rbusMessage_SetInt32(tmp, sz);
        for(i = 0; i < sz; ++i)
        {
            rbusMessage m;
            rbusMessage_Init(&m);
            pDmlObj = rtVector_At(gListOfServerDirectDMLs, i);
            rbusMessage_SetString(m, pDmlObj->m_privateDML);
            rbusMessage_SetString(m, pDmlObj->m_consumerName);
            rbusMessage_SetMessage(tmp, m);
            rbusMessage_Release(m);
        }

        rbusMessage_ToBytes(tmp, &pBuff, &length);

        fwrite(pBuff, 1, length, file);
        fclose(file);
        rbusMessage_Release(tmp);
    }
}

static void _rbuscore_directconnection_load_from_cache()
{
    struct stat st;
    long size;
    FILE* file = NULL;
    uint8_t* pBuff = NULL;
    char cacheFileName[256] = "";

    snprintf(cacheFileName, 256, RBUS_DIRECT_FILE_CACHE, __progname); 

    RBUSCORELOG_DEBUG("Entry of %s", __FUNCTION__);

    if(stat(cacheFileName, &st) != 0)
    {
        RBUSCORELOG_DEBUG("file doesn't exist");
        return;
    }

    file = fopen(cacheFileName, "rb");
    if(!file)
    {
        RBUSCORELOG_ERROR("failed to open file %s", cacheFileName);
        goto invalidFile;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if(size <= 0)
    {
        RBUSCORELOG_DEBUG("file is empty %s", cacheFileName);
        goto invalidFile;
    }

    pBuff  = rt_malloc(size);
    if(pBuff)
    {
        fseek(file, 0, SEEK_SET);
        if(fread(pBuff, 1, size, file) != (size_t)size)
        {
            RBUSCORELOG_ERROR("failed to read entire file");
            goto invalidFile;
        }

        //
        rbusMessage msg = NULL;
        rbusMessage_FromBytes(&msg, pBuff, size);
        int numOfEntries = 0;
        rbusMessage_GetInt32(msg, &numOfEntries);
        RBUSCORELOG_DEBUG("Number of Entries...%d", numOfEntries);

        for (int i = 0; i < numOfEntries; i++)
        {
            rbusMessage tmpMsg = NULL;
            const char* pDMLName = NULL;
            const char* pConsumerName = NULL;
            rbusMessage_GetMessage(msg, &tmpMsg);
            rbusMessage_GetString(tmpMsg, &pDMLName);
            rbusMessage_GetString(tmpMsg, &pConsumerName);
            RBUSCORELOG_INFO("Direct Connection Existed for DML (%s) for this client(%s)", pDMLName, pConsumerName);

            //TODO
            /* Add it to vector and when add_element is called, start a listener */
            //if the PID is running and if the closeDirect was not called yet, means the consumer is still waiting..
        }
        rbusMessage_Release(msg);
    }

    fclose(file);
    file = NULL;

    if(pBuff)
        free(pBuff);

    pBuff = NULL;
    return;

invalidFile:

    RBUSCORELOG_WARN("removing corrupted file %s", cacheFileName);

    if(file)
        fclose(file);

    if(pBuff)
        free(pBuff);

    remove(cacheFileName);
}

rbusServerDMLList_t* rbuscore_FindServerPrivateClient (const char *pParameterName, const char *pConsumerName)
{
    rbusServerDMLList_t* pDmlObj = NULL;

    if (pParameterName && pConsumerName)
    {
        size_t sz = 0, i = 0;

        sz = rtVector_Size(gListOfServerDirectDMLs);
        if(sz > 0)
        {
            for(i = 0; i < sz; ++i)
            {
                pDmlObj = rtVector_At(gListOfServerDirectDMLs, i);
            
                if ((0 == strncmp(pDmlObj->m_privateDML, pParameterName, MAX_OBJECT_NAME_LENGTH)) &&
                    (0 == strncmp(pDmlObj->m_consumerName, pConsumerName, MAX_OBJECT_NAME_LENGTH)))
                {
                    directServerUnlock();
                    return pDmlObj;
                }
            }
        }
    }

    return NULL;
}

const rtPrivateClientInfo* _rbuscore_find_server_privateconnection(const char *pParameterName, const char *pConsumerName)
{
    rbusServerDMLList_t *pObj = NULL;

    pObj = rbuscore_FindServerPrivateClient (pParameterName, pConsumerName);
    if(pObj)
    {
        return &pObj->m_consumerInfo;
    }

    return NULL;
}

rbusCoreError_t rbuscore_updatePrivateListener(const char* pConsumerName, const char *pDMLName)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rbusServerDMLList_t *pObj = NULL;
    if (pConsumerName && pDMLName)
    {
        pObj = rbuscore_FindServerPrivateClient (pDMLName, pConsumerName);
        if (pObj)
        {
            directServerLock();
            rtVector_RemoveItem(gListOfServerDirectDMLs, pObj, rtVector_Cleanup_Free);
            _rbuscore_directconnection_save_to_cache();
            directServerUnlock();
        }
        else
        {
            RBUSCORELOG_WARN("No Direct Connection for this DML(%s) exist for this consumer(%s)", pDMLName, pConsumerName);
            ret = RBUSCORE_ERROR_INVALID_PARAM;
        }
    }
    else
        ret = RBUSCORE_ERROR_INVALID_PARAM;

    return ret;
}

void* rbuscore_PrivateThreadFunc (void* ptr)
{
    rbusServerDirectHandler_t *pInstance = (rbusServerDirectHandler_t *) ptr;

    sleep(2);
    rtRouteDirect_StartInstance(pInstance->m_privConnAddress, pInstance->m_fnRouteCallback);

    /* coming here means that the thread is client is not connected anymore */
    rbusServerDMLList_t *pSubObj;
    directServerLock();
    pSubObj = rtVector_Find(gListOfServerDirectDMLs, pInstance->m_privConnAddress, _findPrivateServer);
    while(pSubObj != NULL)
    {
        rtVector_RemoveItem(gListOfServerDirectDMLs, pSubObj, rtVector_Cleanup_Free);
        pSubObj = rtVector_Find(gListOfServerDirectDMLs, pInstance->m_privConnAddress, _findPrivateServer);
    }
    _rbuscore_directconnection_save_to_cache();
    directServerUnlock();


    free (pInstance);
    return NULL;
}

static rtError _onDirectMessage(uint8_t isClientRequest, rtMessageHeader* hdr, uint8_t const* pInBuff, int inLength, uint8_t** pOutBuff, uint32_t* pOutLength)
{
    rbusMessage msg = NULL;
    rbusServerDMLList_t *pSubObj;

    if (isClientRequest)
    {
        const char* method_name = NULL;
        const char* traceParent = NULL;
        const char* traceState = NULL;

        rbusMessage_FromBytes(&msg, pInBuff, inLength);

        /* Fetch the context that was sent with the message */
         _rbusMessage_GetMetaInfo(msg, &method_name, &traceParent, &traceState);

        rbusMessage response;
        directServerLock();
        pSubObj = rtVector_Find(gListOfServerDirectDMLs, hdr->topic, _findPrivateServerDML);
        directServerUnlock();
        if(pSubObj)
        {
            pSubObj->m_fnCallbackHandler(hdr->topic, method_name, msg, pSubObj->m_fnCallbackuserData, &response, hdr);
        }
        else
        {
            rbusMessage_Init(&response);
            rbusMessage_SetInt32(response, RBUSCORE_ERROR_UNSUPPORTED_METHOD);
            RBUSCORELOG_WARN("Could not find the DML in Private Connection List..");
        }
        _rbusMessage_SetMetaInfo(response, METHOD_RESPONSE, NULL, NULL);
        uint8_t* pData = NULL;
        uint32_t dataLength = 0;
        rbusMessage_ToBytes(response, &pData, &dataLength);
        if ((pData) && (0 != dataLength))
        {
            *pOutLength = dataLength;
            *pOutBuff = rt_malloc(dataLength);
            memcpy(*pOutBuff, pData, dataLength);

        }
        else
        {
            *pOutBuff = NULL;
            *pOutLength = 0;
        }

        rbusMessage_Release(msg);
        rbusMessage_Release(response);
    }
    else
    {
        size_t sz = 0, i = 0;
        rtPrivateClientInfo* pPrivCliInfo = (rtPrivateClientInfo*)pInBuff;

        directServerLock();
        sz = rtVector_Size(gListOfServerDirectDMLs);
        if(sz > 0)
        {
            for(i = 0; i < sz; ++i)
            {
                pSubObj = rtVector_At(gListOfServerDirectDMLs, i);

                if (0 == strncmp(pSubObj->m_consumerName, pPrivCliInfo->clientTopic, MAX_OBJECT_NAME_LENGTH))
                {
                    memcpy(&pSubObj->m_consumerInfo, pPrivCliInfo, sizeof(rtPrivateClientInfo));
                }
            }
        }
        directServerUnlock();
    }

    return RT_OK;
}

rbusCoreError_t rbuscore_startPrivateListener(const char* pPrivateConnAddress, const char* pConsumerName, const char *pDMLName, rbus_callback_t handler, void * user_data)
{
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rbusServerDMLList_t *obj = NULL;
    int err = 0;
    pthread_t pid;
    rtPrivateClientInfo  privConsInfo;

    if (pDMLName && pPrivateConnAddress && handler)
    {
        directServerLock();
        obj = rtVector_Find(gListOfServerDirectDMLs, pPrivateConnAddress, _findPrivateServer);
        if (!obj)
        {
            rbusServerDirectHandler_t *pInstance = rt_malloc(sizeof(rbusServerDirectHandler_t));
            strcpy(pInstance->m_privConnAddress, pPrivateConnAddress);
            pInstance->m_fnRouteCallback = _onDirectMessage;

            if((err = pthread_create(&pid, NULL, rbuscore_PrivateThreadFunc, pInstance)) != 0)
            {
                RBUSCORELOG_ERROR("pthread_create failed: err=%d", err);
                directServerUnlock();
                return RBUSCORE_ERROR_GENERAL;
            }
        }
        else
        {
            RBUSCORELOG_DEBUG("Already we have private session for this consumer(%s)", pPrivateConnAddress);
            pid = obj->m_pid;
            memcpy(&privConsInfo, &obj->m_consumerInfo, sizeof(rtPrivateClientInfo));
        }

        // Update the DMLs
        rbusServerDMLList_t *pTemp = rt_malloc(sizeof(rbusServerDMLList_t));
        strcpy(pTemp->m_privConnAddress, pPrivateConnAddress);
        strcpy(pTemp->m_consumerName, pConsumerName);
        strcpy(pTemp->m_privateDML, pDMLName);
        memcpy(&pTemp->m_consumerInfo, &privConsInfo, sizeof(rtPrivateClientInfo));
        pTemp->m_pid = pid;
        pTemp->m_fnCallbackHandler = handler;
        pTemp->m_fnCallbackuserData = user_data;
        rtVector_PushBack(gListOfServerDirectDMLs, pTemp);

        /* write to cache */
        _rbuscore_directconnection_save_to_cache();
        directServerUnlock();
    }
    else
        ret = RBUSCORE_ERROR_INVALID_PARAM;

    return ret;
}



/////// CLIENT SIDE ///////
static int _findClientPrivateConnection(const void* left, const void* right)
{
    return strncmp(((rbusClientDMLList_t*)left)->m_providerName, (char*)right, MAX_OBJECT_NAME_LENGTH);
}

static int _findClientPrivateDML(const void* left, const void* right)
{
    return strncmp(((rbusClientDMLList_t*)left)->m_privateDML, (char*)right, MAX_OBJECT_NAME_LENGTH);
}

static void _rbuscore_destroy_clientPrivate_connections()
{
    rbusClientDMLList_t *obj = NULL;
    char* pTmpProvider = NULL;

    directClientLock();

    while(rtVector_Size(gListOfClientDirectDMLs) > 0)
    {
        obj = rtVector_At(gListOfClientDirectDMLs, 0);
        rtConnection_Destroy(obj->m_privConn);
        pTmpProvider = strdup(obj->m_providerName);
        rtVector_RemoveItem(gListOfClientDirectDMLs, obj, rtVector_Cleanup_Free);

        while(rtVector_Size(gListOfClientDirectDMLs) > 0)
        {
            rtVector_RemoveItemByCompare(gListOfClientDirectDMLs, pTmpProvider, _findClientPrivateConnection, rtVector_Cleanup_Free);
        }

        free(pTmpProvider);
    }

    directClientUnlock();
}

rtConnection rbuscore_FindClientPrivateConnection(const char *pParameterName)
{
    rbusClientDMLList_t* pDmlObj = NULL;
    rtConnection myConn = NULL;

    if (pParameterName)
    {
        directClientLock();
        pDmlObj = rtVector_Find(gListOfClientDirectDMLs, pParameterName, _findClientPrivateDML);
        if (pDmlObj)
        {
            RBUSCORELOG_DEBUG("Already we have private session for this DML(%s) for this consumer", pParameterName);
            myConn = pDmlObj->m_privConn;
        }
        directClientUnlock();
    }
    return myConn;
}

#if 0
static void rbuscore_private_connection_advisory_callback(rtMessageHeader const* hdr, uint8_t const* data, uint32_t dataLen, void* closure)
{
    rtMessage msg;
    (void)hdr;
    (void)closure;
    int32_t advisory_event;

    RBUSCORELOG_ERROR("got event from advisory msg");
    rtMessage_FromBytes(&msg, data, dataLen);
    if(rtMessage_GetInt32(msg, RTMSG_ADVISE_EVENT, &advisory_event) == RT_OK)
    {
        if(advisory_event == rtAdviseClientDisconnect)
        {
            const char* listener;
            if(rtMessage_GetString(msg, RTMSG_ADVISE_INBOX, &listener) == RT_OK)
            {
                RBUSCORELOG_DEBUG("Advisory event: client disconnect %s", listener);
                rbuscore_terminatePrivateConnection(listener);
            }
            else
            {
                RBUSCORELOG_ERROR("Failed to get inbox from advisory msg");
            }
        }
    }
    else
    {
        RBUSCORELOG_ERROR("Failed to get event from advisory msg");
    }

    rtMessage_Release(msg);

    return;
}
#endif

rbusCoreError_t rbuscore_openPrivateConnectionToProvider(rtConnection *pPrivateConn, const char* pParameterName, const char *pPrivateConnAddress, const char *pProviderName)
{
    rtError       err;
    rbusCoreError_t ret = RBUSCORE_SUCCESS;
    rtMessage     config;
    rtConnection  connection;
    rbusClientDMLList_t *obj = NULL;

    if (pPrivateConn && pPrivateConnAddress)
    {
        directClientLock();
        obj = rtVector_Find(gListOfClientDirectDMLs, pProviderName, _findClientPrivateConnection);
        if (!obj)
        {
            RBUSCORELOG_INFO("Connection does not exist; create new");

            rtMessage_Create(&config);
            rtMessage_SetString(config, "appname", "rbus");
            rtMessage_SetString(config, "uri", pPrivateConnAddress);
            rtMessage_SetInt32(config, "max_retries", 5);
            rtMessage_SetInt32(config, "start_router", 0);

            err = rtConnection_CreateWithConfig(&connection, config);
            if (err != RT_OK)
            {
                RBUSCORELOG_ERROR("failed to create connection to router %s. %s", pPrivateConnAddress, rtStrError(err));
                rtMessage_Release(config);
                directClientUnlock();
                return RBUSCORE_ERROR_GENERAL;
            }
            *pPrivateConn = connection;

            rtConnection_AddDefaultListener(connection, master_event_callback, NULL);
            RBUSCORELOG_DEBUG("pPrivateConn new = %p", connection);
            rtMessage_Release(config);
        }
        else
        {
            *pPrivateConn = connection = obj->m_privConn;
            RBUSCORELOG_DEBUG("pPrivateConn found = %p", obj->m_privConn);
        }

        /* Add an entry to the list */
        {
            rbusClientDMLList_t *pNewObj = NULL;
            /* Update the Vector to avoid multiple connections */
            pNewObj = rt_malloc(sizeof(rbusClientDMLList_t));

            strcpy(pNewObj->m_privateDML, pParameterName);
            strcpy(pNewObj->m_providerName, pProviderName);
            pNewObj->m_privConn = connection;

            rtVector_PushBack(gListOfClientDirectDMLs, pNewObj);
    
        }
        directClientUnlock();
    }
    else
        ret = RBUSCORE_ERROR_INVALID_PARAM;

    return ret;
}

rbusCoreError_t rbuscore_createPrivateConnection(const char *pParameterName, rtConnection *pPrivateConn)
{
    rbusCoreError_t err;
    rbusMessage request, response;
    rbusMessage_Init(&request);
    rbusMessage_SetString(request, rtConnection_GetReturnAddress(g_connection));
    rbusMessage_SetInt32(request, (int32_t)getpid());
    rbusMessage_SetString(request,  pParameterName);
    rbusMessage_SetString(request,  g_daemon_address);

    err = rbus_invokeRemoteMethod(pParameterName, METHOD_OPENDIRECT_CONN, request, 5000, &response);
    if(RBUSCORE_SUCCESS == err)
    {
        rbusMessage_GetInt32(response, (int32_t*)&err);
        RBUSCORELOG_DEBUG("Response from the remote method is [%d]!", err);

        if (err == RBUSCORE_SUCCESS)
        {
            RBUSCORELOG_DEBUG("Received valid response!");
            const char* pDaemonAddress = NULL;
            const char* pProviderName = NULL;
            rbusMessage_GetString(response, &pProviderName);
            rbusMessage_GetString(response, &pDaemonAddress);

            err = rbuscore_openPrivateConnectionToProvider(pPrivateConn, pParameterName, pDaemonAddress, pProviderName);
        }
        rbusMessage_Release(response);
    }
    return err;
}

rbusCoreError_t rbuscore_closePrivateConnection(const char *pParameterName)
{
    rbusCoreError_t err;
    rbusClientDMLList_t *obj = NULL;
    rtConnection  connection = NULL;
    rbusMessage request, response;
    char providerName[MAX_OBJECT_NAME_LENGTH+1] = "";

    if (pParameterName)
    {
        directClientLock();
        obj = rtVector_Find(gListOfClientDirectDMLs, pParameterName, _findClientPrivateDML);
        if (!obj)
        {
            RBUSCORELOG_DEBUG("Private Connection Does Not Exist anymore for (%s)", pParameterName);
            directClientUnlock();
            /* Possibly the Provider Exited and we removed it as part of Advisory message */
            return RBUSCORE_ERROR_GENERAL;
        }

        /* You are here only becoz you have valid connection */
        rbusMessage_Init(&request);
        rbusMessage_SetString(request, rtConnection_GetReturnAddress(g_connection));
        rbusMessage_SetString(request, pParameterName);

        err = rbus_invokeRemoteMethod(pParameterName, METHOD_CLOSEDIRECT_CONN, request, 5000, &response);
        if(RBUSCORE_SUCCESS != err)
        {
            RBUSCORELOG_ERROR("Received error %d from RBUS Daemon for the object (%s)", err, pParameterName);
        }
        else
        {
            rbusMessage_GetInt32(response, (int32_t*)&err);
            RBUSCORELOG_DEBUG("Response from the remote method is [%d]!", err);

            if (RBUSCORE_SUCCESS == err)
            {
                connection = obj->m_privConn;
                memcpy(providerName, obj->m_providerName, MAX_OBJECT_NAME_LENGTH);
                providerName[MAX_OBJECT_NAME_LENGTH] = '\0';
                rtVector_RemoveItem(gListOfClientDirectDMLs, obj, rtVector_Cleanup_Free);
                obj = NULL;
            }
            rbusMessage_Release(response);
        }

        obj = rtVector_Find(gListOfClientDirectDMLs, providerName, _findClientPrivateConnection);
        if (!obj)
        {
            RBUSCORELOG_DEBUG("No more DML for this provider, so lets destroy the connection.");
            rtConnection_Destroy(connection);
        }
        directClientUnlock();
    }
    else
        err = RBUSCORE_ERROR_INVALID_PARAM;

    return err;
}

rbusCoreError_t rbuscore_terminatePrivateConnection(const char *pProviderName)
{
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusClientDMLList_t *obj = NULL;
    rtConnection  connection = NULL;

    directClientLock();

    obj = rtVector_Find(gListOfClientDirectDMLs, pProviderName, _findClientPrivateConnection);
    if (obj)
    {
        connection = obj->m_privConn;
        rtConnection_Destroy(connection);
        rtVector_RemoveItem(gListOfClientDirectDMLs, obj, rtVector_Cleanup_Free);
        while ((obj = rtVector_Find(gListOfClientDirectDMLs, pProviderName, _findClientPrivateConnection)) != NULL)
            rtVector_RemoveItem(gListOfClientDirectDMLs, obj, rtVector_Cleanup_Free);
    }
    directClientUnlock();

    return err;
}



/* End of File */
