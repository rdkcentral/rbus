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
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <rtFuture.h>
#include <rtRetainable.h>
#include <rtVector.h>
#include <rtMemory.h>
#include <rbuscore.h>
#include <rbus_session_mgr.h>
#include <rbus.h>
#include "rbus_buffer.h"
#include "rbus_element.h"
#include "rbus_valuechange.h"
#include "rbus_subscriptions.h"
#include "rbus_asyncsubscribe.h"
#include "rbus_intervalsubscription.h"
#include "rbus_config.h"
#include "rbus_log.h"
#include "rbus_handle.h"
#include "rbus_message.h"

//******************************* MACROS *****************************************//
#define RBUS_MIN(a,b) ((a)<(b) ? (a) : (b))

#define RTINT_TO_PTR(n) ((void *)(intptr_t) (n))
#define RTPTR_TO_INT(p) ((int)(intptr_t) (p))

#ifndef FALSE
#define FALSE                               0
#endif
#ifndef TRUE
#define TRUE                                1
#endif
#define VERIFY_NULL(T)          if(NULL == T){ RBUSLOG_WARN(#T" is NULL"); return RBUS_ERROR_INVALID_INPUT; }
#define VERIFY_ZERO(T)          if(0 == T){ RBUSLOG_WARN(#T" is 0"); return RBUS_ERROR_INVALID_INPUT; }

#define rbusCheckNotNull(T)          if(NULL == T){ RBUSLOG_WARN(#T" is NULL"); return RBUS_ERROR_INVALID_INPUT; }
#define rbusCheckAsyncInvalid(H)   assert(H->useEventLoop == false);

#define LockMutex() pthread_mutex_lock(&gMutex)
#define UnlockMutex() pthread_mutex_unlock(&gMutex)
//********************************************************************************//

//******************************* STRUCTURES *************************************//
struct _rbusMethodAsyncHandle
{
    rtMessageHeader hdr;
};

struct rbusAsyncRequest {
  rtRetainable                  retainable;
  int32_t                       timeout_millis;
  rbusAsyncResponseHandler_t    completion_handler;
  void*                         user_data;
  rbusProperty_t                props;
  rbusHandle_t                  rbus;
  rtAsyncRequestId              rt_request_id;

  // fields for methods
  char*                         method_name;
  rbusMethodAsyncRespHandler_t  method_handler;
  rbusObject_t                  method_params;

  bool                          completed;
};

struct rbusAsyncResponse {
  rbusProperty_t                props;
  rbusError_t                   status;
  void*                         user_data;
  const char*                   error_message;
};

struct rbusEventLoopCallbackContext {
  rbusMessage                 res;
  rbusCoreError_t             status;
  struct rbusAsyncRequest*    req;
};

static void rbusEventLoop_EnqueueResponse(
  rbusHandle_t                rbus,
  rbusMessage                 res,
  rbusCoreError_t             status,
  struct rbusAsyncRequest*    req,
  void                      (*exec)(void *));

typedef enum _rbus_legacy_support
{
    RBUS_LEGACY_STRING = 0,    /**< Null terminated string                                           */
    RBUS_LEGACY_INT,           /**< Integer (2147483647 or -2147483648) as String                    */
    RBUS_LEGACY_UNSIGNEDINT,   /**< Unsigned Integer (ex: 4,294,967,295) as String                   */
    RBUS_LEGACY_BOOLEAN,       /**< Boolean as String (ex:"true", "false"                            */
    RBUS_LEGACY_DATETIME,      /**< ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ) as String                 */
    RBUS_LEGACY_BASE64,        /**< Base64 representation of data as String                          */
    RBUS_LEGACY_LONG,          /**< Long (ex: 9223372036854775807 or -9223372036854775808) as String */
    RBUS_LEGACY_UNSIGNEDLONG,  /**< Unsigned Long (ex: 18446744073709551615) as String               */
    RBUS_LEGACY_FLOAT,         /**< Float (ex: 1.2E-38 or 3.4E+38) as String                         */
    RBUS_LEGACY_DOUBLE,        /**< Double (ex: 2.3E-308 or 1.7E+308) as String                      */
    RBUS_LEGACY_BYTE,
    RBUS_LEGACY_NONE
} rbusLegacyDataType_t;

typedef enum _rbus_legacy_returns {
    RBUS_LEGACY_ERR_SUCCESS = 100,
    RBUS_LEGACY_ERR_MEMORY_ALLOC_FAIL = 101,
    RBUS_LEGACY_ERR_FAILURE = 102,
    RBUS_LEGACY_ERR_NOT_CONNECT = 190,
    RBUS_LEGACY_ERR_TIMEOUT = 191,
    RBUS_LEGACY_ERR_NOT_EXIST = 192,
    RBUS_LEGACY_ERR_NOT_SUPPORT = 193,
    RBUS_LEGACY_ERR_RESOURCE_EXCEEDED = 9004,
    RBUS_LEGACY_ERR_INVALID_PARAMETER_NAME = 9005,
    RBUS_LEGACY_ERR_INVALID_PARAMETER_TYPE = 9006,
    RBUS_LEGACY_ERR_INVALID_PARAMETER_VALUE = 9007,
    RBUS_LEGACY_ERR_NOT_WRITABLE = 9008,
} rbusLegacyReturn_t;

//********************************************************************************//

//******************************* GLOBALS *****************************************//
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

//********************************************************************************//

//******************************* INTERNAL FUNCTIONS *****************************//
static void rbusMessage_GetStatusCodes(rbusMessage m, rbusError_t* rbus_status, rbusLegacyReturn_t* legacy_status);
static rbusError_t rbusCoreTranslateStatusFromResponse(rbusMessage m, rbusCoreError_t core_status);


static rbusError_t rbusCoreError_to_rbusError(rtError e)
{
  rbusError_t err;
  switch (e)
  {
    case RBUSCORE_SUCCESS:
      err = RBUS_ERROR_SUCCESS;
      break;
    case RBUSCORE_ERROR_GENERAL:
      err = RBUS_ERROR_BUS_ERROR;
      break;
    case RBUSCORE_ERROR_INVALID_PARAM:
      err = RBUS_ERROR_INVALID_INPUT;
      break;
    case RBUSCORE_ERROR_INVALID_STATE:
      err = RBUS_ERROR_SESSION_ALREADY_EXIST;
      break;
    case RBUSCORE_ERROR_INSUFFICIENT_MEMORY:
      err = RBUS_ERROR_OUT_OF_RESOURCES;
      break;
    case RBUSCORE_ERROR_REMOTE_END_DECLINED_TO_RESPOND:
      err = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
      break;
    case RBUSCORE_ERROR_REMOTE_END_FAILED_TO_RESPOND:
      err = RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
      break;
    case RBUSCORE_ERROR_REMOTE_TIMED_OUT:
      err = RBUS_ERROR_TIMEOUT;
      break;
    case RBUSCORE_ERROR_MALFORMED_RESPONSE:
      err = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
      break;
    case RBUSCORE_ERROR_UNSUPPORTED_METHOD:
      err = RBUS_ERROR_INVALID_METHOD;
      break;
    case RBUSCORE_ERROR_UNSUPPORTED_EVENT:
      err = RBUS_ERROR_INVALID_EVENT;
      break;
    case RBUSCORE_ERROR_OUT_OF_RESOURCES:
      err = RBUS_ERROR_OUT_OF_RESOURCES;
      break;
    case RBUSCORE_ERROR_DESTINATION_UNREACHABLE:
      err = RBUS_ERROR_DESTINATION_NOT_REACHABLE;
      break;
    case RBUSCORE_SUCCESS_ASYNC:
      err = RBUS_ERROR_ASYNC_RESPONSE;
      break;
    case RBUSCORE_ERROR_SUBSCRIBE_NOT_HANDLED:
      err = RBUS_ERROR_INVALID_OPERATION;
      break;
    default:
      err = RBUS_ERROR_BUS_ERROR;
      break;
  }
  return err;
}

static rbusError_t CCSPError_to_rbusError(rtError e)
{
  rbusError_t err;
  switch (e)
  {
    case RBUS_LEGACY_ERR_SUCCESS:
      err = RBUS_ERROR_SUCCESS;
      break;
    case RBUS_LEGACY_ERR_MEMORY_ALLOC_FAIL:
      err = RBUS_ERROR_OUT_OF_RESOURCES;
      break;
    case RBUS_LEGACY_ERR_FAILURE:
      err = RBUS_ERROR_BUS_ERROR;
      break;
    case RBUS_LEGACY_ERR_NOT_CONNECT:
      err = RBUS_ERROR_OUT_OF_RESOURCES;
      break;
    case RBUS_LEGACY_ERR_TIMEOUT:
      err = RBUS_ERROR_TIMEOUT;
      break;
    case RBUS_LEGACY_ERR_NOT_EXIST:
      err = RBUS_ERROR_DESTINATION_NOT_FOUND;
      break;
    case RBUS_LEGACY_ERR_NOT_SUPPORT:
      err = RBUS_ERROR_BUS_ERROR;
      break;
    case RBUS_LEGACY_ERR_RESOURCE_EXCEEDED:
      err = RBUS_ERROR_OUT_OF_RESOURCES;
      break;
    case RBUS_LEGACY_ERR_INVALID_PARAMETER_NAME:
      err = RBUS_ERROR_INVALID_INPUT;
      break;
    case RBUS_LEGACY_ERR_INVALID_PARAMETER_TYPE:
      err = RBUS_ERROR_INVALID_INPUT;
      break;
    case RBUS_LEGACY_ERR_INVALID_PARAMETER_VALUE:
     err = RBUS_ERROR_INVALID_INPUT;
     break;
    case RBUS_LEGACY_ERR_NOT_WRITABLE:
     err = RBUS_ERROR_INVALID_OPERATION;
     break;
    default:
      err = RBUS_ERROR_BUS_ERROR;
      break;
  }
  return err;
}

void rbusEventSubscription_free(void* p)
{
    rbusEventSubscription_t* sub = (rbusEventSubscription_t*)p;
    free((void*)sub->eventName);
    if(sub->filter)
    {
        rbusFilter_Release(sub->filter);
    }
    free(sub);
}

static rbusEventSubscription_t* rbusEventSubscription_find(rtVector eventSubs, char const* eventName,
        rbusFilter_t filter, uint32_t interval, uint32_t duration)
{
    /*FIXME - convert to map */
    size_t i;
    for(i=0; i < rtVector_Size(eventSubs); ++i)
    {
        rbusEventSubscription_t* sub = (rbusEventSubscription_t*)rtVector_At(eventSubs, i);
        if(sub && !strcmp(sub->eventName, eventName) && !rbusFilter_Compare(sub->filter, filter)
                && (sub->interval == interval) && (sub->duration == duration))
        {
            return sub;
        }
    }
    return NULL;
}
static bool _parse_rbusData_to_value (char const* pBuff, rbusLegacyDataType_t legacyType, rbusValue_t value)
{
    bool rc = false;
    if (pBuff && value)
    {
        switch (legacyType)
        {
            case RBUS_LEGACY_STRING:
            {
                rc = rbusValue_SetFromString(value, RBUS_STRING, pBuff);
                break;
            }
            case RBUS_LEGACY_INT:
            {
                rc = rbusValue_SetFromString(value, RBUS_INT32, pBuff);
                break;
            }
            case RBUS_LEGACY_UNSIGNEDINT:
            {
                rc = rbusValue_SetFromString(value, RBUS_UINT32, pBuff);
                break;
            }
            case RBUS_LEGACY_BOOLEAN:
            {
                rc = rbusValue_SetFromString(value, RBUS_BOOLEAN, pBuff);
                break;
            }
            case RBUS_LEGACY_LONG:
            {
                rc = rbusValue_SetFromString(value, RBUS_INT64, pBuff);
                break;
            }
            case RBUS_LEGACY_UNSIGNEDLONG:
            {
                rc = rbusValue_SetFromString(value, RBUS_UINT64, pBuff);
                break;
            }
            case RBUS_LEGACY_FLOAT:
            {
                rc = rbusValue_SetFromString(value, RBUS_SINGLE, pBuff);
                break;
            }
            case RBUS_LEGACY_DOUBLE:
            {
                 rc = rbusValue_SetFromString(value, RBUS_DOUBLE, pBuff);
                break;
            }
            case RBUS_LEGACY_BYTE:
            {
                rbusValue_SetBytes(value, (uint8_t*)pBuff, strlen(pBuff));
                rc = true;
                break;
            }
            case RBUS_LEGACY_DATETIME:
            {
                rc = rbusValue_SetFromString(value, RBUS_DATETIME, pBuff);
                break;
            }
            case RBUS_LEGACY_BASE64:
            {
                RBUSLOG_WARN("RBUS_LEGACY_BASE64_TYPE: Base64 type was never used in CCSP so far. So, Rbus did not support it till now. Since this is the first Base64 query, please report to get it fixed.");
                rbusValue_SetString(value, pBuff);
                rc = true;
                break;
            }
            default:
                break;
        }
    }
    return rc;
}

//*************************** SERIALIZE/DERIALIZE FUNCTIONS ***************************//
#define DEBUG_SERIALIZER 0

rbusError_t rbusValue_initFromMessage(rbusValue_t* value, rbusMessage msg);
void rbusValue_appendToMessage(char const* name, rbusValue_t value, rbusMessage msg);
rbusError_t rbusProperty_initFromMessage(rbusProperty_t* property, rbusMessage msg);
void rbusPropertyList_initFromMessage(rbusProperty_t* prop, rbusMessage msg);
void rbusPropertyList_appendToMessage(rbusProperty_t prop, rbusMessage msg);
void rbusObject_initFromMessage(rbusObject_t* obj, rbusMessage msg);
void rbusObject_appendToMessage(rbusObject_t obj, rbusMessage msg);
void rbusEventData_updateFromMessage(rbusEvent_t* event, rbusFilter_t* filter, uint32_t* interval,
        uint32_t* duration, int32_t* componentId, rbusMessage msg);
void rbusEventData_appendToMessage(rbusEvent_t* event, rbusFilter_t filter, uint32_t interval,
        uint32_t duration, int32_t componentId, rbusMessage msg);
void rbusFilter_AppendToMessage(rbusFilter_t filter, rbusMessage msg);
void rbusFilter_InitFromMessage(rbusFilter_t* filter, rbusMessage msg);

rbusError_t rbusValue_initFromMessage(rbusValue_t* value, rbusMessage msg)
{
    uint8_t const* data;
    bool rc = true;
    uint32_t length;
    int type;
    char const* pBuffer = NULL;

    rbusValue_Init(value);

    rbusMessage_GetInt32(msg, (int*) &type);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> value pop type=%d", type);
#endif
    if(type>=RBUS_LEGACY_STRING && type<=RBUS_LEGACY_NONE)
    {
        rbusMessage_GetString(msg, &pBuffer);
        RBUSLOG_DEBUG("Received Param Value in string : [%s]", pBuffer);
        rc = _parse_rbusData_to_value (pBuffer, type, *value);
        if(!rc)
        {
            RBUSLOG_WARN("%s: RBUS_INVALID_INPUT", __FUNCTION__);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }
    else
    {
        if(type == RBUS_OBJECT)
        {
            rbusObject_t obj;
            rbusObject_initFromMessage(&obj, msg);
            rbusValue_SetObject(*value, obj);
            rbusObject_Release(obj);
        }
        else
        if(type == RBUS_PROPERTY)
        {
            rbusProperty_t prop;
            rbusPropertyList_initFromMessage(&prop, msg);
            rbusValue_SetProperty(*value, prop);
            rbusProperty_Release(prop);
        }
        else
        {
            int32_t ival;
            int64_t i64;
            double fval;

            switch(type)
            {
                case RBUS_INT16:
                    rbusMessage_GetInt32(msg, &ival);
                    rbusValue_SetInt16(*value, (int16_t)ival);
                    break;
                case RBUS_UINT16:
                    rbusMessage_GetInt32(msg, &ival);
                    rbusValue_SetUInt16(*value, (uint16_t)ival);
                    break;
                case RBUS_INT32:
                    rbusMessage_GetInt32(msg, &ival);
                    rbusValue_SetInt32(*value, (int32_t)ival);
                    break;
                case RBUS_UINT32:
                    rbusMessage_GetInt32(msg, &ival);
                    rbusValue_SetUInt32(*value, (uint32_t)ival);

                    break;
                case RBUS_INT64:
                {
                    rbusMessage_GetInt64(msg, &i64);
                    rbusValue_SetInt64(*value, (int64_t)i64);
                    break;
                }
                case RBUS_UINT64:
                {
                    rbusMessage_GetInt64(msg, &i64);
                    rbusValue_SetUInt64(*value, (uint64_t)i64);
                    break;
                }
                case RBUS_SINGLE:
                    rbusMessage_GetDouble(msg, &fval);
                    rbusValue_SetSingle(*value, (float)fval);
                    break;
                case RBUS_DOUBLE:
                    rbusMessage_GetDouble(msg, &fval);
                    rbusValue_SetDouble(*value, (double)fval);
                    break;
                case RBUS_DATETIME:
                    rbusMessage_GetBytes(msg, &data, &length);
                    rbusValue_SetTLV(*value, type, length, data);
                    break;
                default:
                    rbusMessage_GetBytes(msg, &data, &length);
                    rbusValue_SetTLV(*value, type, length, data);
                    break;
            }

#if DEBUG_SERIALIZER
            char* sv = rbusValue_ToString(*value,0,0);
            RBUSLOG_INFO("> value pop data=%s", sv);
            free(sv);
#endif
        }
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusProperty_initFromMessage(rbusProperty_t* property, rbusMessage msg)
{
    char const* name;
    rbusValue_t value;
    rbusError_t err = RBUS_ERROR_SUCCESS;

    rbusMessage_GetString(msg, (char const**) &name);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> prop pop name=%s", name);
#endif
    rbusProperty_Init(property, name, NULL);
    err= rbusValue_initFromMessage(&value, msg);
    rbusProperty_SetValue(*property, value);
    rbusValue_Release(value);
    return err;
}

void rbusPropertyList_appendToMessage(rbusProperty_t prop, rbusMessage msg)
{
    int numProps = 0;
    rbusProperty_t first = prop;
    while(prop)
    {
        numProps++;
        prop = rbusProperty_GetNext(prop);
    }
    rbusMessage_SetInt32(msg, numProps);/*property count*/
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> prop add numProps=%d", numProps);
#endif
    prop = first;
    while(prop)
    {
        rbusValue_appendToMessage(rbusProperty_GetName(prop), rbusProperty_GetValue(prop), msg);
        prop = rbusProperty_GetNext(prop);
    }
}

void rbusPropertyList_initFromMessage(rbusProperty_t* prop, rbusMessage msg)
{
    rbusProperty_t previous = NULL, first = NULL;
    int numProps = 0;
    rbusMessage_GetInt32(msg, (int*) &numProps);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> prop pop numProps=%d", numProps);
#endif
    while(--numProps >= 0)
    {
        rbusProperty_t prop;
        rbusProperty_initFromMessage(&prop, msg);
        if(first == NULL)
            first = prop;
        if(previous != NULL)
        {
            rbusProperty_SetNext(previous, prop);
            rbusProperty_Release(prop);
        }
        previous = prop;
    }
    /*TODO we need to release the props we inited*/
    *prop = first;
}

void rbusObject_appendToMessage(rbusObject_t obj, rbusMessage msg)
{
    int numChild = 0;
    rbusObject_t child;

    rbusMessage_SetString(msg, rbusObject_GetName(obj));/*object name*/
    rbusMessage_SetInt32(msg, rbusObject_GetType(obj));/*object type*/
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> object add name=%s type=%d", rbusObject_GetName(obj), rbusObject_GetType(obj));
#endif
    rbusPropertyList_appendToMessage(rbusObject_GetProperties(obj), msg);

    child = rbusObject_GetChildren(obj);
    numChild = 0;
    while(child)
    {
        numChild++;
        child = rbusObject_GetNext(child);
    }
    rbusMessage_SetInt32(msg, numChild);/*object child object count*/
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> object add numChild=%d", numChild);
#endif
    child = rbusObject_GetChildren(obj);
    while(child)
    {
        rbusObject_appendToMessage(child, msg);/*object child object*/
        child = rbusObject_GetNext(child);
    }
}

void rbusObject_initFromMessage(rbusObject_t* obj, rbusMessage msg)
{
    char const* name = NULL;
    int type = 0;
    int numChild = 0;
    rbusProperty_t prop;
    rbusObject_t children=NULL, previous=NULL;

    rbusMessage_GetString(msg, &name);
    rbusMessage_GetInt32(msg, &type);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> object pop name=%s type=%d", name, type);
#endif

    rbusPropertyList_initFromMessage(&prop, msg);

    rbusMessage_GetInt32(msg, &numChild);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> object pop numChild=%d", numChild);
#endif

    while(--numChild >= 0)
    {
        rbusObject_t next;
        rbusObject_initFromMessage(&next, msg);/*object child object*/
        if(children == NULL)
            children = next;
        if(previous != NULL)
        {
            rbusObject_SetNext(previous, next);
            rbusObject_Release(next);
        }
        previous = next;
    }

    if(type == RBUS_OBJECT_MULTI_INSTANCE)
        rbusObject_InitMultiInstance(obj, name);
    else
        rbusObject_Init(obj, name);

    rbusObject_SetProperties(*obj, prop);
    rbusProperty_Release(prop);
    rbusObject_SetChildren(*obj, children);
    rbusObject_Release(children);
}

void rbusValue_appendToMessage(char const* name, rbusValue_t value, rbusMessage msg)
{
    rbusValueType_t type = RBUS_NONE;

    rbusMessage_SetString(msg, name);

    if(value)
        type = rbusValue_GetType(value);
    rbusMessage_SetInt32(msg, type);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> value add name=%s type=%d", name, type);
#endif
    if(type == RBUS_OBJECT)
    {
        rbusObject_appendToMessage(rbusValue_GetObject(value), msg);
    }
    else if(type == RBUS_PROPERTY)
    {
        rbusPropertyList_appendToMessage(rbusValue_GetProperty(value), msg);
    }
    else
    {
        int64_t i64;
        switch(type)
        {
            case RBUS_INT16:
                rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetInt16(value));
                break;
            case RBUS_UINT16:
                rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetUInt16(value));
                break;
            case RBUS_INT32:
                rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetInt32(value));
                break;
            case RBUS_UINT32:
                rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetUInt32(value));
                break;
            case RBUS_INT64:
            {
                i64 = rbusValue_GetInt64(value);
                rbusMessage_SetInt64(msg, (int64_t)i64);
                break;
            }
            case RBUS_UINT64:
            {
                i64 = rbusValue_GetUInt64(value);
                rbusMessage_SetInt64(msg, (int64_t)i64);
                break;
            }
            case RBUS_SINGLE:
                rbusMessage_SetDouble(msg, (double)rbusValue_GetSingle(value));
                break;
            case RBUS_DOUBLE:
                rbusMessage_SetDouble(msg, (double)rbusValue_GetDouble(value));
                break;
            case RBUS_DATETIME:
                rbusMessage_SetBytes(msg, rbusValue_GetV(value), rbusValue_GetL(value));
                break;
            default:
            {
                uint8_t const* buff = NULL;
                uint32_t len = 0;
                if(value)
                {
                    buff = rbusValue_GetV(value);
                    len = rbusValue_GetL(value);
                }
                rbusMessage_SetBytes(msg, buff, len);
                break;
            }
        }
#if DEBUG_SERIALIZER
        char* sv = rbusValue_ToString(value,0,0);
        RBUSLOG_INFO("> value add data=%s", sv);
        free(sv);
#endif

    }
}

void rbusFilter_AppendToMessage(rbusFilter_t filter, rbusMessage msg)
{
    rbusMessage_SetInt32(msg, rbusFilter_GetType(filter));
    if(rbusFilter_GetType(filter) == RBUS_FILTER_EXPRESSION_RELATION)
    {
        rbusMessage_SetInt32(msg, rbusFilter_GetRelationOperator(filter));
        rbusValue_appendToMessage("filter", rbusFilter_GetRelationValue(filter), msg);
    }
    else if(rbusFilter_GetType(filter) == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusMessage_SetInt32(msg, rbusFilter_GetLogicOperator(filter));
        rbusFilter_AppendToMessage(rbusFilter_GetLogicLeft(filter), msg);
        if(rbusFilter_GetLogicOperator(filter) != RBUS_FILTER_OPERATOR_NOT)
            rbusFilter_AppendToMessage(rbusFilter_GetLogicRight(filter), msg);
    }
}

void rbusFilter_InitFromMessage(rbusFilter_t* filter, rbusMessage msg)
{
    int32_t type;
    int32_t op;

    rbusMessage_GetInt32(msg, &type);

    if(type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        char const* name;
        rbusValue_t val;
        rbusMessage_GetInt32(msg, &op);
        rbusMessage_GetString(msg, &name);
        rbusValue_initFromMessage(&val, msg);
        rbusFilter_InitRelation(filter, op, val);
        rbusValue_Release(val);
    }
    else if(type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusFilter_t left = NULL, right = NULL;
        rbusMessage_GetInt32(msg, &op);
        rbusFilter_InitFromMessage(&left, msg);
        if(op != RBUS_FILTER_OPERATOR_NOT)
            rbusFilter_InitFromMessage(&right, msg);
        rbusFilter_InitLogic(filter, op, left, right);
        rbusFilter_Release(left);
        if(right)
            rbusFilter_Release(right);
    }
}

void rbusEventData_updateFromMessage(rbusEvent_t* event, rbusFilter_t* filter,
        uint32_t* interval, uint32_t* duration, int32_t* componentId, rbusMessage msg)
{
    char const* name;
    int type;
    rbusObject_t data;
    int hasFilter = false;
    
    rbusMessage_GetString(msg, (char const**) &name);
    rbusMessage_GetInt32(msg, (int*) &type);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> event pop name=%s type=%d", name, type);
#endif

    rbusObject_initFromMessage(&data, msg);

    rbusMessage_GetInt32(msg, &hasFilter);
    if(hasFilter)
        rbusFilter_InitFromMessage(filter, msg);
    else
        *filter = NULL;

    event->name = name;
    event->type = type;
    event->data = data;
    rbusMessage_GetUInt32(msg, interval);
    rbusMessage_GetUInt32(msg, duration);
    rbusMessage_GetInt32(msg, componentId);
}

void rbusEventData_appendToMessage(rbusEvent_t* event, rbusFilter_t filter,
        uint32_t interval, uint32_t duration, int32_t componentId, rbusMessage msg)
{
    rbusMessage_SetString(msg, event->name);
    rbusMessage_SetInt32(msg, event->type);
#if DEBUG_SERIALIZER
    RBUSLOG_INFO("> event add name=%s type=%d", event->name, event->type);
#endif
    rbusObject_appendToMessage(event->data, msg);
    if(filter)
    {
        rbusMessage_SetInt32(msg, 1);
        rbusFilter_AppendToMessage(filter, msg);
    }
    else
    {
        rbusMessage_SetInt32(msg, 0);
    }

    rbusMessage_SetInt32(msg, interval);
    rbusMessage_SetInt32(msg, duration);
    rbusMessage_SetInt32(msg, componentId);
}

bool _is_valid_get_query(char const* name)
{
    /* 1. Find whether the query ends with `!` to find out Event is being queried */
    /* 2. Find whether the query ends with `()` to find out method is being queried */
    if (name != NULL)
    {
        int length = strlen (name);
        int temp = 0;
        temp = length - 1;
        if (('!' == name[temp]) ||
            (')' == name[temp]) ||
            (NULL != strstr (name, "(")))
        {
            RBUSLOG_DEBUG("Event or Method is Queried");
            return false;
        }
    }
    else
    {
        RBUSLOG_DEBUG("Null Pointer sent for Query");
        return false;
    }

    return true;

}

bool _is_wildcard_query(char const* name)
{
    if (name != NULL)
    {
        /* 1. Find whether the query ends with `.` to find out object level query */
        /* 2. Find whether the query has `*` to find out multiple items are being queried */
        int length = strlen (name);
        int temp = 0;
        temp = length - 1;

        if (('.' == name[temp]) || (NULL != strstr (name, "*")))
        {
            RBUSLOG_DEBUG("The Query is having wildcard.. ");
            return true;
        }
    }
    else
    {
        RBUSLOG_DEBUG("Null Pointer sent for Query");
        return true;
    }

    return false;
}

#if 0
char const* getLastTokenInName(char const* name)
{
    if(name == NULL)
        return NULL;

    int len = (int)strlen(name);

    if(len == 0)
        return name;

    len--;

    if(name[len] == '.')
        len--;

    while(len != 0 && name[len] != '.')
        len--;
        
    if(name[len] == '.')
        return &name[len+1];
    else
        return name;
}
#endif

/*  Recurse row elements Adding or Removing value change properties
 *  when adding a row, call this after subscriptions are added to the row element
 *  when removing a row, call this before subscriptions are removed from the row element
 */
void valueChangeTableRowUpdate(rbusHandle_t handle, elementNode* rowNode, bool added)
{
    if(rowNode)
    {
        elementNode* child = rowNode->child;

        while(child)
        {
            if(child->type == RBUS_ELEMENT_TYPE_PROPERTY)
            {
                /*avoid calling ValueChange if there's no subs on it*/
                if(elementHasAutoPubSubscriptions(child, NULL))
                {
                    if(added)
                    {
                        rbusValueChange_AddPropertyNode(handle, child);
                    }
                    else
                    {
                        rbusValueChange_RemovePropertyNode(handle, child);
                    }
                }
            }

            /*recurse into children that are not row templates*/
            if( child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0) )
            {
                valueChangeTableRowUpdate(handle, child, added);
            }

            child = child->nextSibling;
        }
    }
}

int subscribeHandlerImpl(
    rbusHandle_t handle,
    bool added,
    elementNode* el,
    char const* eventName,
    char const* listener,
    int32_t componentId,
    int32_t interval,
    int32_t duration,
    rbusFilter_t filter)
{
    int error = RBUS_ERROR_SUCCESS;
    rbusSubscription_t* subscription = NULL;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    /*autoPublish is an output parameter used to disable the default behaviour where rbus automatically publishing events for
    provider data elements. When providers set autoPublish to true the value will be checked once per second and the maximum event
    rate is one event per two seconds. If faster eventing or real-time eventing is required providers can set autoPublish to false
    and implement a custom approach. For fastest response time and to avoid missing changes that occur faster than once per second,
    the preferred way is to use a callback triggered from the lowest level of code to detect a value change. This callback may be
    invoked by vendor code via a HAL API or other method. This callback can be received by the component that provides this event
    and used to send the publish message in real time.*/
    bool autoPublish = true;

    if(!el)
        return -1;
    /* call the provider subHandler first to see if it overrides autoPublish */
    if(el->cbTable.eventSubHandler)
    {
        rbusError_t err;
        rbusEventSubAction_t action;
        if(added)
            action = RBUS_EVENT_ACTION_SUBSCRIBE;
        else
            action = RBUS_EVENT_ACTION_UNSUBSCRIBE;

        err = el->cbTable.eventSubHandler(handle, action, eventName, filter, interval, &autoPublish);

        if(err != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_DEBUG("%s provider subHandler return err=%d", __FUNCTION__, err);

            return err;
        }
    }

    if(added)
    {
        subscription = rbusSubscriptions_addSubscription(handleInfo->subscriptions, listener, eventName, componentId, filter, interval, duration, autoPublish, el);

        if(!subscription)
        {
            return RBUSCORE_ERROR_INVALID_STATE; /*unexpected*/
        }
    }
    else
    {
        subscription = rbusSubscriptions_getSubscription(handleInfo->subscriptions, listener, eventName, componentId, filter);

        if(!subscription)
        {
            RBUSLOG_INFO("unsubscribing from event which isn't currectly subscribed to event=%s listener=%s", eventName, listener);
            return RBUSCORE_ERROR_INVALID_PARAM; /*unsubscribing from event which isn't currectly subscribed to*/
        }
    }

    /* if autoPublish and its a property being subscribed to
       then update rbusValueChange to handle the property */
    if(el->type == RBUS_ELEMENT_TYPE_PROPERTY && subscription->autoPublish)
    {
        rtListItem item;
        rtList_GetFront(subscription->instances, &item);
        while(item)
        {
            elementNode* node;

            rtListItem_GetData(item, (void**)&node);

            if (subscription->interval)
            {
                RBUSLOG_INFO("%s: subscription with interval  %s event=%s prop=%s", __FUNCTION__,
                        added ? "Add" : "Remove", subscription->eventName, node->fullName);
                if(added) {
                    if((error = rbusInterval_AddSubscriptionRecord(handle, node, subscription)) != RBUS_ERROR_SUCCESS)
                        RBUSLOG_ERROR("rbusInterval_AddSubscriptionRecord failed with error : %d\n", error);
                    break;
                }
                else
                {
                    rbusInterval_RemoveSubscriptionRecord(handle, node, subscription);
                    break;
                }
            }
            else if(!elementHasAutoPubSubscriptions(node, subscription))
            {
                /* Check if the node has other subscribers or not.  If it has other
                   subs then we don't need to either add or remove it from ValueChange */
                RBUSLOG_INFO("%s: ValueChange %s event=%s prop=%s", __FUNCTION__,
                        added ? "Add" : "Remove", subscription->eventName, node->fullName);
                if(added)
                {
                    rbusValueChange_AddPropertyNode(handle, node);
                }
                else
                {
                    rbusValueChange_RemovePropertyNode(handle, node);
                }
            }

            rtListItem_GetNext(item, &item);
        }
    }

    /*remove subscription only after handling its ValueChange properties above*/
    if(!added)
    {
        rbusSubscriptions_removeSubscription(handleInfo->subscriptions, subscription);
    }
    return RBUSCORE_SUCCESS;
}

static void registerTableRow (rbusHandle_t handle, elementNode* tableInstElem, char const* tableName, char const* aliasName, uint32_t instNum)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    elementNode* rowElem;

    RBUSLOG_DEBUG("%s table [%s] alias [%s] instNum [%u]", __FUNCTION__, tableName, aliasName, instNum);

    rowElem = instantiateTableRow(tableInstElem, instNum, aliasName);

    rbusSubscriptions_onTableRowAdded(handleInfo->subscriptions, rowElem);

    /*update ValueChange after rbusSubscriptions_onTableRowAdded */
    valueChangeTableRowUpdate(handle, rowElem, true);

    /*send OBJECT_CREATED event after we create the row*/
    {
        rbusEvent_t event = {0};
        rbusError_t respub;
        rbusObject_t data;
        rbusValue_t instNumVal;
        rbusValue_t aliasVal;
        rbusValue_t rowNameVal;

        rbusValue_Init(&rowNameVal);
        rbusValue_Init(&instNumVal);
        rbusValue_Init(&aliasVal);

        rbusValue_SetString(rowNameVal, rowElem->fullName);
        rbusValue_SetUInt32(instNumVal, instNum);
        rbusValue_SetString(aliasVal, aliasName ? aliasName : "");

        rbusObject_Init(&data, NULL);
        rbusObject_SetValue(data, "rowName", rowNameVal);
        rbusObject_SetValue(data, "instNum", instNumVal);
        rbusObject_SetValue(data, "alias", aliasVal);

        event.name = tableName;
        event.type = RBUS_EVENT_OBJECT_CREATED;
        event.data = data;

        RBUSLOG_INFO("%s publishing ObjectCreated table=%s rowName=%s", __FUNCTION__, tableName, rowElem->fullName);
        respub = rbusEvent_Publish(handle, &event);

        if(respub != RBUS_ERROR_SUCCESS && respub != RBUS_ERROR_NOSUBSCRIBERS)
        {
            RBUSLOG_WARN("failed to publish ObjectCreated event err:%d", respub);
        }

        /* Re-subscribe all the child elements of this row */
        if(handleInfo->subscriptions)
            rbusSubscriptions_resubscribeRowElementCache(handle, handleInfo->subscriptions, rowElem);

        rbusValue_Release(rowNameVal);
        rbusValue_Release(instNumVal);
        rbusValue_Release(aliasVal);
        rbusObject_Release(data);
    }
}

static void unregisterTableRow (rbusHandle_t handle, elementNode* rowInstElem)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    char* rowInstName = strdup(rowInstElem->fullName); /*must dup because we are deleting the instance*/
    elementNode* tableInstElem = rowInstElem->parent;

    RBUSLOG_DEBUG("%s [%s]", __FUNCTION__, rowInstElem->fullName);

    /*update ValueChange before rbusSubscriptions_onTableRowRemoved */
    valueChangeTableRowUpdate(handle, rowInstElem, false);

    rbusSubscriptions_onTableRowRemoved(handleInfo->subscriptions, rowInstElem);

    deleteTableRow(rowInstElem);

    /*send OBJECT_DELETED event after we delete the row*/
    {
        rbusEvent_t event = {0};
        rbusError_t respub;
        rbusValue_t rowNameVal;
        rbusObject_t data;
        char tableName[RBUS_MAX_NAME_LENGTH];

        /*must end the table name with a dot(.)*/
        snprintf(tableName, RBUS_MAX_NAME_LENGTH, "%s.", tableInstElem->fullName);

        rbusValue_Init(&rowNameVal);
        rbusValue_SetString(rowNameVal, rowInstName);

        rbusObject_Init(&data, NULL);
        rbusObject_SetValue(data, "rowName", rowNameVal);

        event.name = tableName;
        event.data = data;
        event.type = RBUS_EVENT_OBJECT_DELETED;
        RBUSLOG_INFO("%s publishing ObjectDeleted table=%s rowName=%s", __FUNCTION__, tableInstElem->fullName, rowInstName);
        respub = rbusEvent_Publish(handle, &event);

        rbusValue_Release(rowNameVal);
        rbusObject_Release(data);
        free(rowInstName);

        if(respub != RBUS_ERROR_SUCCESS && respub != RBUS_ERROR_NOSUBSCRIBERS)
        {
            RBUSLOG_WARN("failed to publish ObjectDeleted event err:%d", respub);
        }
    }
}

//******************************* CALLBACKS *************************************//

struct rbusEventLoopEventSubscribeContext {
  rtFuture_t*               future;
  const char*               object;
  const char*               event_name;
  const char*               listener;
  int                       added;
  int                       component_id;
  int                       interval;
  int                       duration;
  rbusFilter_t              filter;
  void*                     user_data;
};

static int rbusEventSubscribeCallbackHandler(
  const char*               object,
  const char*               event_name,
  const char*               listener,
  int                       added,
  int                       component_id,
  int                       interval,
  int                       duration,
  rbusFilter_t              filter,
  void*                     user_data);

static void rbusEventSubscribeCallbackHandlerAdapter(void* argp) {
  struct rbusEventLoopEventSubscribeContext* ctx = (struct rbusEventLoopEventSubscribeContext *) argp;
  int ret = rbusEventSubscribeCallbackHandler(ctx->object, ctx->event_name, ctx->listener, ctx->added,
    ctx->component_id, ctx->interval, ctx->duration, ctx->filter, ctx->user_data);
  rtFuture_SetCompleted(ctx->future, RT_OK, RTINT_TO_PTR(ret));
}


static int _event_subscribe_callback_handler(
  char const*               object,
  char const*               eventName,
  char const*               listener,
  int                       added,
  int                       componentId,
  int                       interval,
  int                       duration,
  rbusFilter_t              filter,
  void*                     userData)
{
  int ret;
  rbusHandle_t rbus = (rbusHandle_t) userData;

  if (rbus->useEventLoop) {
    struct rbusEventLoopEventSubscribeContext ctx;
    ctx.future = rtFuture_Create();
    ctx.object = object;
    ctx.event_name = eventName;
    ctx.listener = listener;
    ctx.added = added;
    ctx.component_id = componentId;
    ctx.interval = interval;
    ctx.duration = duration;
    ctx.filter = filter;
    rbusFilter_Retain(ctx.filter);
    ctx.user_data = userData;

    rbusRunnable_t r;
    r.argp = &ctx;
    r.exec = &rbusEventSubscribeCallbackHandlerAdapter;
    r.cleanup = NULL;
    r.next = NULL;
    r.id = 0;

    rbusRunnableQueue_PushBack(&rbus->eventQueue, r);

    rtError err = rtFuture_Wait(ctx.future, RT_TIMEOUT_INFINITE);
    if (err == RT_OK) {
      ret = RTPTR_TO_INT( rtFuture_GetValue(ctx.future) );
    }
    else {
      RBUSLOG_WARN("error dispatching event subscription notification. %s", rtStrError(err));
    }
  }
  else {
    ret = rbusEventSubscribeCallbackHandler(object, eventName, listener, added, componentId,
      interval, duration, filter, userData);
  }

  return ret;
}

int rbusEventSubscribeCallbackHandler(
  const char*     RT_UNUSED(object),
  const char*               event_name,
  const char*               listener,
  int                       added,
  int                       component_id,
  int                       interval,
  int                       duration,
  rbusFilter_t              filter,
  void*                     user_data)
{
    rbusHandle_t rbus = (rbusHandle_t) user_data;

    rbusCoreError_t err = RBUSCORE_SUCCESS;

    RBUSLOG_DEBUG("%s: event subscribe callback for [%s] event!", __FUNCTION__, event_name);

    elementNode* el = retrieveInstanceElement(rbus->elementRoot, event_name);

    if(el)
    {
        RBUSLOG_DEBUG("%s: found element of type %d", __FUNCTION__, el->type);

        err = subscribeHandlerImpl(rbus, added, el, event_name, listener, component_id, interval,
          duration, filter);

        if(filter)
        {
            rbusFilter_Release(filter);
        }
    }
    else
    {
        RBUSLOG_WARN("event subscribe callback: unexpected! element not found");
        err = RBUSCORE_ERROR_UNSUPPORTED_EVENT;
    }
    return err;
}

static void _client_disconnect_callback_handler(const char * listener)
{
    LockMutex();
    rbusHandleList_ClientDisconnect(listener);
    UnlockMutex();
}

void _subscribe_async_callback_handler(rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    subscription->asyncHandler(subscription->handle, subscription, error);

    if(error == RBUS_ERROR_SUCCESS)
    {
        rtVector_PushBack(handleInfo->eventSubs, subscription);
    }
    else
    {
        rbusEventSubscription_free(subscription);
    }
}

int _event_callback_handler (char const* objectName, char const* eventName, rbusMessage message, void* userData)
{
    rbusEventSubscription_t* subscription = NULL;
    rbusEventHandler_t handler = NULL;
    rbusEvent_t event = {0};
    rbusFilter_t filter = NULL;
    int32_t componentId = 0;
    uint32_t interval = 0;
    uint32_t duration = 0;

    RBUSLOG_DEBUG("Received event callback: objectName=%s eventName=%s", 
        objectName, eventName);

    subscription = (rbusEventSubscription_t*)userData;

    if(!subscription || !subscription->handle || !subscription->handler)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    handler = (rbusEventHandler_t)subscription->handler;

    rbusEventData_updateFromMessage(&event, &filter, &interval, &duration, &componentId, message);
    
    (*handler)(subscription->handle, &event, subscription);

    rbusObject_Release(event.data);

    return 0;
}

static int _master_event_callback_handler(char const* sender, char const* eventName, rbusMessage message,
  void* RT_UNUSED(userData))
{
    rbusEvent_t event = {0};
    rbusFilter_t filter = NULL;
    int32_t componentId = -1;
    rbusEventSubscription_t* subscription = NULL;
    struct _rbusHandle* handleInfo = NULL;
    uint32_t interval = 0;
    uint32_t duration = 0;

    rbusEventData_updateFromMessage(&event, &filter, &interval, &duration, &componentId, message);

    LockMutex();
    handleInfo = rbusHandleList_GetByComponentID(componentId);
    UnlockMutex();

    if(!handleInfo)
    {
        RBUSLOG_INFO("Received master event callback with invalid componentId: sender=%s eventName=%s componentId=%d", sender, eventName, componentId);
        return RBUSCORE_ERROR_EVENT_NOT_HANDLED;
    }

    RBUSLOG_DEBUG("Received master event callback: sender=%s eventName=%s componentId=%d", sender, eventName, componentId);

    subscription = rbusEventSubscription_find(handleInfo->eventSubs, eventName, filter, interval, duration);

    if(subscription)
    {
        ((rbusEventHandler_t)subscription->handler)(subscription->handle, &event, subscription);
    }
    else
    {
        RBUSLOG_DEBUG("Received master event callback: sender=%s eventName=%s, but no subscription found", sender, event.name);
        return RBUSCORE_ERROR_EVENT_NOT_HANDLED;
    }

    rbusObject_Release(event.data);
    rbusFilter_Release(filter);

    return RBUSCORE_SUCCESS;
}

static void _set_callback_handler (rbusHandle_t handle, rbusMessage request, rbusMessage *response)
{
    rbusError_t rc = 0;
    int sessionId = 0;
    int numVals = 0;
    int loopCnt = 0;
    char* pCompName = NULL;
    char* pIsCommit = NULL;
    bool isCommit = false;
    char const* pFailedElement = NULL;
    rbusProperty_t* pProperties = NULL;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusSetHandlerOptions_t opts;

    memset(&opts, 0, sizeof(opts));

    rbusMessage_GetInt32(request, &sessionId);
    rbusMessage_GetString(request, (char const**) &pCompName);
    rbusMessage_GetInt32(request, &numVals);

    if(numVals > 0)
    {
        /* Update the Get Handler input options */
        opts.sessionId = sessionId;
        opts.requestingComponent = pCompName;

        elementNode* el = NULL;

        pProperties = (rbusProperty_t*)rt_try_malloc(numVals*sizeof(rbusProperty_t));
        if(pProperties)
        {
            for (loopCnt = 0; loopCnt < numVals; loopCnt++)
            {
                rc = rbusProperty_initFromMessage(&pProperties[loopCnt], request);
            }
            if(rc != RBUS_ERROR_SUCCESS)
                goto exit;
            rbusMessage_GetString(request, (char const**) &pIsCommit);

            /* Since we set as string, this needs to compared with string..
            * Otherwise, just the #define in the top for TRUE/FALSE should be used.
            * isCommit used to set only the last parameter in the list in order
            * to turn this into a bulk set operation: FIXME maybe we need a 'bulk' option
            */
            if (strncasecmp("TRUE", pIsCommit, 4) == 0)
                isCommit = true;

            for (loopCnt = 0; loopCnt < numVals; loopCnt++)
            {
                /* Retrive the element node */
                char const* paramName = rbusProperty_GetName(pProperties[loopCnt]);
                el = retrieveInstanceElement(handleInfo->elementRoot, paramName);
                if(el != NULL)
                {
                    if(el->cbTable.setHandler)
                    {
                        if(isCommit && loopCnt == numVals -1)
                            opts.commit = true;

                        rc = el->cbTable.setHandler(handle, pProperties[loopCnt], &opts);
                        if (rc != RBUS_ERROR_SUCCESS)
                        {
                            RBUSLOG_WARN("Set Failed for %s; Component Owner returned Error", paramName);
                            pFailedElement = paramName;
                            break;
                        }
                        else
                        {
                            setPropertyChangeComponent(el, pCompName);
                        }
                    }
                    else
                    {
                        RBUSLOG_WARN("Set Failed for %s; No Handler found", paramName);
                        rc = RBUS_ERROR_INVALID_OPERATION;
                        pFailedElement = paramName;
                        break;
                    }
                }
                else
                {
                    RBUSLOG_WARN("Set Failed for %s; No Element registered", paramName);
                    rc = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
                    pFailedElement = paramName;
                    break;
                }
            }
        }
        else
        {
            RBUSLOG_WARN("Set Failed: failed to malloc %d properties", numVals);
            rc = RBUS_ERROR_OUT_OF_RESOURCES;
            pFailedElement = pCompName;
        }
    }
    else
    {
        RBUSLOG_WARN("Set Failed as %s did not send any input", pCompName);
        rc = RBUS_ERROR_INVALID_INPUT;
        pFailedElement = pCompName;
    }

exit:
    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, (int) rc);
    if (pFailedElement)
        rbusMessage_SetString(*response, pFailedElement);

    if(pProperties)
    {
        for (loopCnt = 0; loopCnt < numVals; loopCnt++)
        {
            rbusProperty_Release(pProperties[loopCnt]);
        }
        free(pProperties);
    }

    return;
}

/*
    convert a registration element name to a instance name based on the instance numbers in the original 
    partial path query
    example:
    registration name like: Device.Services.VoiceService.{i}.X_BROADCOM_COM_Announcement.ServerAddress
    and query name like   : Device.Services.VoiceService.1.X_BROADCOM_COM_Announcement.
    final name should be  : Device.Services.VoiceService.1.X_BROADCOM_COM_Announcement.ServerAddress
 */
static char const* _convert_reg_name_to_instance_name(char const* registrationName, char const* query, char* buffer)
{
    int idx = 0;
    char const* preg = registrationName;
    char const* pinst= query;

    while(*preg != 0 && *pinst != 0)
    {
        while(*preg == *pinst)
        {
            buffer[idx++] = *preg;
            preg++;
            pinst++;
        }
        
        if(*preg == '{')
        {
            while(*pinst && *pinst != '.')
                buffer[idx++] = *pinst++;

            while(*preg && *preg != '.')
                preg++;
        }

        if(*pinst == 0) /*end of partial path but continue to write out the full name of child*/
        {
            while(*preg)
            {
                buffer[idx++] = *preg++;
            }
        }
        /*
         *---------------------------------------------------------------------------------------------------
         * In the Registered Name, whereever {i} is present, must be replaced with the number from the Query.
         * We should not be changing the registered name with additional info from queries.
         * Ex: When query received for the subtables,
         *      RegisteredName  : Device.Tables1.T1.{i}.T2
         *      Query           : Device.Tables1.T1.2.T2.
         *      Final should be : Device.Tables1.T1.2.T2
         * If we enable below code, it will change the final buffer as below which is wrong.
         *                        Device.Tables1.T1.2.T2.
         *---------------------------------------------------------------------------------------------------
         * In short, the input query is used only to find & replace {i} in the registered name.
         * So removed the below code
         *---------------------------------------------------------------------------------------------------
         */
#if 0
        else if(*preg == 0) /*end of query path but continue to write out the full name of child*/
        {
            while(*pinst)
            {
                buffer[idx++] = *pinst++;
            }
        }
#endif
    }
    buffer[idx++] = 0;

    RBUSLOG_DEBUG(" _convert_reg_name_to_instance_name");
    RBUSLOG_DEBUG("   reg: %s", registrationName);
    RBUSLOG_DEBUG(" query: %s", query);
    RBUSLOG_DEBUG(" final: %s", buffer);

    return buffer;
}

/*
    node can be either an instance node or a registration node (if an instance node doesn't exist).
    query will be set if node is a registration node, so that registration names can be converted to instance names
 */
static void _get_recursive_partialpath_handler(elementNode* node, char const* query, rbusHandle_t handle, const char* pRequestingComp, rbusProperty_t properties, int *pCount, int level)
{
    rbusGetHandlerOptions_t options;
    memset(&options, 0, sizeof(options));

    /* Update the Get Handler input options */
    options.requestingComponent = pRequestingComp;

    RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler node=%s type=%d query=%s", level*4, " ", node ? node->fullName : "NULL", node ? node->type : 0, query ? query : "NULL");

    if (node != NULL)
    {
        /*if table getHandler, then pass the query to it and stop recursion*/
        if(node->type == RBUS_ELEMENT_TYPE_TABLE && node->cbTable.getHandler)
        {
            rbusError_t result;
            rbusProperty_t tmpProperties;
            char instanceName[RBUS_MAX_NAME_LENGTH];
            char partialPath[RBUS_MAX_NAME_LENGTH];

            snprintf(partialPath, RBUS_MAX_NAME_LENGTH-1, "%s.", 
                     query ? _convert_reg_name_to_instance_name(node->fullName, query, instanceName) : node->fullName);

            RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler calling table getHandler partialPath=%s", level*4, " ", partialPath);

            rbusProperty_Init(&tmpProperties, partialPath, NULL);

            result = node->cbTable.getHandler(handle, tmpProperties, &options);

            if (result == RBUS_ERROR_SUCCESS )
            {
                int count = rbusProperty_Count(tmpProperties);

                RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler table getHandler returned %d properties", level*4, " ", count-1);

                /*the first property is just the partialPath we passed in */
                if(count > 1)
                {
                    /*take the second property, which is a list*/
                    rbusProperty_Append(properties, rbusProperty_GetNext(tmpProperties));
                    *pCount += count - 1;
                }
            }
            else
            {
                RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler table getHandler failed rc=%d", level*4, " ", result);
            }
            
            rbusProperty_Release(tmpProperties);
            return;
        }

        elementNode* child = node->child;

        while(child)
        {
            if(child->type != RBUS_ELEMENT_TYPE_TABLE && child->cbTable.getHandler)
            {
                rbusError_t result;
                char instanceName[RBUS_MAX_NAME_LENGTH];
                rbusProperty_t tmpProperties;

                RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler calling property getHandler node=%s", level*4, " ", child->fullName);

                rbusProperty_Init(&tmpProperties, query ? _convert_reg_name_to_instance_name(child->fullName, query, instanceName) : child->fullName, NULL);
                result = child->cbTable.getHandler(handle, tmpProperties, &options);
                if (result == RBUS_ERROR_SUCCESS)
                {
                    rbusProperty_Append(properties, tmpProperties);
                    *pCount += 1;
                }
                rbusProperty_Release(tmpProperties);
            }
            /*recurse into children that are not row templates without table getHandler*/
            else if(child->child && !(child->parent->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0 && child->cbTable.getHandler == NULL) )
            {
                RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler recurse into %s", level*4, " ", child->fullName);
                _get_recursive_partialpath_handler(child, query, handle, pRequestingComp, properties, pCount, level+1);
            }
            else
            {
                RBUSLOG_DEBUG("%*s_get_recursive_partialpath_handler skipping %s", level*4, " ", child->fullName);
            }

            child = child->nextSibling;
        }
    }
}

static rbusError_t _get_recursive_wildcard_handler (rbusHandle_t handle, char const *parameterName, const char* pRequestingComp, rbusProperty_t properties, int *pCount)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusError_t result = RBUS_ERROR_SUCCESS;
    elementNode* el = NULL;
    elementNode* child = NULL;

    char instanceName[RBUS_MAX_NAME_LENGTH];
    char wildcardName[RBUS_MAX_NAME_LENGTH];
    char* tmpPtr = NULL;

    /* Update the Get Handler input options */
    rbusGetHandlerOptions_t options;
    options.requestingComponent = pRequestingComp;

    /* Have the backup of given name */
    snprintf(instanceName, RBUS_MAX_NAME_LENGTH, "%s", parameterName);
    int length = strlen(instanceName) - 1;

    if ((instanceName[length] == '*') && (instanceName[length-1] == '.'))
    {
        instanceName[length] = '\0';
        length--;
    }

    /* Identify whether it is wildcard or partial path */
    tmpPtr = strstr(instanceName, "*");
    if (tmpPtr)
    {
        tmpPtr[0] = '\0';
        tmpPtr++;

        el = retrieveInstanceElement(handleInfo->elementRoot, instanceName);
        if (!el)
            return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;

        else if(el->type != RBUS_ELEMENT_TYPE_TABLE)
            return RBUS_ERROR_ACCESS_NOT_ALLOWED;

        child = el->child;
        while(child)
        {
            if(strcmp(child->name, "{i}") != 0)
            {
                snprintf (wildcardName, RBUS_MAX_NAME_LENGTH, "%s%s%s", instanceName, child->name, tmpPtr);
                result = _get_recursive_wildcard_handler(handle, wildcardName, pRequestingComp, properties, pCount);
                if (result != RBUS_ERROR_SUCCESS)
                {
                    RBUSLOG_WARN("Something went wrong while retriving the datamodel value...");
                    break;
                }
            }
            child = child->nextSibling;
        }
    }
    else if (instanceName[length] == '.')
    {
        int hasInstance = 1;
        el = retrieveInstanceElement(handleInfo->elementRoot, instanceName);
        if(el)
        {
            if(strstr(el->fullName, "{i}"))
                hasInstance = 0;
            _get_recursive_partialpath_handler(el, hasInstance ? NULL : parameterName, handle, pRequestingComp, properties, pCount, 0);
        }
        else
            result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    else
    {
        child = retrieveInstanceElement(handleInfo->elementRoot, instanceName);
        if (!child)
            return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;

        if(child->type != RBUS_ELEMENT_TYPE_TABLE && child->cbTable.getHandler)
        {
            rbusError_t result;
            rbusProperty_t tmpProperties;
            rbusProperty_Init(&tmpProperties, instanceName, NULL);
            result = child->cbTable.getHandler(handle, tmpProperties, &options);
            if (result == RBUS_ERROR_SUCCESS)
            {
                rbusProperty_Append(properties, tmpProperties);
                *pCount += 1;
            }
            rbusProperty_Release(tmpProperties);
            return result;
        }
    }
    return result;
}


static rbusError_t _get_single_dml_handler (rbusHandle_t handle, char const *parameterName, char const *pRequestingComp, rbusProperty_t properties)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    elementNode* el = NULL;
    rbusError_t result = RBUS_ERROR_SUCCESS;
    /* Update the Get Handler input options */
    rbusGetHandlerOptions_t options;
    options.requestingComponent = pRequestingComp;

    RBUSLOG_DEBUG("calling get single for [%s]", parameterName);

    el = retrieveInstanceElement(handleInfo->elementRoot, parameterName);
    if(el != NULL)
    {
        RBUSLOG_DEBUG("Retrieved [%s]", parameterName);

        if(el->cbTable.getHandler)
        {
            RBUSLOG_DEBUG("Table and CB exists for [%s], call the CB!", parameterName);

            result = el->cbTable.getHandler(handle, properties, &options);

            if (result != RBUS_ERROR_SUCCESS)
            {
                RBUSLOG_WARN("called CB with result [%d]", result);
            }
        }
        else
        {
            RBUSLOG_WARN("Element retrieved, but no cb installed for [%s]!", parameterName);
            result = RBUS_ERROR_INVALID_OPERATION;
        }
    }
    else
    {
        RBUSLOG_WARN("Not able to retrieve element [%s]. Element doesn't exist.", parameterName);
        result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    return result;
}

static void _get_callback_handler (rbusHandle_t handle, rbusMessage request, rbusMessage *response)
{
    int paramSize = 1, i = 0;
    rbusError_t result = RBUS_ERROR_SUCCESS;
    char const *parameterName = NULL;
    char const *pCompName = NULL;
    rbusProperty_t* properties = NULL;
    rbusGetHandlerOptions_t options;

    memset(&options, 0, sizeof(options));
    rbusMessage_GetString(request, &pCompName);
    rbusMessage_GetInt32(request, &paramSize);

    RBUSLOG_DEBUG("Param Size [%d]", paramSize);

    if(paramSize > 0)
    {
        /* Update the Get Handler input options */
        options.requestingComponent = pCompName;

        properties = rt_try_malloc(paramSize*sizeof(rbusProperty_t));
        if(properties)
        {
            for(i = 0; i < paramSize; i++)
            {
                rbusProperty_Init(&properties[i], NULL, NULL);
            }

            for(i = 0; i < paramSize; i++)
            {
                parameterName = NULL;
                rbusMessage_GetString(request, &parameterName);

                RBUSLOG_DEBUG("Param Name [%d]:[%s]", i, parameterName);

                rbusProperty_SetName(properties[i], parameterName);

                /* Check for wildcard query */
                if (_is_wildcard_query(parameterName))
                {
                    rbusProperty_t xproperties, first;
                    rbusValue_t xtmp;
                    int count = 0;
                            
                    rbusValue_Init(&xtmp);
                    rbusValue_SetString(xtmp, "tmpValue");
                    rbusProperty_Init(&xproperties, "tmpProp", xtmp);
                    rbusValue_Release(xtmp);
                    result = _get_recursive_wildcard_handler(handle, parameterName, pCompName, xproperties, &count);
                    rbusMessage_Init(response);
                    rbusMessage_SetInt32(*response, (int) result);
                    if (result == RBUS_ERROR_SUCCESS)
                    {
                        rbusMessage_SetInt32(*response, count);
                        if (count > 0)
                        {
                            first = rbusProperty_GetNext(xproperties);
                            for(i = 0; i < count; i++)
                            {
                                rbusValue_appendToMessage(rbusProperty_GetName(first), rbusProperty_GetValue(first), *response);
                                first = rbusProperty_GetNext(first);
                            }
                        }
                    }
                    /* Release the memory */
                    rbusProperty_Release(xproperties);
                    for (i = 0; i < paramSize; i++)
                    {
                        rbusProperty_Release(properties[i]);
                    }
                    free (properties);
                    return;
                }
                else
                {
                    //Do a look up and call the corresponding method
                    result = _get_single_dml_handler(handle, parameterName, pCompName, properties[i]);
                    if (result != RBUS_ERROR_SUCCESS)
                        break;
                }
            }
        }
        else
        {
            RBUSLOG_WARN("Failed to malloc %d properties", paramSize);
            result = RBUS_ERROR_OUT_OF_RESOURCES;
        }

        rbusMessage_Init(response);
        rbusMessage_SetInt32(*response, (int) result);
        if(properties)
        {
            if (result == RBUS_ERROR_SUCCESS)
            {
                rbusMessage_SetInt32(*response, paramSize);
                for(i = 0; i < paramSize; i++)
                {
                    rbusValue_appendToMessage(rbusProperty_GetName(properties[i]), rbusProperty_GetValue(properties[i]), *response);
                }
            }
        
            /* Free the memory, regardless of success or not.. */
            for (i = 0; i < paramSize; i++)
            {
                rbusProperty_Release(properties[i]);
            }
            free (properties);
        }
    }
    else
    {
        RBUSLOG_WARN("Get Failed as %s did not send any input", pCompName);
        result = RBUS_ERROR_INVALID_INPUT;
        rbusMessage_Init(response);
        rbusMessage_SetInt32(*response, (int) result);
    }

    return;
}

static void _get_parameter_names_recurse(elementNode* el, int* count, rbusMessage response, int requestedDepth, int currentDepth)
{
    int absDepth = abs(requestedDepth);

    if(requestedDepth >= 0 || absDepth == currentDepth)
    {
        if(count)
        {
            (*count)++;
        }
        else if(response)
        {
            uint32_t access = 0;
            char objectName[RBUS_MAX_NAME_LENGTH];

            if(el->cbTable.getHandler)
                access |= RBUS_ACCESS_GET;
            if(el->cbTable.setHandler)
                access |= RBUS_ACCESS_SET;
            if(el->cbTable.tableAddRowHandler)
                access |= RBUS_ACCESS_ADDROW;
            if(el->cbTable.tableRemoveRowHandler)
                access |= RBUS_ACCESS_REMOVEROW;
            if(el->cbTable.eventSubHandler)
                access |= RBUS_ACCESS_SUBSCRIBE;
            if(el->cbTable.methodHandler)
                access |= RBUS_ACCESS_INVOKE;

            /*rows must show read-write access to be ccsp compatible*/
            if(el->type == 0 && el->parent && el->parent->type == RBUS_ELEMENT_TYPE_TABLE)
            {
                access |= RBUS_ACCESS_GET | RBUS_ACCESS_SET;
            }

            /*objects must end with a dot to be ccsp compatible*/
            if(el->type == 0 || el->type == RBUS_ELEMENT_TYPE_TABLE)
            {
                snprintf(objectName, RBUS_MAX_NAME_LENGTH, "%s.", el->fullName);
                rbusMessage_SetString(response, objectName);
            }
            else
            {
                rbusMessage_SetString(response, el->fullName);
            }
            rbusMessage_SetInt32(response, el->type);
            rbusMessage_SetInt32(response, access);
        }
    }

    if(currentDepth < absDepth)
    {
        elementNode* child = el->child;
        while(child)
        {
            if( !(child->type == RBUS_ELEMENT_TYPE_TABLE && child->cbTable.getHandler) && /*TODO table with get handler */
                !(el->type == RBUS_ELEMENT_TYPE_TABLE && strcmp(child->name, "{i}") == 0))/*if not a table row template*/
            {
                _get_parameter_names_recurse(child, count, response, requestedDepth, currentDepth+1);
            }
            child = child->nextSibling;
        }
    }
}

static void _get_parameter_names_handler (rbusHandle_t handle, rbusMessage request, rbusMessage *response)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    char const *objName = NULL;
    int32_t requestedDepth = 0;
    int32_t getRowNamesOnly = 0;
    //int32_t isCcsp = 0;
    elementNode* el = NULL;
    
    rbusMessage_GetString(request, &objName);
    rbusMessage_GetInt32(request, &requestedDepth);
    if(rbusMessage_GetInt32(request, &getRowNamesOnly) != RT_OK)/*set to 1 by rbusTable_GetRowNames and 0 by rbus_getNames, but unset by ccsp*/
    {
        //isCcsp = 1;
        getRowNamesOnly = 0;
    }

    RBUSLOG_DEBUG("%s: object=%s depth=%d, rbusFlag=%d", __FUNCTION__, objName, requestedDepth, getRowNamesOnly);

    rbusMessage_Init(response);

    el = retrieveInstanceElement(handleInfo->elementRoot, objName);
    if (!el)
    {
        rbusMessage_SetInt32(*response, (int)RBUS_ERROR_ELEMENT_DOES_NOT_EXIST);
        return;
    }

    if(getRowNamesOnly)
    {
        elementNode* child = el->child;
        int numRows = 0;

        if(el->type != RBUS_ELEMENT_TYPE_TABLE)
        {
            rbusMessage_SetInt32(*response, (int)RBUS_ERROR_INVALID_INPUT);
            return;
        }

        while(child)
        {
            if(strcmp(child->name, "{i}") != 0)/*if not a table row template*/
            {
                numRows++;
            }
            child = child->nextSibling;
        }

        rbusMessage_SetInt32(*response, RBUS_ERROR_SUCCESS);
        rbusMessage_SetInt32(*response, (int)numRows);

        child = el->child;
        while(child)
        {
            if(strcmp(child->name, "{i}") != 0)
            {
                rbusMessage_SetInt32(*response, atoi(child->name));
                rbusMessage_SetString(*response, child->alias ? child->alias : "");
            }
            child = child->nextSibling;
        }

        return;
    }

    if( !(el->type == RBUS_ELEMENT_TYPE_TABLE && el->cbTable.getHandler) )
    {
        int count = 0;

        rbusMessage_SetInt32(*response, RBUS_ERROR_SUCCESS);

        _get_parameter_names_recurse(el, &count, NULL, requestedDepth, 0);

        RBUSLOG_DEBUG("%s found %d elements", __FUNCTION__, count);

        rbusMessage_SetInt32(*response, (int)count);

        _get_parameter_names_recurse(el, NULL, *response, requestedDepth, 0);
    }
    #if 0 //TODO-finish
    else /*if table with getHandler*/
    {
        rbusProperty_t props;
        rbusGetHandlerOptions_t options;
        memset(&options, 0, sizeof(options));
        options.requestingComponent = handleInfo->componentName; /*METHOD_GETPARAMETERNAMES doesn't include component name, so we just use ourselves*/

        RBUSLOG_DEBUG("%s calling table getHandler %s", __FUNCTION__, el->fullName);

        rbusProperty_Init(&props, el->fullName, NULL);

        /*there's no getNames handler so for table level getHandler which will return all properties as name/value pairs,
        we have to parse the values and get the parameter names for only the next level*/
        result = el->cbTable.getHandler(handle, props, &options);

        if (result == RBUS_ERROR_SUCCESS )
        {
            size_t objNameLen = strlen(objName);
            rbusProperty_t prop = rbusProperty_GetNext(props);/*2nd property is the actual list, 1st is the partialPath name*/
            while(prop)
            {
                char const* name = rbusProperty_GetName(prop);
                char propertyName[RBUS_MAX_NAME_LENGTH];
                char const* begName = name + objNameLen + 1;
                char const* endName = strchr(begName, '.');
                if(endName == NULL)
                {
                    strcpy(propertyName, name);
                }
                else
                {
                    size_t len = endName - name;
                    strncpy(propertyName, name, len);
                    propertyName[len] = 0;
                }
                if(recurse 
                || !rtList_HasItem(namesList, propertyName, rtList_Compare_String))
                    /*If recurse then we have to check that we only add the rows and not anything inside the rows.
                      Since there might be several properties in each row retured from the getHandler, we have to check
                      that we only add a single row item per row*/                   
                {
                    RBUSLOG_DEBUG("%s adding property %s", __FUNCTION__, propertyName);
                    rtList_PushBack(namesList, strdup(propertyName), NULL);
                }
                prop = rbusProperty_GetNext(prop);
            }
        }
        else
        {
            RBUSLOG_DEBUG("%s %s table getHandler failed rc=%d", __FUNCTION__, objName, result);
        }
        rbusProperty_Release(props);
    }
    #endif
}

static void _table_add_row_callback_handler (rbusHandle_t handle, rbusMessage request, rbusMessage* response)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusError_t result = RBUS_ERROR_BUS_ERROR;
    int sessionId;
    char const* tableName;
    char const* aliasName = NULL;
    int err;
    uint32_t instNum = 0;

    rbusMessage_GetInt32(request, &sessionId);
    rbusMessage_GetString(request, &tableName);
    err = rbusMessage_GetString(request, &aliasName); /*this presumes rbus_updateTable sent the alias.  
                                                 if CCSP/dmcli is calling us, then this will be NULL*/
    if(err != RT_OK || (aliasName && strlen(aliasName)==0))
        aliasName = NULL;

    RBUSLOG_DEBUG("%s table [%s] alias [%s] name [%s]", __FUNCTION__, tableName, aliasName, handleInfo->componentName);

    elementNode* tableRegElem = retrieveElement(handleInfo->elementRoot, tableName);
    elementNode* tableInstElem = retrieveInstanceElement(handleInfo->elementRoot, tableName);

    if(tableRegElem && tableInstElem)
    {
        if(tableRegElem->cbTable.tableAddRowHandler)
        {
            RBUSLOG_INFO("%s calling tableAddRowHandler table [%s] alias [%s]", __FUNCTION__, tableName, aliasName);

            result = tableRegElem->cbTable.tableAddRowHandler(handle, tableName, aliasName, &instNum);

            if (result == RBUS_ERROR_SUCCESS)
            {
                registerTableRow(handle, tableInstElem, tableName, aliasName, instNum);
            }
            else
            {
                RBUSLOG_WARN("%s tableAddRowHandler failed table [%s] alias [%s]", __FUNCTION__, tableName, aliasName);
            }
        }
        else
        {
            RBUSLOG_WARN("%s tableAddRowHandler not registered table [%s] alias [%s]", __FUNCTION__, tableName, aliasName);
            result = RBUS_ERROR_INVALID_OPERATION;
        }
    }
    else
    {
        RBUSLOG_WARN("%s no element found table [%s] alias [%s]", __FUNCTION__, tableName, aliasName);
        result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, result);
    rbusMessage_SetInt32(*response, (int32_t)instNum);
}

static void _table_remove_row_callback_handler (rbusHandle_t handle, rbusMessage request, rbusMessage* response)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusError_t result = RBUS_ERROR_BUS_ERROR;
    int sessionId;
    char const* rowName;

    rbusMessage_GetInt32(request, &sessionId);
    rbusMessage_GetString(request, &rowName);

    RBUSLOG_DEBUG("%s row [%s]", __FUNCTION__, rowName);

    /*get the element for the row */
    elementNode* rowRegElem = retrieveElement(handleInfo->elementRoot, rowName);
    elementNode* rowInstElem = retrieveInstanceElement(handleInfo->elementRoot, rowName);

    if(rowRegElem && rowInstElem)
    {
        /*switch to the row's table */
        elementNode* tableRegElem = rowRegElem->parent;
        elementNode* tableInstElem = rowInstElem->parent;

        if(tableRegElem && tableInstElem)
        {
            if(tableRegElem->cbTable.tableRemoveRowHandler)
            {
                RBUSLOG_INFO("%s calling tableRemoveRowHandler row [%s]", __FUNCTION__, rowName);

                result = tableRegElem->cbTable.tableRemoveRowHandler(handle, rowName);

                if (result == RBUS_ERROR_SUCCESS)
                {
                    unregisterTableRow(handle, rowInstElem);
                }
                else
                {
                    RBUSLOG_WARN("%s tableRemoveRowHandler failed row [%s]", __FUNCTION__, rowName);
                }
            }
            else
            {
                RBUSLOG_INFO("%s tableRemoveRowHandler not registered row [%s]", __FUNCTION__, rowName);
                result = RBUS_ERROR_INVALID_OPERATION;
            }
        }
        else
        {
            RBUSLOG_WARN("%s no parent element found row [%s]", __FUNCTION__, rowName);
            result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }
    }
    else
    {
        RBUSLOG_WARN("%s no element found row [%s]", __FUNCTION__, rowName);
        result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    rbusMessage_Init(response);
    rbusMessage_SetInt32(*response, result);
}

static int _method_callback_handler(rbusHandle_t handle, rbusMessage request, rbusMessage* response, const rtMessageHeader* hdr)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusError_t result = RBUS_ERROR_BUS_ERROR;
    int sessionId;
    int hasInputParameters = 0;
    char const* methodName;
    rbusObject_t inParams, outParams;
    rbusValue_t value1, value2;
    rbusValue_t outParamsVal = NULL;

    rbusValue_Init(&value1);
    rbusValue_Init(&value2);

    rbusMessage_GetInt32(request, &sessionId);
    rbusMessage_GetString(request, &methodName);

    // next flag indicates whether the method has any arguments
    rbusMessage_GetInt32(request, &hasInputParameters);
    if (hasInputParameters)
      rbusObject_initFromMessage(&inParams, request);
    else
      inParams = NULL;

    RBUSLOG_INFO("%s method [%s]", __FUNCTION__, methodName);

    rbusObject_Init(&outParams, NULL);
    /*get the element for the row */
    elementNode* methRegElem = retrieveElement(handleInfo->elementRoot, methodName);
    elementNode* methInstElem = retrieveInstanceElement(handleInfo->elementRoot, methodName);

    if(methRegElem && methInstElem)
    {
        if(methRegElem->cbTable.methodHandler)
        {
            RBUSLOG_INFO("%s calling methodHandler method [%s]", __FUNCTION__, methodName);

            rbusMethodAsyncHandle_t asyncHandle = rt_malloc(sizeof(struct _rbusMethodAsyncHandle));
            asyncHandle->hdr = *hdr;

            result = methRegElem->cbTable.methodHandler(handle, methodName, inParams, outParams, asyncHandle);
            
            if (result == RBUS_ERROR_ASYNC_RESPONSE)
            {
                /*outParams will be sent async*/
                RBUSLOG_INFO("%s async method in progress [%s]", __FUNCTION__, methodName);
            }
            else
            {
                if (result != RBUS_ERROR_SUCCESS)
                {
                    outParamsVal = rbusObject_GetValue(outParams, "error_code");
                    if(!outParamsVal)
                    {
                        rbusValue_SetInt32(value1, result);
                        rbusValue_SetString(value2, rbusError_ToString(result));
                        rbusObject_SetValue(outParams, "error_code", value1);
                        rbusObject_SetValue(outParams, "error_string", value2);
                    }
                }
                free(asyncHandle);
            }

        }
        else
        {
            RBUSLOG_INFO("%s methodHandler not registered method [%s]", __FUNCTION__, methodName);
            result = RBUS_ERROR_INVALID_OPERATION;
            rbusValue_SetInt32(value1, RBUS_ERROR_INVALID_OPERATION);
            rbusValue_SetString(value2, "RBUS_ERROR_INVALID_OPERATION");
            rbusObject_SetValue(outParams, "error_code", value1);
            rbusObject_SetValue(outParams, "error_string", value2);
        }
    }
    else
    {
        RBUSLOG_WARN("%s no element found method [%s]", __FUNCTION__, methodName);
        result = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        rbusValue_SetInt32(value1, RBUS_ERROR_ELEMENT_DOES_NOT_EXIST);
        rbusValue_SetString(value2, "RBUS_ERROR_ELEMENT_DOES_NOT_EXIST");
        rbusObject_SetValue(outParams, "error_code", value1);
        rbusObject_SetValue(outParams, "error_string", value2);
    }

    rbusObject_Release(inParams);
    rbusValue_Release(value1);
    rbusValue_Release(value2);

    if(result == RBUS_ERROR_ASYNC_RESPONSE)
    {
        rbusObject_Release(outParams);
        return RBUSCORE_SUCCESS_ASYNC;
    }
    else
    {
        rbusMessage_Init(response);
        rbusMessage_SetInt32(*response, result);
        rbusObject_appendToMessage(outParams, *response);
        rbusObject_Release(outParams);
        return RBUSCORE_SUCCESS; 
    }
}

struct rbusEventLoopCallContext {
  rtFuture_t*               future;
  const char*               destination;
  const char*               method;
  rbusMessage               request;
  void*                     user_data;
  rbusMessage*              response;
  const rtMessageHeader*    header;
};

static int rbusCallbackHandler(
  rbusHandle_t              rbus,
  char const*               destination,
  char const*               method,
  rbusMessage               request,
  rbusMessage*              response,
  const rtMessageHeader*    header);

static void rbusCallbackHandlerAdapter(void *argp) {
  struct rbusEventLoopCallContext* ctx = (struct rbusEventLoopCallContext *) argp;
  int ret = rbusCallbackHandler((rbusHandle_t) ctx->user_data, ctx->destination, ctx->method, ctx->request,
    ctx->response, ctx->header);
  rtFuture_SetCompleted(ctx->future, RT_OK, RTINT_TO_PTR(ret));
}

static void _subscribe_callback_handler (rbusHandle_t handle, rbusMessage request, rbusMessage* response, char const* method)
{
    const char * sender = NULL;
    const char * event_name = NULL;
    int has_payload = 0;
    rbusMessage payload = NULL;
    int publishOnSubscribe = 0;
    struct _rbusHandle* handleInfo = handle;
    int32_t componentId = 0;
    int32_t interval = 0;
    int32_t duration = 0;
    rbusFilter_t filter = NULL;

    rbusMessage_Init(response);

    if((RT_OK == rbusMessage_GetString(request, &event_name)) &&
        (RT_OK == rbusMessage_GetString(request, &sender)))
    {
        /*Extract arguments*/
        if((NULL == sender) || (NULL == event_name))
        {
            RBUSLOG_ERROR("Malformed subscription request. Sender: %s. Event: %s.", sender, event_name);
            rbusMessage_SetInt32(*response, RBUSCORE_ERROR_INVALID_PARAM);
        }
        else
        {
            rbusMessage_GetInt32(request, &has_payload);
            if(has_payload)
                rbusMessage_GetMessage(request, &payload);
            if(payload)
            {
                int hasFilter = 0;
                rbusMessage_GetInt32(payload, &componentId);
                rbusMessage_GetInt32(payload, &interval);
                rbusMessage_GetInt32(payload, &duration);
                rbusMessage_GetInt32(payload, &hasFilter);
                if(hasFilter)
                {
                    rbusFilter_InitFromMessage(&filter, payload);
                }
            }
            else
            {
                RBUSLOG_ERROR("%s: payload missing in subscribe request for event %s from %s", __FUNCTION__, event_name, sender);
            }

            int added = strncmp(method, METHOD_SUBSCRIBE, MAX_METHOD_NAME_LENGTH) == 0 ? 1 : 0;
            if(added)
                rbusMessage_GetInt32(request, &publishOnSubscribe);
            rbusCoreError_t ret = _event_subscribe_callback_handler(NULL, event_name, sender, added, componentId, interval, duration, filter, handle);
            rbusMessage_SetInt32(*response, ret);

            if(publishOnSubscribe)
            {
                elementNode* el = NULL;
                rbusEvent_t event = {0};
                rbusObject_t data = NULL;
                rbusProperty_t tmpProperties = NULL;
                rbusError_t err = RBUS_ERROR_SUCCESS;

                rbusObject_Init(&data, NULL);
                el = retrieveInstanceElement(handleInfo->elementRoot, event_name);
                if(el->type == RBUS_ELEMENT_TYPE_TABLE)
                {
                    rbusMessage tableRequest = NULL;
                    rbusMessage tableResponse = NULL;
                    int tableRet = 0;
                    int count = 0;
                    int i = 0;

                    rbusMessage_Init(&tableRequest);
                    rbusMessage_SetString(tableRequest, event_name);
                    rbusMessage_SetInt32(tableRequest, -1);
                    rbusMessage_SetInt32(tableRequest, 1);
                    _get_parameter_names_handler(handle, tableRequest, &tableResponse);
                    rbusMessage_GetInt32(tableResponse, &tableRet);
                    rbusMessage_GetInt32(tableResponse, &count);
                    rbusProperty_Init(&tmpProperties, "numberOfEntries", NULL);
                    if(count != 0)
                    {
                        for(i = 0; i < count; ++i)
                        {
                            int32_t instNum = 0;
                            char fullName[RBUS_MAX_NAME_LENGTH] = {0};
                            char row_instance[RBUS_MAX_NAME_LENGTH] = {0};
                            char const* alias = NULL;

                            rbusMessage_GetInt32(tableResponse, &instNum);
                            rbusMessage_GetString(tableResponse, &alias);
                            snprintf(fullName, RBUS_MAX_NAME_LENGTH, "%s%d.", event_name, instNum);
                            if(i == 0)
                            {
                                rbusProperty_SetInt32(tmpProperties, count);
                            }
                            snprintf(row_instance, RBUS_MAX_NAME_LENGTH, "path%d", instNum);
                            rbusProperty_AppendString(tmpProperties, row_instance, fullName);
                        }
                        rbusMessage_Release(tableRequest);
                        rbusMessage_Release(tableResponse);
                    }
                    else
                    {
                        rbusProperty_SetInt32(tmpProperties, count);
                    }
                    rbusObject_SetProperty(data, tmpProperties);
                }
                else
                {
                    if(ret == RBUSCORE_SUCCESS && el->cbTable.getHandler)
                    {
                        rbusValue_t val = NULL;
                        rbusGetHandlerOptions_t options;
                        memset(&options, 0, sizeof(options));

                        options.requestingComponent = handleInfo->componentName;
                        rbusProperty_Init(&tmpProperties, event_name, NULL);
                        err = el->cbTable.getHandler(handle, tmpProperties, &options);
                        val = rbusProperty_GetValue(tmpProperties);
                        rbusObject_SetValue(data, "initialValue", val);
                    }
                    else
                    {
                        err = RBUS_ERROR_INVALID_OPERATION;
                        rbusMessage_SetInt32(*response, 0); /* No initial value returned, as get handler is not present */
                        RBUSLOG_WARN("%s: Get handler does not exist %s", __FUNCTION__, event_name);
                    }
                }
                if (err == RBUS_ERROR_SUCCESS)
                {
                    event.name = event_name;
                    event.type = RBUS_EVENT_INITIAL_VALUE;
                    event.data = data;
                    rbusMessage_SetInt32(*response, 1); /* Based on this value initial value will be published to the consumer in
                                                           rbusEvent_SubscribeWithRetries() function call */
                    rbusEventData_appendToMessage(&event, filter, interval, duration, handleInfo->componentId, *response);
                    if(filter)
                    {
                        rbusFilter_Release(filter);
                    }

                    rbusProperty_Release(tmpProperties);
                    rbusObject_Release(data);
                }
            }
            if(payload)
                rbusMessage_Release(payload);
        }
    }
}

static int _callback_handler(
  char const*               destination,
  char const*               method,
  rbusMessage               request,
  void*                     userData,
  rbusMessage*              response,
  const rtMessageHeader*    header)
{
  int ret = 0;
  rbusHandle_t rbus = (rbusHandle_t) userData;
  if (rbus->useEventLoop) {
    struct rbusEventLoopCallContext ctx;
    ctx.future = rtFuture_Create();
    ctx.destination = destination;
    ctx.method = method;
    ctx.request = request;
    ctx.user_data = userData;
    ctx.response = response;
    ctx.header = header;

    rbusRunnable_t r;
    r.argp = &ctx;
    r.exec = &rbusCallbackHandlerAdapter;
    r.cleanup = NULL;
    r.next = NULL;
    r.id = 0;

    rbusRunnableQueue_PushBack(&rbus->eventQueue, r);

    // wait for eventloop to actually process the above request
    // once it's completed, we can resume
    rtError err = rtFuture_Wait(ctx.future, 10000); // TODO: where does timeout come from?
    if (err == RT_OK) {
      ret = RTPTR_TO_INT( rtFuture_GetValue(ctx.future) );
    }

    rtFuture_Destroy(ctx.future, NULL);
    ret = 0;
  }
  else {
    ret = rbusCallbackHandler(rbus, destination, method, request, response, header);
  }

  return ret;
}

int rbusCallbackHandler(
  rbusHandle_t              rbus,
  char const*     RT_UNUSED(destination),
  char const*               method,
  rbusMessage               request,
  rbusMessage*              response,
  const rtMessageHeader*    header)
{
  if (!strcmp(method, METHOD_GETPARAMETERVALUES))
    _get_callback_handler(rbus, request, response);
  else if (!strcmp(method, METHOD_SETPARAMETERVALUES))
    _set_callback_handler(rbus, request, response);
  else if (!strcmp(method, METHOD_GETPARAMETERNAMES))
    _get_parameter_names_handler(rbus, request, response);
  else if (!strcmp(method, METHOD_ADDTBLROW))
    _table_add_row_callback_handler(rbus, request, response);
  else if (!strcmp(method, METHOD_DELETETBLROW))
    _table_remove_row_callback_handler(rbus, request, response);
  else if (!strcmp(method, METHOD_RPC))
    return _method_callback_handler(rbus, request, response, header);
  else if (!strcmp(method, METHOD_SUBSCRIBE) || !strcmp(method, METHOD_UNSUBSCRIBE))
    _subscribe_callback_handler(rbus, request, response, method);

  else {
    RBUSLOG_ERROR("unhandled callback for [%s] method!", method);
  }
  return 0;
}

/*
    Handle once per process initialization or deinitialization needed by rbus_open
 */
static void _rbus_open_pre_initialize(bool retain)
{
    RBUSLOG_DEBUG("%s", __FUNCTION__);
    static bool sRetained = false;
    
    if(retain && !sRetained)
    {
        rbusConfig_CreateOnce();
        rbus_registerMasterEventHandler(_master_event_callback_handler, NULL);
        sRetained = true;
    }
    else if(!retain && sRetained)
    {
        rbusConfig_Destroy();
        rbusElement_mutex_destroy();
        sRetained = false;
    }
}

__attribute__((destructor))
static void _rbus_mutex_destructor()
{
    pthread_mutex_destroy(&gMutex);
}

//******************************* Bus Initialization *****************************//

void rbusOptions_Init(rbusOptions_t* popts)
{
    rbusOptions_t opts = rt_calloc(1, sizeof(struct _rbusOptions));
    opts->use_event_loop = false;
    opts->component_name = NULL;
    if(popts)
        *popts = opts;
}

void rbusOptions_EnableEventLoop(rbusOptions_t opts, bool val)
{
    opts->use_event_loop = val;
}

void rbusOptions_SetName(rbusOptions_t opts, char const* name)
{
    opts->component_name = strdup(name);
}

void rbusOptions_Release(rbusOptions_t opts)
{
    if(opts->component_name)
    {
        free((void*)opts->component_name);
        opts->component_name = NULL;
    }
    free(opts);
}

rbusError_t rbus_open(rbusHandle_t* handle, char const* componentName)
{
  rbusError_t ret = RBUS_ERROR_SUCCESS;
  rbusOptions_t opts;
  rbusOptions_Init(&opts);
  rbusOptions_EnableEventLoop(opts, false);
  rbusOptions_SetName(opts, componentName);
  ret = rbusHandle_Open(handle, opts);
  rbusOptions_Release(opts);
  return ret;
}

rbusError_t rbusHandle_Open(rbusHandle_t* handle, rbusOptions_t const opts)
{
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusHandle_t tmpHandle = NULL;
    static int32_t sLastComponentId = 0;

    if(!handle || !opts->component_name)
    {
        if(!handle)
            RBUSLOG_WARN("%s(%s): handle is NULL", __FUNCTION__, opts->component_name);
        if(!opts->component_name)
            RBUSLOG_WARN("%s: componentName is NULL", __FUNCTION__);
        ret = RBUS_ERROR_INVALID_INPUT;
        goto exit_error0;
    }

    RBUSLOG_INFO("%s(%s)", __FUNCTION__, opts->component_name);

    LockMutex();

    _rbus_open_pre_initialize(true);

    /*
        Per spec: If a component calls this API more than once, any previous busHandle 
        and all previous data element registrations will be canceled.
    */
    tmpHandle = rbusHandleList_GetByName(opts->component_name);

    if(tmpHandle)
    {
        UnlockMutex();
        RBUSLOG_WARN("%s(%s): closing previously opened component with the same name", __FUNCTION__,
          opts->component_name);
        rbus_close(tmpHandle);
        LockMutex();
    }

    if(rbusHandleList_IsFull())
    {
        RBUSLOG_ERROR("%s(%s): at maximum handle count %d", __FUNCTION__, opts->component_name, RBUS_MAX_HANDLES);
        ret = RBUS_ERROR_OUT_OF_RESOURCES;
        goto exit_error1;
    }

    /*
        Per spec: the first component that calls rbus_open will establishes a new socket connection to the bus broker.
        The connection might already be open due to a previous rbus_open or ccsp msg bus init.
    */
    if(!rbus_getConnection())
    {
        RBUSLOG_DEBUG("%s(%s): opening broker connection", __FUNCTION__, opts->component_name);

        if((err = rbus_openBrokerConnection(opts->component_name)) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s(%s): rbus_openBrokerConnection error %d", __FUNCTION__, opts->component_name, err);
            goto exit_error1;
        }
    }

    tmpHandle = rt_calloc(1, sizeof(struct _rbusHandle));

    if((err = rbus_registerObj(opts->component_name, _callback_handler, tmpHandle)) != RBUSCORE_SUCCESS)
    {
        /*This will fail if the same name was previously registered (by another rbus_open or ccsp msg bus init)*/
        RBUSLOG_ERROR("%s(%s): rbus_registerObj error %d", __FUNCTION__, opts->component_name, err);
        goto exit_error2;
    }

    tmpHandle->busyWaitSleepDurationMillis = 1;
    tmpHandle->componentId = ++sLastComponentId;
    tmpHandle->connection = rbus_getConnection();
    tmpHandle->componentName = strdup(opts->component_name);
    tmpHandle->useEventLoop = opts->use_event_loop;
    rtVector_Create(&tmpHandle->eventSubs);
    rtVector_Create(&tmpHandle->messageCallbacks);
    rbusValueChange_Init(&tmpHandle->valueChangeDetector);
    tmpHandle->reentrancyGuard = 0;

    if (opts->use_event_loop)
      rbusRunnableQueue_Init(&tmpHandle->eventQueue);

    *handle = tmpHandle;

    rbusHandleList_Add(tmpHandle);

    UnlockMutex();

    RBUSLOG_INFO("%s(%s) success", __FUNCTION__, opts->component_name);

    return RBUS_ERROR_SUCCESS;

exit_error2:

    if(rbus_getConnection() && rbusHandleList_IsEmpty())
        if((err = rbus_unregisterClientDisconnectHandler()) != RBUSCORE_SUCCESS)
            RBUSLOG_ERROR("%s(%s): rbus_unregisterClientDisconnectHandler error %d", __FUNCTION__, opts->component_name, err);

    if(rbus_getConnection() && rbusHandleList_IsEmpty())
        if((err = rbus_closeBrokerConnection()) != RBUSCORE_SUCCESS)
            RBUSLOG_ERROR("%s(%s): rbus_closeBrokerConnection error %d", __FUNCTION__, opts->component_name, err);

exit_error1:

    if(rbusHandleList_IsEmpty())
    {
        _rbus_open_pre_initialize(false);
    }

    UnlockMutex();

    if(tmpHandle)
        rt_free(tmpHandle);

exit_error0:

    if(ret == RBUS_ERROR_SUCCESS)
        ret = RBUS_ERROR_BUS_ERROR;

    return ret;
}

rbusError_t rbus_close(rbusHandle_t handle)
{
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    char* componentName = NULL;

    VERIFY_NULL(handle);

    RBUSLOG_INFO("%s(%s)", __FUNCTION__, handleInfo->componentName);

    LockMutex();

    if(handleInfo->eventSubs)
    {
        int i;
        int count = (int)rtVector_Size(handleInfo->eventSubs);
        for(i = 0; i < count; ++i)
        {
            rbusEvent_UnsubscribeEx(handle, (rbusEventSubscription_t*)rtVector_At(handleInfo->eventSubs, 0), 1);
        }
        count = (int)rtVector_Size(handleInfo->eventSubs);
        if(count)
        {
            RBUSLOG_ERROR("%s(%s): failed to unsubscribe %d subscriptions", __FUNCTION__, handleInfo->componentName, count);
            ret = RBUS_ERROR_BUS_ERROR;
        }
        rtVector_Destroy(handleInfo->eventSubs, NULL);
        handleInfo->eventSubs = NULL;
    }

    if (handleInfo->messageCallbacks)
    {
        rbusMessage_RemoveAllListeners(handle);
        rtVector_Destroy(handleInfo->messageCallbacks, NULL);
        handleInfo->messageCallbacks = NULL;
    }

    if(handleInfo->subscriptions != NULL)
    {
        rbusSubscriptions_destroy(handleInfo->subscriptions);
        handleInfo->subscriptions = NULL;
    }

    rbusValueChange_Destroy(&handle->valueChangeDetector);

    rbusAsyncSubscribe_CloseHandle(handle);

    if(handleInfo->elementRoot)
    {
        freeElementNode(handleInfo->elementRoot);
        handleInfo->elementRoot = NULL;
    }

    if((err = rbus_unregisterObj(handleInfo->componentName)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s(%s): rbus_unregisterObj error %d", __FUNCTION__, handleInfo->componentName, err);
        ret = RBUS_ERROR_INVALID_HANDLE;
    }

    componentName = handleInfo->componentName;

    rbusHandleList_Remove(handleInfo);

    if(rbusHandleList_IsEmpty())
    {
        RBUSLOG_DEBUG("%s(%s): closing broker connection", __FUNCTION__, componentName);

        //calling before closing connection
        if((err = rbus_unregisterClientDisconnectHandler()) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s(%s): rbus_unregisterClientDisconnectHandler error %d", __FUNCTION__, componentName, err);
            ret = RBUS_ERROR_BUS_ERROR;
        }

        if((err = rbus_closeBrokerConnection()) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s(%s): rbus_closeBrokerConnection error %d", __FUNCTION__, componentName, err);
            ret = RBUS_ERROR_BUS_ERROR;
        }

        _rbus_open_pre_initialize(false);
    }

    UnlockMutex();

    if(ret == RBUS_ERROR_SUCCESS)
        RBUSLOG_INFO("%s(%s) success", __FUNCTION__, componentName);

    free(componentName);

    return ret;
}

rbusError_t rbus_regDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t *elements)
{
    static bool sDisConnHandler = false;
    int i;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    VERIFY_NULL(handleInfo);
    VERIFY_NULL(elements);
    VERIFY_ZERO(numDataElements);

    for(i=0; i<numDataElements; ++i)
    {
        char* name = elements[i].name;

        if((!name) || (0 == strlen(name))) {
            rc = RBUS_ERROR_INVALID_INPUT;
            break ;
        }

        RBUSLOG_DEBUG("%s: %s", __FUNCTION__, name);

        if(handleInfo->elementRoot == NULL)
        {
            RBUSLOG_DEBUG("First Time, create the root node for [%s]!", handleInfo->componentName);
            handleInfo->elementRoot = getEmptyElementNode();
            handleInfo->elementRoot->name = strdup(handleInfo->componentName);
            RBUSLOG_DEBUG("Root node created for [%s]", handleInfo->elementRoot->name);
        }

        if(handleInfo->subscriptions == NULL)
        {
            rbusSubscriptions_create(&handleInfo->subscriptions, handle, handleInfo->componentName, handleInfo->elementRoot, rbusConfig_Get()->tmpDir);
        }

        if((err = rbus_addElement(handleInfo->componentName, name)) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s: failed to add element with core [%s] err=%d!!", __FUNCTION__, name, err);
            rc = RBUS_ERROR_ELEMENT_NAME_DUPLICATE;
            break;
        }
        else
        {
            elementNode* node;
            if((node = insertElement(handleInfo->elementRoot, &elements[i])) == NULL)
            {
                RBUSLOG_ERROR("%s: failed to insert element [%s]!!", __FUNCTION__, name);
                rc = RBUS_ERROR_OUT_OF_RESOURCES;
                break;
            }
            else
            {
                rbusSubscriptions_resubscribeElementCache(handle, handleInfo->subscriptions, name, node);
                RBUSLOG_INFO("%s inserted successfully!", name);
            }
        }
    }

    /*TODO: need to review if this is how we should handle any failed register.
      To avoid a provider having a half registered data model, and to avoid
      the complexity of returning a list of error codes for each element in the list,
      we treat rbus_regDataElements as a transaction.  If any element from the elements list
      fails to register, we abort the whole thing.  We do this as follows: As soon 
      as 1 fail occurs above, we break out of loop and we unregister all the 
      successfully registered elements that happened during this call, before we failed.
      Thus we unregisters elements 0 to i (i was when we broke from loop above).*/
    if(rc != RBUS_ERROR_SUCCESS && i > 0)
        rbus_unregDataElements(handle, i, elements);

    if((rc == RBUS_ERROR_SUCCESS) && (!sDisConnHandler))
    {
        err = rbus_registerClientDisconnectHandler(_client_disconnect_callback_handler);
        if(err != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s : rbus_registerClientDisconnectHandler error %d", __FUNCTION__, err);
        }
        else
            sDisConnHandler = true;
    }

    return rc;
}

rbusError_t rbus_unregDataElements(
    rbusHandle_t handle,
    int numDataElements,
    rbusDataElement_t *elements)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    int i;

    VERIFY_NULL(handleInfo);
    VERIFY_NULL(elements);
    VERIFY_ZERO(numDataElements);

    for(i=0; i<numDataElements; ++i)
    {
        char const* name = elements[i].name;
/*
        if(rbus_unregisterEvent(handleInfo->componentName, name) != RBUSCORE_SUCCESS)
            RBUSLOG_INFO("%s: failed to remove event [%s]!!", __FUNCTION__, name);
*/
        if(rbus_removeElement(handleInfo->componentName, name) != RBUSCORE_SUCCESS)
            RBUSLOG_WARN("%s: failed to remove element from core [%s]!!", __FUNCTION__, name);

/*      TODO: we need to remove all instance elements that this registration element instantiated
        rbusValueChange_RemoveParameter(handle, NULL, name);
        removeElement(&(handleInfo->elementRoot), name);
*/
    }
    return RBUS_ERROR_SUCCESS;
}

//************************* Discovery related Operations *******************//
rbusError_t rbus_discoverComponentName (rbusHandle_t handle,
                            int numElements, char const** elementNames,
                            int *numComponents, char ***componentName)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    if(handle == NULL)
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    else if ((numElements < 1) || (NULL == elementNames[0]))
    {
        RBUSLOG_WARN("Invalid input passed to rbus_discoverElementsObjects");
        return RBUS_ERROR_INVALID_INPUT;
    }
    *numComponents = 0;
    *componentName = 0;
    char **output = NULL;
    int out_count = 0;
    if(RBUSCORE_SUCCESS == rbus_discoverElementsObjects(numElements, elementNames, &out_count, &output))
    {
        *componentName = output;
        *numComponents = out_count;
    }
    else
    {
         RBUSLOG_WARN("return from rbus_discoverElementsObjects is not success");
    }
  
    return errorcode;
}

rbusError_t rbus_discoverComponentDataElements (rbusHandle_t handle,
                            char const* name, bool RT_UNUSED(nextLevel),
                            int *numElements, char*** elementNames)
{
    rbusCoreError_t ret;

    *numElements = 0;
    *elementNames = 0;
    VERIFY_NULL(handle);
    VERIFY_NULL(name);

    ret = rbus_discoverObjectElements(name, numElements, elementNames);

    /* mrollins FIXME what was this doing before and do i need it after I changed the rbuscore discover api ?
        if(*numElements > 0)
            *numElements = *numElements-1;  //Fix this. We need a better way to ignore the component name as element name.
    */

    return ret == RBUSCORE_SUCCESS ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
}

//************************* Parameters related Operations *******************//
rbusError_t rbus_get(rbusHandle_t handle, char const* name, rbusValue_t* value)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage request, response;
    int ret = -1;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;

    rbusCheckAsyncInvalid(handle);

    VERIFY_NULL(handleInfo);

    /* Is it a valid Query */
    if (!_is_valid_get_query(name))
    {
        RBUSLOG_WARN("%s This method is only to get Parameters", __FUNCTION__);
        return RBUS_ERROR_INVALID_INPUT;
    }

    if (_is_wildcard_query(name))
    {
        RBUSLOG_WARN("%s This method does not support wildcard query", __FUNCTION__);
        return RBUS_ERROR_ACCESS_NOT_ALLOWED;
    }

    rbusMessage_Init(&request);
    /* Set the Component name that invokes the set */
    rbusMessage_SetString(request, handleInfo->componentName);
    /* Param Size */
    rbusMessage_SetInt32(request, (int32_t)1);
    rbusMessage_SetString(request, name);

    RBUSLOG_DEBUG("Calling rbus_invokeRemoteMethod for [%s]", name);

    if((err = rbus_invokeRemoteMethod(name, METHOD_GETPARAMETERVALUES, request, rbusConfig_ReadGetTimeout(), &response)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, name);
        errorcode = rbusCoreError_to_rbusError(err);
    }
    else
    {
        int valSize;
        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;

        RBUSLOG_DEBUG("Received response for remote method invocation!");

        rbusMessage_GetInt32(response, &ret);

        RBUSLOG_DEBUG("Response from the remote method is [%d]!",ret);
        errorcode = (rbusError_t) ret;
        legacyRetCode = (rbusLegacyReturn_t) ret;

        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            errorcode = RBUS_ERROR_SUCCESS;
            RBUSLOG_DEBUG("Received valid response!");
            rbusMessage_GetInt32(response, &valSize);
            if(1/*valSize*/)
            {
                char const *buff = NULL;

                //Param Name
                rbusMessage_GetString(response, &buff);
                if(buff && (strcmp(name, buff) == 0))
                {
                    rbusValue_initFromMessage(value, response);
                }
                else
                {
                    RBUSLOG_WARN("Param mismatch!");
                    RBUSLOG_WARN("Requested param: [%s], Received Param: [%s]", name, buff);
                    errorcode = RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION;
                }
            }
        }
        else
        {
            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
            {
                errorcode = CCSPError_to_rbusError(legacyRetCode);
            }
        }
        rbusMessage_Release(response);
    }
    return errorcode;
}

rbusError_t _getExt_response_parser(rbusMessage response, int *numValues, rbusProperty_t* retProperties)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
    int numOfVals = 0;
    int ret = -1;
    int i = 0;
    RBUSLOG_DEBUG("Received response for remote method invocation!");

    rbusMessage_GetInt32(response, &ret);
    RBUSLOG_DEBUG("Response from the remote method is [%d]!",ret);

    errorcode = (rbusError_t) ret;
    legacyRetCode = (rbusLegacyReturn_t) ret;

    *numValues = 0;
    if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
    {
        errorcode = RBUS_ERROR_SUCCESS;
        RBUSLOG_DEBUG("Received valid response!");
        rbusMessage_GetInt32(response, &numOfVals);
        *numValues = numOfVals;
        RBUSLOG_DEBUG("Number of return params = %d", numOfVals);

        if(numOfVals)
        {
            rbusProperty_t last;
            for(i = 0; i < numOfVals; i++)
            {
                /* For the first instance, lets use the given pointer */
                if (0 == i)
                {
                    rbusProperty_initFromMessage(retProperties, response);
                    last = *retProperties;
                }
                else
                {
                    rbusProperty_t tmpProperties;
                    rbusProperty_initFromMessage(&tmpProperties, response);
                    rbusProperty_SetNext(last, tmpProperties);
                    rbusProperty_Release(tmpProperties);
                    last = tmpProperties;
                }
            }
        }
    }
    else
    {
        if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
        {
            errorcode = CCSPError_to_rbusError(legacyRetCode);
        }
    }
    rbusMessage_Release(response);

    return errorcode;
}

rbusError_t rbus_getExt(rbusHandle_t handle, int paramCount, char const** pParamNames, int *numValues, rbusProperty_t* retProperties)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    int i;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;

    VERIFY_NULL(handleInfo);
    VERIFY_NULL(pParamNames);
    VERIFY_NULL(numValues);
    VERIFY_NULL(retProperties);
    VERIFY_ZERO(paramCount);

    if ((1 == paramCount) && (_is_wildcard_query(pParamNames[0])))
    {
        int numDestinations = 0;
        char** destinations;
        //int length = strlen(pParamNames[0]);

        err = rbus_discoverWildcardDestinations(pParamNames[0], &numDestinations, &destinations);
        if (RBUSCORE_SUCCESS == err)
        {
            RBUSLOG_DEBUG("Query for expression %s was successful. See result below:", pParamNames[0]);
            rbusProperty_t last = NULL;
            *numValues = 0;
            if (0 == numDestinations)
            {
                RBUSLOG_DEBUG("It is possibly a table entry from single component.");
            }
            else
            {
                for(i = 0; i < numDestinations; i++)
                {
                    int tmpNumOfValues = 0;
                    rbusMessage request, response;
                    RBUSLOG_DEBUG("Destination %d is %s", i, destinations[i]);

                    /* Get the query sent to each component identified */
                    rbusMessage_Init(&request);
                    /* Set the Component name that invokes the set */
                    rbusMessage_SetString(request, handleInfo->componentName);
                    rbusMessage_SetInt32(request, 1);
                    rbusMessage_SetString(request, pParamNames[0]);
                    /* Invoke the method */
                    if((err = rbus_invokeRemoteMethod(destinations[i], METHOD_GETPARAMETERVALUES, request, rbusConfig_ReadGetTimeout(), &response)) != RBUSCORE_SUCCESS)
                    {
                        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, destinations[i]);
                        errorcode = rbusCoreError_to_rbusError(err);
                    }
                    else
                    {
                        if (0 == i)
                        {
                            if((errorcode = _getExt_response_parser(response, &tmpNumOfValues, retProperties)) != RBUS_ERROR_SUCCESS)
                            {
                                RBUSLOG_ERROR("%s error parsing response %d", __FUNCTION__, errorcode);
                            }
                            else
                            {
                                if(tmpNumOfValues > 0)
                                    last = *retProperties;
                            }
                        }
                        else
                        {
                            rbusProperty_t tmpProperties;

                            if((errorcode = _getExt_response_parser(response, &tmpNumOfValues, &tmpProperties)) != RBUS_ERROR_SUCCESS)
                            {
                                RBUSLOG_ERROR("%s error parsing response %d", __FUNCTION__, errorcode);
                            }
                            else
                            {
                                if(tmpNumOfValues > 0 && tmpProperties)
                                {
                                    if(NULL != last)
                                    {
                                        rbusProperty_Append(last, tmpProperties);
                                    }
                                    else
                                    {
                                        last = tmpProperties;
                                        *retProperties = last;
                                    }
                                }
                            }
                        }
                    }
                    if (errorcode != RBUS_ERROR_SUCCESS)
                    {
                        RBUSLOG_WARN("Failed to get the data from %s Component", destinations[i]);
                        break;
                    }
                    else
                    {
                        *numValues += tmpNumOfValues;
                    }
                }

                for(i = 0; i < numDestinations; i++)
                    free(destinations[i]);
                free(destinations);

                return errorcode;
            }
        }
        else
        {
            RBUSLOG_DEBUG("Query for expression %s was not successful.", pParamNames[0]);
            return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }
    }

    {
        rbusMessage request, response;
        int numComponents;
        char** componentNames = NULL;

        /*discover which components have some ownership of the params in the list*/
        errorcode = rbus_discoverComponentName(handle, paramCount, pParamNames, &numComponents, &componentNames);
        if(errorcode == RBUS_ERROR_SUCCESS && paramCount == numComponents)
        {
#if 0
            RBUSLOG_DEBUG("rbus_discoverComponentName return %d component for %d params", numComponents, paramCount);
            for(i = 0; i < numComponents; ++i)
            {
                RBUSLOG_DEBUG("%d: %s %s",i, pParamNames[i], componentNames[i]);
            }
#endif
            for(i = 0; i < paramCount; ++i)
            {
                if(!componentNames[i] || !componentNames[i][0])
                {
                    RBUSLOG_ERROR("Cannot find component for %s", pParamNames[i]);
                    errorcode = RBUS_ERROR_INVALID_INPUT;
                }
            }

            if(errorcode == RBUS_ERROR_INVALID_INPUT)
            {
                free(componentNames);
                return RBUS_ERROR_INVALID_INPUT;
            }

            *retProperties = NULL;/*NULL to mark first batch*/
            *numValues = 0;

            /*batch by component*/
            for(;;)
            {
                char* componentName = NULL;
                char const* firstParamName = NULL;
                int batchCount = 0;

                for(i = 0; i < paramCount; ++i)
                {
                    if(componentNames[i])
                    {
                        if(!componentName)
                        {
                            RBUSLOG_DEBUG("%s starting batch for component %s", __FUNCTION__, componentNames[i]);
                            componentName = strdup(componentNames[i]);
                            firstParamName = pParamNames[i];
                            batchCount = 1;
                        }
                        else if(strcmp(componentName, componentNames[i]) == 0)
                        {
                            batchCount++;
                        }
                    }
                }

                if(componentName)
                {
                    rbusMessage_Init(&request);
                    rbusMessage_SetString(request, componentName);
                    rbusMessage_SetInt32(request, batchCount);

                    RBUSLOG_DEBUG("batchCount %d", batchCount);

                    for(i = 0; i < paramCount; ++i)
                    {
                        if(componentNames[i] && strcmp(componentName, componentNames[i]) == 0)
                        {
                            RBUSLOG_DEBUG("%s adding %s to batch", __FUNCTION__, pParamNames[i]);                            
                            rbusMessage_SetString(request, pParamNames[i]);

                            /*free here so its removed from batch scan*/
                            free(componentNames[i]);
                            componentNames[i] = NULL;
                        }
                    }                  

                    RBUSLOG_DEBUG("%s sending batch request with %d params to component %s", __FUNCTION__, batchCount, componentName);
                    free(componentName);

                    if((err = rbus_invokeRemoteMethod(firstParamName, METHOD_GETPARAMETERVALUES, request, rbusConfig_ReadGetTimeout(), &response)) != RBUSCORE_SUCCESS)
                    {
                        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, firstParamName);
                        errorcode = rbusCoreError_to_rbusError(err);
                        break;
                    }
                    else
                    {
                        rbusProperty_t batchResult;
                        int batchNumVals;
                        if((errorcode = _getExt_response_parser(response, &batchNumVals, &batchResult)) != RBUS_ERROR_SUCCESS)
                        {
                            RBUSLOG_ERROR("%s error parsing response %d", __FUNCTION__, errorcode);
                        }
                        else
                        {
                            RBUSLOG_DEBUG("%s got valid response", __FUNCTION__);
                            if(*retProperties == NULL) /*first batch*/
                            {
                                *retProperties = batchResult;
                            }
                            else /*append subsequent batches*/
                            {
                                rbusProperty_Append(*retProperties, batchResult);
                                rbusProperty_Release(batchResult);
                            }
                            *numValues += batchNumVals;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            errorcode = RBUS_ERROR_DESTINATION_NOT_REACHABLE;
            RBUSLOG_ERROR("Discover component names failed with error %d and counts %d/%d", errorcode, paramCount, numComponents);
        }
        if(componentNames)
            free(componentNames);
    }
    return errorcode;
}

static rbusError_t rbus_getByType(rbusHandle_t handle, char const* paramName, void* paramVal, rbusValueType_t type)
{
    rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
    VERIFY_NULL(handle);
    if (paramVal && paramName)
    {
        rbusValue_t value;
        
        errorcode = rbus_get(handle, paramName, &value);

        if (errorcode == RBUS_ERROR_SUCCESS)
        {
            if (rbusValue_GetType(value) == type)
            {
                switch(type)
                {
                    case RBUS_BOOLEAN:
                        *((bool*)paramVal) = rbusValue_GetBoolean(value);
                        break;
                    case RBUS_INT32:
                        *((int*)paramVal) = rbusValue_GetInt32(value);
                        break;
                    case RBUS_UINT32:
                        *((unsigned int*)paramVal) = rbusValue_GetUInt32(value);
                        break;
                    case RBUS_STRING:
                        *((char**)paramVal) = strdup(rbusValue_GetString(value,NULL));
                        break;
                    default:
                        RBUSLOG_WARN("%s unexpected type param %d", __FUNCTION__, type);
                        break;
                }

            }
            else
            {
                RBUSLOG_ERROR("%s rbus_get type missmatch. expected %d. got %d", __FUNCTION__, type, rbusValue_GetType(value));
                errorcode = RBUS_ERROR_BUS_ERROR;
            }
            rbusValue_Release(value);
        }
    }
    return errorcode;
}

rbusError_t rbus_getBoolean(rbusHandle_t handle, char const* paramName, bool* paramVal)
{
    return rbus_getByType(handle, paramName, paramVal, RBUS_BOOLEAN);
}

rbusError_t rbus_getInt(rbusHandle_t handle, char const* paramName, int* paramVal)
{
    return rbus_getByType(handle, paramName, paramVal, RBUS_INT32);
}

rbusError_t rbus_getUint (rbusHandle_t handle, char const* paramName, unsigned int* paramVal)
{
    return rbus_getByType(handle, paramName, paramVal, RBUS_UINT32);
}

rbusError_t rbus_getStr (rbusHandle_t handle, char const* paramName, char** paramVal)
{
    return rbus_getByType(handle, paramName, paramVal, RBUS_STRING);
}

rbusError_t rbus_set(rbusHandle_t handle, char const* name,rbusValue_t value, rbusSetOptions_t* opts)
{
    rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setRequest, setResponse;

    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;

    VERIFY_NULL(handle);
    VERIFY_NULL(name);
    VERIFY_NULL(value);
    rbusCheckAsyncInvalid(handle);

    if (RBUS_NONE == rbusValue_GetType(value))
    {
        return errorcode;
    }
    rbusMessage_Init(&setRequest);
    /* Set the Session ID first */
    if ((opts) && (opts->sessionId != 0))
        rbusMessage_SetInt32(setRequest, opts->sessionId);
    else
        rbusMessage_SetInt32(setRequest, 0);

    /* Set the Component name that invokes the set */
    rbusMessage_SetString(setRequest, handleInfo->componentName);
    /* Set the Size of params */
    rbusMessage_SetInt32(setRequest, 1);

    /* Set the params in details */
    rbusValue_appendToMessage(name, value, setRequest);

    /* Set the Commit value; FIXME: Should we use string? */
    rbusMessage_SetString(setRequest, (!opts || opts->commit) ? "TRUE" : "FALSE");

    if((err = rbus_invokeRemoteMethod(name, METHOD_SETPARAMETERVALUES, setRequest, rbusConfig_ReadSetTimeout(), &setResponse)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, name);
        errorcode = rbusCoreError_to_rbusError(err);
    }
    else
    {
        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
        int ret = -1;
        char const* pErrorReason = NULL;
        rbusMessage_GetInt32(setResponse, &ret);

        RBUSLOG_DEBUG("Response from the remote method is [%d]!", ret);
        errorcode = (rbusError_t) ret;
        legacyRetCode = (rbusLegacyReturn_t) ret;

        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            errorcode = RBUS_ERROR_SUCCESS;
            RBUSLOG_DEBUG("Successfully Set the Value");
        }
        else
        {
            rbusMessage_GetString(setResponse, &pErrorReason);
            RBUSLOG_WARN("Failed to Set the Value for %s", pErrorReason);
            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
            {
                errorcode = CCSPError_to_rbusError(legacyRetCode);
            }
        }

        /* Release the reponse message */
        rbusMessage_Release(setResponse);
    }
    return errorcode;
}

rbusError_t rbus_setMulti(rbusHandle_t handle, int numProps, rbusProperty_t properties, rbusSetOptions_t* opts)
{
    rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setRequest, setResponse;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;
    rbusValueType_t type = RBUS_NONE;
    rbusProperty_t current;

    VERIFY_NULL(handle);

    if (numProps > 0 && properties != NULL)
    {
        char const** pParamNames;
        int numComponents;
        char** componentNames = NULL;
        int i;

        /*create list of paramNames to pass to rbus_discoverComponentName*/
        pParamNames = rt_try_malloc(sizeof(char*) * numProps);
        if(!pParamNames)
        {
            RBUSLOG_WARN ("Failed to malloc %d property names", numProps);
            return RBUS_ERROR_OUT_OF_RESOURCES;
        }
        current = properties;
        i = 0;
        while(current && i < numProps)
        {
            pParamNames[i++] = rbusProperty_GetName(current);
            type = rbusValue_GetType(rbusProperty_GetValue(current));
            if (RBUS_NONE == type)
            {
                RBUSLOG_DEBUG("Invalid data type passed in one of the data type\n");
                free(pParamNames);
                return errorcode;
            }
            current = rbusProperty_GetNext(current);
        }
        if(i != numProps)
        {
            RBUSLOG_WARN ("Invalid input: numProps more then actual number of properties.");
            free(pParamNames);
            return RBUS_ERROR_INVALID_INPUT;
        }

        /*discover which components have some ownership of the params*/
        errorcode = rbus_discoverComponentName(handle, numProps, pParamNames, &numComponents, &componentNames);
        if(errorcode == RBUS_ERROR_SUCCESS && numProps == numComponents)
        {
#if 0
            RBUSLOG_DEBUG("rbus_discoverComponentName return %d component for %d params", numComponents, numProps);
            for(i = 0; i < numComponents; ++i)
            {
                RBUSLOG_DEBUG("%d: %s %s",i, pParamNames[i], componentNames[i]);
            }
#endif
            current = properties;
            for(i = 0; i < numProps; ++i, current = rbusProperty_GetNext(current))
            {
                if(!componentNames[i] || !componentNames[i][0])
                {
                    RBUSLOG_ERROR("Cannot find component for %s", rbusProperty_GetName(current));
                    errorcode = RBUS_ERROR_INVALID_INPUT;
                }
            }

            if(errorcode == RBUS_ERROR_INVALID_INPUT)
            {
                free(componentNames);
                return RBUS_ERROR_INVALID_INPUT;
            }

            for(;;)
            {
                char* componentName = NULL;
                char const* firstParamName = NULL;
                int batchCount = 0;

                for(i = 0; i < numProps; ++i)
                {
                    if(componentNames[i])
                    {
                        if(!componentName)
                        {
                            RBUSLOG_DEBUG("%s starting batch for component %s", __FUNCTION__, componentNames[i]);
                            componentName = strdup(componentNames[i]);
                            firstParamName = pParamNames[i];
                            batchCount = 1;
                        }
                        else if(strcmp(componentName, componentNames[i]) == 0)
                        {
                            batchCount++;
                        }
                    }
                }

                if(componentName)
                {

                    rbusMessage_Init(&setRequest);

                    /* Set the Session ID first */
                    if ((opts) && (opts->sessionId != 0))
                        rbusMessage_SetInt32(setRequest, opts->sessionId);
                    else
                        rbusMessage_SetInt32(setRequest, 0);

                    /* Set the Component name that invokes the set */
                    rbusMessage_SetString(setRequest, handleInfo->componentName);
                    /* Set the Size of params */
                    rbusMessage_SetInt32(setRequest, batchCount);

                    current = properties;
                    for(i = 0; i < numProps; ++i, current = rbusProperty_GetNext(current))
                    {
                        if(componentNames[i] && strcmp(componentName, componentNames[i]) == 0)
                        {
                            if(strcmp(pParamNames[i], rbusProperty_GetName(current)))
                                RBUSLOG_ERROR("paramName doesn't match current property");

                            RBUSLOG_DEBUG("%s adding %s to batch", __FUNCTION__, rbusProperty_GetName(current));                            
                            rbusValue_appendToMessage(rbusProperty_GetName(current), rbusProperty_GetValue(current), setRequest);

                            /*free here so its removed from batch scan*/
                            free(componentNames[i]);
                            componentNames[i] = NULL;
                        }
                    }  

                    /* Set the Commit value; FIXME: Should we use string? */
                    rbusMessage_SetString(setRequest, (!opts || opts->commit) ? "TRUE" : "FALSE");

                    if((err = rbus_invokeRemoteMethod(firstParamName, METHOD_SETPARAMETERVALUES, setRequest, rbusConfig_ReadSetTimeout(), &setResponse)) != RBUSCORE_SUCCESS)
                    {
                        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, firstParamName);
                        errorcode = rbusCoreError_to_rbusError(err);
                    }
                    else
                    {
                        char const* pErrorReason = NULL;
                        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
                        int ret = -1;
                        rbusMessage_GetInt32(setResponse, &ret);

                        RBUSLOG_DEBUG("Response from the remote method is [%d]!", ret);
                        errorcode = (rbusError_t) ret;
                        legacyRetCode = (rbusLegacyReturn_t) ret;

                        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
                        {
                            errorcode = RBUS_ERROR_SUCCESS;
                            RBUSLOG_DEBUG("Successfully Set the Value");
                        }
                        else
                        {
                            rbusMessage_GetString(setResponse, &pErrorReason);
                            RBUSLOG_WARN("Failed to Set the Value for %s", pErrorReason);
                            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
                            {
                                errorcode = CCSPError_to_rbusError(legacyRetCode);
                            }
                        }

                        /* Release the reponse message */
                        rbusMessage_Release(setResponse);
                    }
                    free(componentName);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            errorcode = RBUS_ERROR_DESTINATION_NOT_REACHABLE;
            RBUSLOG_ERROR("Discover component names failed with error %d and counts %d/%d", errorcode, numProps, numComponents);
            for(i = 0; i < numComponents; i++)
            {
                if(componentNames[i])
                {
                    free(componentNames[i]);
                    componentNames[i] = NULL;
                }
            }
        }
        if(pParamNames)
            free(pParamNames);
        if(componentNames)
            free(componentNames);
    }
    return errorcode;
}

#if 0
rbusError_t rbus_setMulti(rbusHandle_t handle, int numValues,
        char const** valueNames, rbusValue_t* values, rbusSetOptions_t* opts)
{
    rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage setRequest, setResponse;
    int loopCnt = 0;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;

    if (values != NULL)
    {
        rbusMessage_Init(&setRequest);
        /* Set the Session ID first */
        rbusMessage_SetInt32(setRequest, 0);
        /* Set the Component name that invokes the set */
        rbusMessage_SetString(setRequest, handleInfo->componentName);
        /* Set the Size of params */
        rbusMessage_SetInt32(setRequest, numValues);

        /* Set the params in details */
        for (loopCnt = 0; loopCnt < numValues; loopCnt++)
        {
            rbusValue_appendToMessage(valueNames[loopCnt], values[loopCnt], setRequest);
        }

        /* Set the Commit value; FIXME: Should we use string? */
        rbusMessage_SetString(setRequest, (!opts || opts->commit) ? "TRUE" : "FALSE");

        /* TODO: At this point in time, only given Table/Component can be updated with SET/GET..
         * So, passing the elementname as first arg is not a issue for now..
         * We must enhance the rbus in such a way that we shd be able to set across components. Lets revist this area at that time.
         */
#if 0
        /* TODO: First step towards the above comment. When we enhace to support acorss components, this following has to be looped or appropriate count will be passed */
        char const* pElementNames[] = {values[0].name, NULL};
        char** pComponentName = NULL;
        err = rbus_discoverElementObjects(pElementNames, 1, &pComponentName);
        if (err != RBUSCORE_SUCCESS)
        {
            RBUSLOG_INFO ("Element not found");
            errorcode = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }
        else
        {
            RBUSLOG_INFO ("Component name is, %s", pComponentName[0]);
            free (pComponentName[0]);
        }
#endif
        if((err = rbus_invokeRemoteMethod(valueNames[0], METHOD_SETPARAMETERVALUES, setRequest, 6000, &setResponse)) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_INFO("%s rbus_invokeRemoteMethod failed with err %d", __FUNCTION__, err);
            errorcode = RBUS_ERROR_BUS_ERROR;
        }
        else
        {
            char const* pErrorReason = NULL;
            rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
            int ret = -1;
            rbusMessage_GetInt32(setResponse, &ret);

            RBUSLOG_INFO("Response from the remote method is [%d]!", ret);
            errorcode = (rbusError_t) ret;
            legacyRetCode = (rbusLegacyReturn_t) ret;

            if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
            {
                errorcode = RBUS_ERROR_SUCCESS;
                RBUSLOG_INFO("Successfully Set the Value");
            }
            else
            {
                rbusMessage_GetString(setResponse, &pErrorReason);
                RBUSLOG_INFO("Failed to Set the Value for %s", pErrorReason);
            }

            /* Release the reponse message */
            rbusMessage_Release(setResponse);
        }
    }
    return errorcode;
}
#endif
static rbusError_t rbus_setByType(rbusHandle_t handle, char const* paramName, void const* paramVal, rbusValueType_t type)
{
    rbusError_t errorcode = RBUS_ERROR_INVALID_INPUT;

    VERIFY_NULL(handle);
    VERIFY_NULL(paramName);
    rbusValue_t value;

    rbusValue_Init(&value);

    switch(type)
    {
        case RBUS_BOOLEAN:
            rbusValue_SetBoolean(value, *((bool*)paramVal));
            break;
        case RBUS_INT32:
            rbusValue_SetInt32(value, *((int*)paramVal));
            break;
        case RBUS_UINT32:
            rbusValue_SetUInt32(value, *((unsigned int*)paramVal));
            break;
        case RBUS_STRING:
            rbusValue_SetString(value, (char*)paramVal);
            break;
        default:
            RBUSLOG_WARN("%s unexpected type param %d", __FUNCTION__, type);
            break;
    }

    errorcode = rbus_set(handle, paramName, value, NULL);

    rbusValue_Release(value);

    return errorcode;
}

rbusError_t rbus_setBoolean(rbusHandle_t handle, char const* paramName, bool paramVal)
{
    return rbus_setByType(handle, paramName, &paramVal, RBUS_BOOLEAN);
}

rbusError_t rbus_setInt(rbusHandle_t handle, char const* paramName, int paramVal)
{
    return rbus_setByType(handle, paramName, &paramVal, RBUS_INT32);
}

rbusError_t rbus_setUInt(rbusHandle_t handle, char const* paramName, unsigned int paramVal)
{
    return rbus_setByType(handle, paramName, &paramVal, RBUS_UINT32);
}

rbusError_t rbus_setStr(rbusHandle_t handle, char const* paramName, char const* paramVal)
{
    VERIFY_NULL(paramVal);
    return rbus_setByType(handle, paramName, paramVal, RBUS_STRING);
}

rbusError_t rbusTable_addRow(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    rbusCoreError_t err;
    int returnCode = 0;
    int32_t instanceId = 0;
    const char dot = '.';
    rbusMessage request, response;
    (void)handle;
    rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;

    VERIFY_NULL(handle);
    VERIFY_NULL(tableName);

    RBUSLOG_DEBUG("%s: %s %s", __FUNCTION__, tableName, aliasName);

    if(tableName[strlen(tableName)-1] != dot)
    {
        RBUSLOG_WARN("%s invalid table name %s", __FUNCTION__, tableName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusMessage_Init(&request);
    rbusMessage_SetInt32(request, 0);/*TODO: this should be the session ID*/
    rbusMessage_SetString(request, tableName);/*TODO: do we need to append the name as well as pass the name as the 1st arg to rbus_invokeRemoteMethod ?*/
    if(aliasName)
        rbusMessage_SetString(request, aliasName);
    else
        rbusMessage_SetString(request, "");

    if((err = rbus_invokeRemoteMethod(
        tableName, /*as taken from ccsp_base_api.c, this was the destination component ID, but to locate the route, the table name can be used
                     because the broker simlpy looks at the top level nodes that are owned by a component route.  maybe this breaks if the broker changes*/
        METHOD_ADDTBLROW, 
        request, 
        rbusConfig_ReadSetTimeout(),
        &response)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, tableName);
        return rbusCoreError_to_rbusError(err);
    }
    else
    {
        rbusMessage_GetInt32(response, &returnCode);
        rbusMessage_GetInt32(response, &instanceId);
        legacyRetCode = (rbusLegacyReturn_t)returnCode;

        if(instNum)
            *instNum = (uint32_t)instanceId;/*FIXME we need an rbus_PopUInt32 to avoid loosing a bit */

        RBUSLOG_INFO("%s rbus_invokeRemoteMethod success response returnCode:%d instanceId:%d", __FUNCTION__, returnCode, instanceId);
        if((returnCode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            returnCode = RBUS_ERROR_SUCCESS;
            RBUSLOG_DEBUG("Successfully Set the Value");
        }
        else
        {
            RBUSLOG_WARN("Response from remote method indicates the call failed!!");
            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
            {
                returnCode = CCSPError_to_rbusError(legacyRetCode);
            }
        }
        rbusMessage_Release(response);
    }

    return returnCode;
}

rbusError_t rbusTable_removeRow(
    rbusHandle_t handle,
    char const* rowName)
{
    rbusCoreError_t err;
    int returnCode = 0;
    rbusMessage request, response;
    (void)handle;
    rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;

    VERIFY_NULL(handle);
    VERIFY_NULL(rowName);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, rowName);

    rbusMessage_Init(&request);
    rbusMessage_SetInt32(request, 0);/*TODO: this should be the session ID*/
    rbusMessage_SetString(request, rowName);/*TODO: do we need to append the name as well as pass the name as the 1st arg to rbus_invokeRemoteMethod ?*/

    if((err = rbus_invokeRemoteMethod(
        rowName,
        METHOD_DELETETBLROW, 
        request, 
        rbusConfig_ReadSetTimeout(),
        &response)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, rowName);
        return rbusCoreError_to_rbusError(err);
    }
    else
    {
        rbusMessage_GetInt32(response, &returnCode);
        legacyRetCode = (rbusLegacyReturn_t)returnCode;

        RBUSLOG_INFO("%s rbus_invokeRemoteMethod success response returnCode:%d", __FUNCTION__, returnCode);
        if((returnCode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            returnCode = RBUS_ERROR_SUCCESS;
            RBUSLOG_DEBUG("Successfully Set the Value");
        }
        else
        {
            RBUSLOG_WARN("Response from remote method indicates the call failed!!");
            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
            {
                returnCode = CCSPError_to_rbusError(legacyRetCode);
            }
        }
        rbusMessage_Release(response);
    }

    return returnCode;
}

rbusError_t rbusTable_registerRow(
    rbusHandle_t handle,
    char const* tableName,
    uint32_t instNum,
    char const* aliasName)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    char rowName[RBUS_MAX_NAME_LENGTH] = {0};
    int rc;

    VERIFY_NULL(handleInfo);
    VERIFY_NULL(tableName);

    rc = snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d", tableName, instNum);
    if(rc < 0 || rc >= RBUS_MAX_NAME_LENGTH)
    {
        RBUSLOG_WARN("%s: invalid table name %s", __FUNCTION__, tableName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    elementNode* rowInstElem = retrieveInstanceElement(handleInfo->elementRoot, rowName);
    elementNode* tableInstElem = retrieveInstanceElement(handleInfo->elementRoot, tableName);

    if(rowInstElem)
    {
        RBUSLOG_WARN("%s: row already exists %s", __FUNCTION__, rowName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(!tableInstElem)
    {
        RBUSLOG_WARN("%s: table does not exist %s", __FUNCTION__, tableName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    RBUSLOG_DEBUG("%s: register table row %s", __FUNCTION__, rowName);
    registerTableRow(handle, tableInstElem, tableName, aliasName, instNum);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusTable_unregisterRow(
    rbusHandle_t handle,
    char const* rowName)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    VERIFY_NULL(handleInfo);
    VERIFY_NULL(rowName);

    elementNode* rowInstElem = retrieveInstanceElement(handleInfo->elementRoot, rowName);

    if(!rowInstElem)
    {
        RBUSLOG_DEBUG("%s: row does not exists %s", __FUNCTION__, rowName);
        return RBUS_ERROR_INVALID_INPUT;
    }

    unregisterTableRow(handle, rowInstElem);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusTable_getRowNames(
    rbusHandle_t handle,
    char const* tableName,
    rbusRowName_t** rowNames)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage request, response;

    VERIFY_NULL(handle);

    *rowNames = NULL;

    rbusMessage_Init(&request);
    rbusMessage_SetString(request, tableName);
    rbusMessage_SetInt32(request, -1);/*nextLevel*/
    rbusMessage_SetInt32(request, 1);/*getRowNames*/

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, tableName);

    if((err = rbus_invokeRemoteMethod(tableName, METHOD_GETPARAMETERNAMES, request, rbusConfig_ReadGetTimeout(), &response)) == RBUSCORE_SUCCESS)
    {
        rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
        int ret = -1;

        rbusMessage_GetInt32(response, &ret);

        errorcode = (rbusError_t) ret;
        legacyRetCode = (rbusLegacyReturn_t) ret;

        if((errorcode == RBUS_ERROR_SUCCESS) || (legacyRetCode == RBUS_LEGACY_ERR_SUCCESS))
        {
            int count, i;
            rbusRowName_t* tmpNames = NULL;

            errorcode = RBUS_ERROR_SUCCESS;

            rbusMessage_GetInt32(response, &count);

            RBUSLOG_DEBUG("%s: getparamnames %s got %d results", __FUNCTION__, tableName, count);

            if(count > 0)
            {
                tmpNames = rt_try_malloc(count * sizeof(struct _rbusRowName));
                if(!tmpNames)
                {
                    RBUSLOG_ERROR("%s:failed to malloc %d row names", __FUNCTION__, count);    
                    rbusMessage_Release(response);
                    return RBUS_ERROR_OUT_OF_RESOURCES;
                }
            }

            for(i = 0; i < count; ++i)
            {
                int32_t instNum;
                char const* alias = NULL;
                char fullName[RBUS_MAX_NAME_LENGTH];

                rbusMessage_GetInt32(response, &instNum);
                rbusMessage_GetString(response, &alias);
                snprintf(fullName, RBUS_MAX_NAME_LENGTH, "%s%d.", tableName, instNum);
                tmpNames[i].name = strdup(fullName);
                tmpNames[i].instNum = instNum;
                tmpNames[i].alias = alias && alias[0] != '\0' ? strdup(alias) : NULL;

                if(i < count -1)
                    tmpNames[i].next = &tmpNames[i+1];
                else
                    tmpNames[i].next = NULL;
            }

            *rowNames = tmpNames;
        }
        else
        {
            RBUSLOG_ERROR("%s: getparamnames %s failed with provider err %d", __FUNCTION__, tableName, err);
            if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
            {
                errorcode = CCSPError_to_rbusError(legacyRetCode);
            }
        }
        rbusMessage_Release(response);
    }
    else
    {
        RBUSLOG_ERROR("%s: getparamnames %s failed with buss err %d", __FUNCTION__, tableName, err);
        errorcode = rbusCoreError_to_rbusError(err);
    }
    return errorcode;
}

rbusError_t rbusTable_freeRowNames(
    rbusHandle_t handle,
    rbusRowName_t* rowNames)
{
    VERIFY_NULL(handle);
    if(rowNames)
    {
        rbusRowName_t* row = rowNames;
        while(row)
        {
            if(row->name)
                free((char*)row->name);
            if(row->alias)
                free((char*)row->alias);
            row = row->next;
        }
        free(rowNames);
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusElementInfo_get(
    rbusHandle_t handle,
    char const* elemName,
    int depth,
    rbusElementInfo_t** elemInfo)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage request, response;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*) handle;
    int numDestinations = 0;
    char** destinations = NULL;
    int d;
    int runningCount = 0;

    VERIFY_NULL(elemInfo);
    *elemInfo = NULL;

    VERIFY_NULL(handleInfo);

    if(abs(depth) > RBUS_MAX_NAME_DEPTH)
    {
        RBUSLOG_ERROR("%s %s depth %d exceeds RBUS_MAX_NAME_DEPTH %d", __FUNCTION__, elemName, depth, RBUS_MAX_NAME_DEPTH);
        return RBUS_ERROR_INVALID_INPUT;
    }

    err = rbus_discoverElementObjects(elemName, &numDestinations, &destinations);
    if (RBUSCORE_SUCCESS != err)
    {
        RBUSLOG_ERROR("%s rbus_discoverElementObjects %s failed: err=%d", __FUNCTION__, elemName, err);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if (numDestinations == 0)
    {
        RBUSLOG_ERROR("%s rbus_discoverElementObjects %s found 0 destinations", __FUNCTION__, elemName);
    }

    for(d = 0; d < numDestinations; d++)
    {
        rbusMessage_Init(&request);
        rbusMessage_SetString(request, elemName);
        rbusMessage_SetInt32(request, depth);/*depth*/
        rbusMessage_SetInt32(request, 0);/*not row names*/

        if((err = rbus_invokeRemoteMethod(destinations[d], METHOD_GETPARAMETERNAMES, request, rbusConfig_ReadGetTimeout(), &response)) != RBUSCORE_SUCCESS)
        {
            RBUSLOG_ERROR("%s rbus_invokeRemoteMethod %s destination=%s object=%s failed: err=%d", __FUNCTION__, METHOD_GETPARAMETERNAMES, destinations[d], elemName, err);
            errorcode = rbusCoreError_to_rbusError(err);
        }
        else
        {
            int providerErr = 0;

            RBUSLOG_DEBUG("%s rbus_invokeRemoteMethod %s destination=%s object=%s success", __FUNCTION__, METHOD_GETPARAMETERNAMES, destinations[d], elemName);

            rbusMessage_GetInt32(response, &providerErr);
            errorcode = providerErr < (int)RBUS_LEGACY_ERR_SUCCESS ? (rbusError_t)providerErr :CCSPError_to_rbusError((rbusLegacyReturn_t)providerErr);
            if(errorcode == RBUS_ERROR_SUCCESS)
            {
                int startIndex = runningCount;
                int count = 0;
                int i;

                rbusMessage_GetInt32(response, &count);
                RBUSLOG_DEBUG("%s rbus_invokeRemoteMethod %s %s success: count=%d", __FUNCTION__, METHOD_GETPARAMETERNAMES, elemName, count);
                if(count > 0)
                {
                    runningCount += count;
                    if(*elemInfo == NULL)
                        *elemInfo = rt_try_malloc(runningCount * sizeof(rbusElementInfo_t));
                    else
                        *elemInfo = rt_try_realloc(*elemInfo, runningCount * sizeof(rbusElementInfo_t));
                    if(!*elemInfo)
                    {
                        RBUSLOG_ERROR("%s failed to malloc %d element infos", __FUNCTION__, runningCount);
                        return RBUS_ERROR_OUT_OF_RESOURCES;
                    }
                    for(i = 0; i < runningCount-1; ++i)
                       (*elemInfo)[i].next = &(*elemInfo)[i+1];
                    (*elemInfo)[runningCount-1].next = NULL;
                }

                for(i = startIndex; i < runningCount; ++i)
                {
                    rbusMessage_GetString(response, (char const**)&(*elemInfo)[i].name);
                    rbusMessage_GetInt32(response, (int32_t*)&(*elemInfo)[i].type);
                    rbusMessage_GetInt32(response, (int32_t*)&(*elemInfo)[i].access);
                    (*elemInfo)[i].name = strdup((*elemInfo)[i].name);
                    (*elemInfo)[i].component = strdup(destinations[d]);
                    RBUSLOG_DEBUG("%s adding name %s", __FUNCTION__, (*elemInfo)[i].name);
                }
            }
            else
            {
                char const* providerErrMsg = NULL;
                rbusMessage_GetString(response, &providerErrMsg);
                RBUSLOG_ERROR("%s rbus_invokeRemoteMethod %s %s got provider error:%d reason:%s", __FUNCTION__, METHOD_GETPARAMETERNAMES, elemName, providerErr, providerErrMsg);
            }
            rbusMessage_Release(response);
        }
    }

    for(d = 0; d < numDestinations; d++)
        free(destinations[d]);
    free(destinations);

    return errorcode;
}

rbusError_t rbusElementInfo_free(
    rbusHandle_t handle, 
    rbusElementInfo_t* elemInfo)
{
    VERIFY_NULL(handle);
    if(elemInfo)
    {
        rbusElementInfo_t* elem = elemInfo;
        while(elem)
        {
            if(elem->name)
                free((char*)elem->name);
            if(elem->component)
                free((char*)elem->component);
            elem = elem->next;
        }
        free(elemInfo);
    }
    return RBUS_ERROR_SUCCESS;
}

//************************** Events ****************************//

static rbusMessage rbusEvent_CreateSubscribePayload(rbusEventSubscription_t* sub, int32_t componentId)
{
    rbusMessage payload = NULL;

    rbusMessage_Init(&payload);

    rbusMessage_SetInt32(payload, componentId);
    rbusMessage_SetInt32(payload, sub->interval);
    rbusMessage_SetInt32(payload, sub->duration);

    if(sub->filter)
    {
        rbusMessage_SetInt32(payload, 1);
        rbusFilter_AppendToMessage(sub->filter, payload);
    }
    else
    {
        rbusMessage_SetInt32(payload, 0);
    }

    return payload;
}

static rbusError_t rbusEvent_SubscribeWithRetries(
    rbusHandle_t                    handle,
    char const*                     eventName,
    rbusEventHandler_t              handler,
    void*                           userData,
    rbusFilter_t                    filter,
    uint32_t                        interval,
    uint32_t                        duration,    
    int                             timeout,
    rbusSubscribeAsyncRespHandler_t async,
    bool                            publishOnSubscribe)
{
    rbusCoreError_t coreerr;
    int providerError = RBUS_ERROR_SUCCESS;
    rbusEventSubscription_t* sub;
    rbusMessage payload = NULL;
    rbusMessage response = NULL;
    int destNotFoundSleep = 1000; /*miliseconds*/
    int destNotFoundTimeout;
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;

    if ((rbusEventSubscription_find(handleInfo->eventSubs, eventName, filter, interval, duration))||
            (rbusAsyncSubscribe_GetSubscription(handle, eventName, filter)))
    {
        return RBUS_ERROR_SUBSCRIPTION_ALREADY_EXIST;
    }

    if(timeout == -1)
    {
        destNotFoundTimeout = rbusConfig_Get()->subscribeTimeout;
    }
    else
    {
        destNotFoundTimeout = timeout * 1000; /*convert seconds to milliseconds */
    }

    sub = rt_malloc(sizeof(rbusEventSubscription_t));

    sub->handle = handle;
    sub->eventName = strdup(eventName);
    sub->handler = handler;
    sub->userData = userData;
    sub->filter = filter;
    sub->duration = duration;
    sub->interval = interval;
    sub->asyncHandler = async;

    if(sub->filter)
        rbusFilter_Retain(sub->filter);


    payload = rbusEvent_CreateSubscribePayload(sub, handleInfo->componentId);

    if(sub->asyncHandler)
    {
        //FIXME: this should take the payload too (mrollins) because rbus_asynsubscribe is passing NULL for filter to rbus_subscribeToEvent
        rbusAsyncSubscribe_AddSubscription(sub, payload);

        rbusMessage_Release(payload);
        return RBUS_ERROR_SUCCESS;
    }

    for(;;)
    {
        RBUSLOG_DEBUG("%s: %s subscribing", __FUNCTION__, eventName);

        coreerr = rbus_subscribeToEventTimeout(NULL, sub->eventName, _event_callback_handler, payload, sub, &providerError, destNotFoundTimeout, publishOnSubscribe, &response);
        
        if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE && destNotFoundTimeout > 0)
        {
            int sleepTime = destNotFoundSleep;

            if(sleepTime > destNotFoundTimeout)
                sleepTime = destNotFoundTimeout;

            RBUSLOG_DEBUG("%s: %s no provider. retry in %d ms with %d left", __FUNCTION__, eventName, sleepTime, destNotFoundTimeout );

            //TODO: do we need pthread_cond_timedwait ?  e.g. maybe another thread calls rbus_close and we need to shutdown
            sleep(sleepTime/1000);

            destNotFoundTimeout -= destNotFoundSleep;

            //double the wait time
            destNotFoundSleep *= 2;

            //cap it so the wait time still allows frequent retries
            if(destNotFoundSleep > rbusConfig_Get()->subscribeMaxWait)
                destNotFoundSleep = rbusConfig_Get()->subscribeMaxWait;
        }
        else
        {
            break;
        }
    }

    if(payload)
    {
        rbusMessage_Release(payload);
    }

    if(coreerr == RBUSCORE_SUCCESS)
    {
        int initial_value = 0;
        
        rtVector_PushBack(handleInfo->eventSubs, sub);

        if(publishOnSubscribe)
        {
            rbusMessage_GetInt32(response, &initial_value);
            if(initial_value)
                _master_event_callback_handler(NULL, eventName, response, userData);
        }
        if(response)
            rbusMessage_Release(response);
        RBUSLOG_INFO("%s: %s subscribe retries succeeded", __FUNCTION__, eventName);
        return RBUS_ERROR_SUCCESS;
    }
    else
    {
        rbusEventSubscription_free(sub);

        if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE)
        {
            RBUSLOG_DEBUG("%s: %s all subscribe retries failed because no provider could be found", __FUNCTION__, eventName);
            RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_NO_PROVIDER_COMPONENT  %s", eventName);/*RDKB-33658-AC7*/
            return RBUS_ERROR_TIMEOUT;
        }
        else if(providerError != RBUS_ERROR_SUCCESS)
        {   
            RBUSLOG_DEBUG("%s: %s subscribe retries failed due provider error %d", __FUNCTION__, eventName, providerError);
            RBUSLOG_WARN("EVENT_SUBSCRIPTION_FAIL_INVALID_INPUT  %s", eventName);/*RDKB-33658-AC9*/
            return providerError;
        }
        else
        {
            RBUSLOG_WARN("%s: %s subscribe retries failed due to core error %d", __FUNCTION__, eventName, coreerr);
            return RBUS_ERROR_BUS_ERROR;
        }
    }
}

rbusError_t  rbusEvent_Subscribe(
    rbusHandle_t        handle,
    char const*         eventName,
    rbusEventHandler_t  handler,
    void*               userData,
    int                 timeout)
{
    rbusError_t errorcode;

    VERIFY_NULL(handle);
    VERIFY_NULL(eventName);
    VERIFY_NULL(handler);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, eventName);

    errorcode = rbusEvent_SubscribeWithRetries(handle, eventName, handler, userData, NULL, 0, 0 , timeout, NULL, false);

    return errorcode;
}

rbusError_t  rbusEvent_SubscribeAsync(
    rbusHandle_t                    handle,
    char const*                     eventName,
    rbusEventHandler_t              handler,
    rbusSubscribeAsyncRespHandler_t subscribeHandler,
    void*                           userData,
    int                             timeout)
{
    rbusError_t errorcode;

    VERIFY_NULL(handle);
    VERIFY_NULL(eventName);
    VERIFY_NULL(handler);
    VERIFY_NULL(subscribeHandler);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, eventName);

    errorcode = rbusEvent_SubscribeWithRetries(handle, eventName, handler, userData, NULL, 0, 0, timeout, subscribeHandler, false);

    return errorcode;
}

rbusError_t rbusEvent_Unsubscribe(
    rbusHandle_t        handle,
    char const*         eventName)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusEventSubscription_t* sub;

    VERIFY_NULL(handle);
    VERIFY_NULL(eventName);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, eventName);

    /*the use of rtVector is inefficient here.  I have to loop through the vector to find the sub by name, 
        then call RemoveItem, which loops through again to find the item by address to destroy */
    sub = rbusEventSubscription_find(handleInfo->eventSubs, eventName, NULL, 0, 0);

    if(sub)
    {
        rbusMessage payload = rbusEvent_CreateSubscribePayload(sub, handleInfo->componentId);

        rbusCoreError_t coreerr = rbus_unsubscribeFromEvent(NULL, eventName, payload);

        if(payload)
        {
            rbusMessage_Release(payload);
        }

        rtVector_RemoveItem(handleInfo->eventSubs, sub, rbusEventSubscription_free);

        if(coreerr == RBUSCORE_SUCCESS)
        {
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            RBUSLOG_INFO("%s: %s failed with core err=%d", __FUNCTION__, eventName, coreerr);
            
            if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE)
            {
                return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
            }
            else
            {
                return RBUS_ERROR_BUS_ERROR;
            }
        }
    }
    else
    {
        RBUSLOG_INFO("%s: %s no existing subscription found", __FUNCTION__, eventName);
        return RBUS_ERROR_INVALID_OPERATION; //TODO - is the the right error to return
    }
}

rbusError_t rbusEvent_SubscribeEx(
    rbusHandle_t                handle,
    rbusEventSubscription_t*    subscription,
    int                         numSubscriptions,
    int                         timeout)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    int i;

    VERIFY_NULL(handle);
    VERIFY_NULL(subscription);
    VERIFY_ZERO(numSubscriptions); 

    for(i = 0; i < numSubscriptions; ++i)
    {
        RBUSLOG_DEBUG ("%s: %s", __FUNCTION__, subscription[i].eventName);

        //FIXME/TODO -- since this is not using async path, this could block and thus block the rest of the subs to come
        //For rbusEvent_Subscribe, since it a single subscribe, blocking is fine but for rbusEvent_SubscribeEx,
        //where we can have multiple, we need to actually run all these in parallel.  So we might need to leverage
        //the asyncsubscribe api to handle this.
        errorcode = rbusEvent_SubscribeWithRetries(
            handle, subscription[i].eventName, subscription[i].handler, subscription[i].userData, 
            subscription[i].filter, subscription[i].interval, subscription[i].duration, timeout, NULL, subscription[i].publishOnSubscribe);

        if(errorcode != RBUS_ERROR_SUCCESS)
        {
            /*  Treat SubscribeEx like a transaction because
                if any subs fails, how will the user know which ones succeeded and which failed ?
                So, as a transaction, we just undo everything, which are all those from 0 to i-1.
            */
            if(i > 0)
                rbusEvent_UnsubscribeEx(handle, subscription, i);
            break;
        }
    }

    return errorcode;
}

rbusError_t rbusEvent_SubscribeExAsync(
    rbusHandle_t                    handle,
    rbusEventSubscription_t*        subscription,
    int                             numSubscriptions,
    rbusSubscribeAsyncRespHandler_t subscribeHandler,
    int                             timeout)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;
    int i;

    VERIFY_NULL(handle);
    VERIFY_NULL(subscription);
    VERIFY_NULL(subscribeHandler);
    VERIFY_ZERO(numSubscriptions);

    for(i = 0; i < numSubscriptions; ++i)
    {
        RBUSLOG_INFO("%s: %s", __FUNCTION__, subscription[i].eventName);

        errorcode = rbusEvent_SubscribeWithRetries(
            handle, subscription[i].eventName, subscription[i].handler, subscription[i].userData, 
            subscription[i].filter, subscription[i].interval, subscription[i].duration, timeout, subscribeHandler, false);

        if(errorcode != RBUS_ERROR_SUCCESS)
        {
            RBUSLOG_WARN("%s: %s failed err=%d", __FUNCTION__, subscription[i].eventName, errorcode);

            /*  Treat SubscribeEx like a transaction because
                if any subs fails, how will the user know which ones succeeded and which failed ?
                So, as a transaction, we just undo everything, which are all those from 0 to i-1.
            */
            if(i > 0)
                rbusEvent_UnsubscribeEx(handle, subscription, i);
            break;
        }
    }

    return errorcode;    
}

rbusError_t rbusEvent_UnsubscribeEx(
    rbusHandle_t                handle,
    rbusEventSubscription_t*    subscription,
    int                         numSubscriptions)
{
    rbusError_t errorcode = RBUS_ERROR_SUCCESS;

    VERIFY_NULL(handle);
    VERIFY_NULL(subscription);
    VERIFY_ZERO(numSubscriptions);

    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    int i;

    //TODO we will call unsubscribe for every sub in list
    //if any unsubscribe fails below we use RBUS_ERROR_BUS_ERROR for return error
    //The caller will have no idea which ones failed to unsub and which succeeded (if any)
    //and unlike SubscribeEx, I don't think we can treat this like a transactions because
    //its assumed that caller has successfully subscribed before so we need to attempt all 
    //to get as many as possible unsubscribed and off the bus

    for(i = 0; i < numSubscriptions; ++i)
    {
        rbusEventSubscription_t* sub;

        RBUSLOG_INFO("%s: %s", __FUNCTION__, subscription[i].eventName);

        /*the use of rtVector is inefficient here.  I have to loop through the vector to find the sub by name, 
            then call RemoveItem, which loops through again to find the item by address to destroy */
        sub = rbusEventSubscription_find(handleInfo->eventSubs, subscription[i].eventName, subscription[i].filter, subscription[i].interval, subscription[i].duration);
        if(sub)
        {
            rbusCoreError_t coreerr;
            rbusMessage payload;

            payload = rbusEvent_CreateSubscribePayload(sub, handleInfo->componentId);

            coreerr = rbus_unsubscribeFromEvent(NULL, sub->eventName, payload);

            if(payload)
            {
                rbusMessage_Release(payload);
            }

            rtVector_RemoveItem(handleInfo->eventSubs, sub, rbusEventSubscription_free);

            if(coreerr != RBUSCORE_SUCCESS)
            {
                RBUSLOG_INFO("%s: failed with core err=%d", __FUNCTION__, coreerr);
                
                //FIXME -- we just overwrite any existing error that might have happened in a previous loop
                if(coreerr == RBUSCORE_ERROR_DESTINATION_UNREACHABLE)
                {
                    errorcode = RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
                }
                else
                {
                    errorcode = RBUS_ERROR_BUS_ERROR;
                }
            }
        }
        else
        {
            rbusEventSubscription_t* sub = rbusAsyncSubscribe_GetSubscription(handle, subscription[i].eventName, subscription[i].filter);
            if(sub)
            {
                RBUSLOG_INFO("%s: %s removed pending async subscription", __FUNCTION__, sub->eventName);
                rbusAsyncSubscribe_RemoveSubscription(sub);
            }
            else
            {
                RBUSLOG_INFO("%s: %s no existing subscription found", __FUNCTION__, subscription[i].eventName);
                errorcode = RBUS_ERROR_INVALID_OPERATION; //TODO - is the the right error to return
            }
        }
    }

    return errorcode;
}

rbusError_t  rbusEvent_Publish(
  rbusHandle_t          handle,
  rbusEvent_t*          eventData)
{
    struct _rbusHandle* handleInfo = (struct _rbusHandle*)handle;
    rbusCoreError_t err, errOut = RBUSCORE_SUCCESS;
    rtListItem listItem;
    rbusSubscription_t* subscription;
    rbusValue_t newVal = NULL;
    rbusValue_t oldVal = NULL;

    VERIFY_NULL(handle);
    VERIFY_NULL(eventData);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, eventData->name);

    /*get the node and walk its subscriber list, 
      publishing event to each subscriber*/
    elementNode* el = retrieveInstanceElement(handleInfo->elementRoot, eventData->name);

    if(!el)
    {
        RBUSLOG_WARN("rbusEvent_Publish failed: retrieveElement return NULL for %s", eventData->name);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    if(!el->subscriptions)/*nobody subscribed yet*/
    {
        return RBUS_ERROR_NOSUBSCRIBERS;
    }

    if(eventData->type == RBUS_EVENT_VALUE_CHANGED)
    {
        if(eventData->data)
        {
            newVal = rbusObject_GetValue(eventData->data, "value");
            oldVal = rbusObject_GetValue(eventData->data, "oldValue");
        }
        if(!eventData->data || !newVal || !oldVal)
        {
            RBUSLOG_ERROR("%s: missing value data for value change event %s", __FUNCTION__, eventData->name);
            return RBUS_ERROR_INVALID_INPUT;
        }
    }

    /*Loop through element's subscriptions*/
    rtList_GetFront(el->subscriptions, &listItem);
    while(listItem)
    {
        bool publish = true;

        rtListItem_GetData(listItem, (void**)&subscription);
        if(!subscription || !subscription->eventName || !subscription->listener)
        {
            RBUSLOG_INFO("rbusEvent_Publish failed: null subscriber data");
            if(errOut == RBUSCORE_SUCCESS)
                errOut = RBUSCORE_ERROR_GENERAL;
            rtListItem_GetNext(listItem, &listItem);
        }

        if(eventData->type == RBUS_EVENT_VALUE_CHANGED)
        {

            /* if the subscriber has a filter we check the filter to determine if we publish to them.
            if the subscriber does not have a filter, we publish always to them*/
            if(subscription->filter)
            {
                /*We publish an event only when the value crosses the filter threshold boundary.
                When the value crosses into the threshold we publish a single event signally the filter started matching.
                When the value crosses out of the threshold we publish a single event signally the filter stopped matching.
                We do not publish continuous events while the filter continues to match. The consumer can read the 'filter'
                property from the event data to determine if the filter has started or stopped matching.  If the consumer
                wants to get continuous value-change events, they can unsubscribe the filter and resubscribe without a filter*/

                int newResult = rbusFilter_Apply(subscription->filter, newVal);
                int oldResult = rbusFilter_Apply(subscription->filter, oldVal);

                if(newResult != oldResult)
                {
                    /*set 'filter' to true/false implying that either the filter has started or stopped matching*/
                    rbusValue_t filterResult = NULL;
                    rbusValue_Init(&filterResult);
                    rbusValue_SetBoolean(filterResult, newResult != 0);
                    rbusObject_SetValue(eventData->data, "filter", filterResult);
                    rbusValue_Release(filterResult);                    
                }
                else
                {
                    publish =  false;
                }
            }
        }

        if(publish)
        {
            rbusMessage msg;
            rbusMessage_Init(&msg);

            rbusEventData_appendToMessage(eventData, subscription->filter, subscription->interval, subscription->duration, subscription->componentId, msg);

            RBUSLOG_DEBUG("rbusEvent_Publish: publishing event %s to listener %s", subscription->eventName, subscription->listener);

            err = rbus_publishSubscriberEvent(
                handleInfo->componentName,  
                subscription->eventName/*use the same eventName the consumer subscribed with; not event instance name eventData->name*/, 
                subscription->listener, 
                msg);

            rbusMessage_Release(msg);

            if(err != RBUSCORE_SUCCESS)
            {
                if(errOut == RBUSCORE_SUCCESS)
                    errOut = err;
                RBUSLOG_INFO("rbusEvent_Publish failed: rbus_publishSubscriberEvent return error %d", err);
            }
        }   

        rtListItem_GetNext(listItem, &listItem);
    }

    return errOut == RBUSCORE_SUCCESS ? RBUS_ERROR_SUCCESS: RBUS_ERROR_BUS_ERROR;
}

rbusError_t rbusMethod_InvokeInternal(
    rbusHandle_t handle, 
    char const* methodName, 
    rbusObject_t inParams, 
    rbusObject_t* outParams,
    int timeout)
{
    (void)handle;
    rbusCoreError_t err;
    int returnCode = RBUS_ERROR_INVALID_INPUT;
    rbusMessage request, response;
    rbusLegacyReturn_t legacyRetCode = RBUS_LEGACY_ERR_FAILURE;
    rbusValue_t value1 = NULL, value2 = NULL;

    VERIFY_NULL(handle);
    VERIFY_NULL(methodName);

    RBUSLOG_DEBUG("%s: %s", __FUNCTION__, methodName);

    rbusMessage_Init(&request);
    rbusMessage_SetInt32(request, 0);/*TODO: this should be the session ID*/
    rbusMessage_SetString(request, methodName); /*TODO: do we need to append the name as well as pass the name as the 1st arg to rbus_invokeRemoteMethod ?*/

    if(inParams)
        rbusObject_appendToMessage(inParams, request);

    if((err = rbus_invokeRemoteMethod(
        methodName,
        METHOD_RPC, 
        request, 
        timeout, 
        &response)) != RBUSCORE_SUCCESS)
    {
        RBUSLOG_ERROR("%s by %s failed; Received error %d from RBUS Daemon for the object %s", __FUNCTION__, handle->componentName, err, methodName);
        /* Updating the outParmas as RBUS core is returning failure */
        rbusObject_Init(outParams, NULL);
        rbusValue_Init(&value1);
        rbusValue_Init(&value2);

        rbusValue_SetInt32(value1, rbusCoreError_to_rbusError(err));
        rbusValue_SetString(value2, rbusError_ToString(rbusCoreError_to_rbusError(err)));
        rbusObject_SetValue(*outParams, "error_code", value1);
        rbusObject_SetValue(*outParams, "error_string", value2);
        rbusValue_Release(value1);
        rbusValue_Release(value2);
        return rbusCoreError_to_rbusError(err);
    }

    rbusMessage_GetInt32(response, &returnCode);
    legacyRetCode = (rbusLegacyReturn_t)returnCode;

    rbusObject_initFromMessage(outParams, response);
    if(legacyRetCode > RBUS_LEGACY_ERR_SUCCESS)
    {
        returnCode = CCSPError_to_rbusError(legacyRetCode);
    }

    rbusMessage_Release(response);

    RBUSLOG_INFO("%s rbus_invokeRemoteMethod success response returnCode:%d", __FUNCTION__, returnCode);

    return returnCode;
}

rbusError_t rbusMethod_Invoke(
    rbusHandle_t handle, 
    char const* methodName, 
    rbusObject_t inParams, 
    rbusObject_t* outParams)
{
    VERIFY_NULL(handle);
    VERIFY_NULL(methodName);
    rbusCheckAsyncInvalid(handle);
    return rbusMethod_InvokeInternal(handle, methodName, inParams, outParams, rbusConfig_ReadSetTimeout());
}

static inline void rbusAsyncRequest_SetCompleted(rbusAsyncRequest_t req, bool b) {
  req->completed = b;
}

static void rbusMethodCompletionHandlerImpl(rbusMessage res, rbusCoreError_t status, void* user_data);
static void rbusMethodCompletionHandlerAdapter(void* user_data) {
  struct rbusEventLoopCallbackContext* ctx = (struct rbusEventLoopCallbackContext *) user_data;
  rbusMethodCompletionHandlerImpl(ctx->res, ctx->status, ctx->req);
  rbusMessage_Release(ctx->res);
  rt_free(ctx);
}
static int rbusMethodCompletionHandler(rbusMessage res, rbusCoreError_t status, void* user_data)
{
  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;
  if (req->rbus->useEventLoop)
    rbusEventLoop_EnqueueResponse(req->rbus, res, status, req, &rbusMethodCompletionHandlerAdapter);
  else
    rbusMethodCompletionHandlerImpl(res, status, user_data);
  return 0;
}

void rbusMethodCompletionHandlerImpl(rbusMessage res, rbusCoreError_t core_status, void* user_data)
{
  rbusError_t rbus_status = rbusCoreTranslateStatusFromResponse(res, core_status);
  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;

  rbusObject_t out_params = NULL;
  if (core_status == RBUSCORE_SUCCESS)
    rbusObject_initFromMessage(&out_params, res);
  if (req->method_handler)
    req->method_handler(req->rbus, req->method_name, rbus_status, NULL);
  else {
    struct rbusAsyncResponse rbus_res;
    if (out_params) {
      rbus_res.props = rbusObject_GetProperties(out_params);
      rbusProperty_Retain(rbus_res.props);
    }
    else
      rbus_res.props = NULL;
    rbus_res.status = rbus_status;
    rbus_res.user_data = req->user_data;
    req->completion_handler(req->rbus, &rbus_res);
    if (rbus_res.props)
      rbusProperty_Release(rbus_res.props);
  }
  rbusAsyncRequest_SetCompleted(req, true);
  rbusAsyncRequest_Release(req);
  rbusObject_Release(out_params);
}

rbusError_t rbusMethod_InvokeAsync(
    rbusHandle_t                      rbus,
    char const*                       method_name,
    rbusObject_t                      method_params,
    rbusMethodAsyncRespHandler_t      method_handler,
    int                               timeout_millis)
{
  rbusError_t err;

  rbusCheckNotNull(rbus);
  rbusCheckNotNull(method_name);
  rbusCheckNotNull(method_handler);

  rbusAsyncRequest_t req = rbusAsyncRequest_New();
  req->rbus = rbus;
  rbusAsyncRequest_SetMethodName(req, method_name);
  rbusAsyncRequest_SetMethodParameters(req, rbusObject_GetProperties(method_params));
  req->method_handler = req->method_handler;
  rbusAsyncRequest_SetTimeout(req, timeout_millis);
  err = rbusMethod_InvokeAsyncEx(rbus, req);
  rbusAsyncRequest_SetCompleted(req, true);
  rbusAsyncRequest_Release(req);
  return err;
}

rbusError_t rbusMethod_InvokeAsyncEx(rbusHandle_t rbus, rbusAsyncRequest_t req)
{
  rbusMessage rbus_req;
  rbusMessage_Init(&rbus_req);
  rbusMessage_SetInt32(rbus_req, 0);
  rbusMessage_SetString(rbus_req, req->method_name);
  if (req->method_params) {
    rbusMessage_SetInt32(rbus_req, 1);
    rbusObject_appendToMessage(req->method_params, rbus_req);
  }
  else
    rbusMessage_SetInt32(rbus_req, 0);

  rbusAsyncRequest_Retain(req);
  req->rbus = rbus;

  rbusCoreError_t err = rbus_invokeRemoteMethodAsync(
    req->method_name,
    METHOD_RPC,
    rbus_req,
    req->timeout_millis,
    &rbusMethodCompletionHandler,
    req,
    &req->rt_request_id);

  rbusMessage_Release(rbus_req);

  return rbusCoreError_to_rbusError(err);
}

rbusError_t rbusMethod_SendAsyncResponse(
    rbusMethodAsyncHandle_t asyncHandle,
    rbusError_t error,
    rbusObject_t outParams)
{
    rbusMessage response;

    VERIFY_NULL(asyncHandle);
    rbusValue_t value1, value2;

    rbusValue_Init(&value1);
    rbusValue_Init(&value2);

    rbusMessage_Init(&response);
    rbusMessage_SetInt32(response, error);
    if ((error != RBUS_ERROR_SUCCESS) && (outParams == NULL))
    {
        rbusObject_Init(&outParams, NULL);
        rbusValue_SetInt32(value1, error);
        rbusValue_SetString(value2, rbusError_ToString(error));
        rbusObject_SetValue(outParams, "error_code", value1);
        rbusObject_SetValue(outParams, "error_string", value2);
    }
    rbusObject_appendToMessage(outParams, response);
    rbus_sendResponse(&asyncHandle->hdr, response);
    rbusValue_Release(value1);
    rbusValue_Release(value2);
    free(asyncHandle);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbus_createSession(rbusHandle_t handle, uint32_t *pSessionId)
{
    (void)handle;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response  = NULL;
    if (pSessionId && handle)
    {
        *pSessionId = 0;
        if((err = rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_REQUEST_SESSION_ID, NULL, rbusConfig_ReadSetTimeout(), &response)) == RBUSCORE_SUCCESS)
        {
            rbusMessage_GetInt32(response, /*MESSAGE_FIELD_RESULT,*/ (int*) &err);
            if(RBUSCORE_SUCCESS != err)
            {
                RBUSLOG_ERROR("Session manager reports internal error %d for the object %s", err, RBUS_SMGR_DESTINATION_NAME);
                rc = RBUS_ERROR_SESSION_ALREADY_EXIST;
            }
            else
            {
                rbusMessage_GetInt32(response, /*MESSAGE_FIELD_PAYLOAD,*/ (int*) pSessionId);
                RBUSLOG_INFO("Received new session id %u", *pSessionId);
            }
        }
        else
        {
            RBUSLOG_ERROR("Failed to communicated with session manager.");
            rc = rbusCoreError_to_rbusError(err);
        }
	rbusMessage_Release(response);
    }
    else
    {
        RBUSLOG_WARN("Invalid Input passed..");
        rc = RBUS_ERROR_INVALID_INPUT;
    }
    return rc;
}

rbusError_t rbus_getCurrentSession(rbusHandle_t handle, uint32_t *pSessionId)
{
    (void)handle;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;
    rbusMessage response = NULL;

    if (pSessionId && handle)
    {
        *pSessionId = 0;
        if((err = rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID, NULL, rbusConfig_ReadGetTimeout(), &response)) == RBUSCORE_SUCCESS)
        {
            rbusMessage_GetInt32(response, /*MESSAGE_FIELD_RESULT,*/ (int*) &err);
            if(RBUSCORE_SUCCESS != err)
            {
                RBUSLOG_ERROR("Session manager reports internal error %d from %s for the object %s", err, handle->componentName, RBUS_SMGR_DESTINATION_NAME);
                rc = RBUS_ERROR_SESSION_ALREADY_EXIST;
            }
            else
            {
                rbusMessage_GetInt32(response, /*MESSAGE_FIELD_PAYLOAD,*/ (int*) pSessionId);
                RBUSLOG_INFO("Received new session id %u", *pSessionId);
            }
        }
        else
        {
            RBUSLOG_ERROR("Failed to communicated with session manager.");
            rc = rbusCoreError_to_rbusError(err);
        }
	rbusMessage_Release(response);
    }
    else
    {
        RBUSLOG_WARN("Invalid Input passed..");
        rc = RBUS_ERROR_INVALID_INPUT;
    }
    return rc;
}

rbusError_t rbus_closeSession(rbusHandle_t handle, uint32_t sessionId)
{
    (void)handle;
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusCoreError_t err = RBUSCORE_SUCCESS;

    if (handle)
    {
        rbusMessage inputSession;
        rbusMessage response = NULL;

        if (sessionId == 0)
        {
            RBUSLOG_WARN("Passing default session ID which is 0");
            return RBUS_ERROR_SUCCESS;
        }
        rbusMessage_Init(&inputSession);
        rbusMessage_SetInt32(inputSession, /*MESSAGE_FIELD_PAYLOAD,*/ sessionId);
        if((err = rbus_invokeRemoteMethod(RBUS_SMGR_DESTINATION_NAME, RBUS_SMGR_METHOD_END_SESSION, inputSession, rbusConfig_ReadSetTimeout(), &response)) == RBUSCORE_SUCCESS)
        {
            rbusMessage_GetInt32(response, /*MESSAGE_FIELD_RESULT,*/ (int*) &err);
            if(RBUSCORE_SUCCESS != err)
            {
                RBUSLOG_ERROR("Session manager reports internal error %d from %s for the object %s", err, handle->componentName, RBUS_SMGR_DESTINATION_NAME);
                rc = RBUS_ERROR_SESSION_ALREADY_EXIST;
            }
            else
                RBUSLOG_INFO("Successfully ended session %u.", sessionId);
        }
        else
        {
            RBUSLOG_ERROR("Failed to communicated with session manager.");
            rc = rbusCoreError_to_rbusError(err);
        }
        rbusMessage_Release(response);
    }
    else
    {
        RBUSLOG_WARN("Invalid Input passed..");
        rc = RBUS_ERROR_INVALID_INPUT;
    }

    return rc;
}

rbusStatus_t rbus_checkStatus(void)
{
    rbuscore_bus_status_t busStatus = rbuscore_checkBusStatus();

    return (rbusStatus_t) busStatus;
}

rbusError_t rbus_registerLogHandler(rbusLogHandler logHandler)
{
    if (logHandler)
    {
        rtLogSetLogHandler ((rtLogHandler) logHandler);
        return RBUS_ERROR_SUCCESS;
    }
    else
    {
        RBUSLOG_WARN("Invalid Input passed..");
        return RBUS_ERROR_INVALID_INPUT;
    }
}

rbusError_t rbus_setLogLevel(rbusLogLevel_t level)
{
    if (level <= RBUS_LOG_FATAL)
    {
        rtLog_SetLevel((rtLogLevel) level);
        return RBUS_ERROR_SUCCESS;
    }
    else
        return RBUS_ERROR_INVALID_INPUT;
}

char const*
rbusError_ToString(rbusError_t e)
{

#define rbusError_String(E, S) case E: s = S; break;

  char const * s = NULL;
  switch (e)
  {
    rbusError_String(RBUS_ERROR_SUCCESS, "ok");
    rbusError_String(RBUS_ERROR_BUS_ERROR, "generic error");
    rbusError_String(RBUS_ERROR_INVALID_INPUT, "invalid input");
    rbusError_String(RBUS_ERROR_NOT_INITIALIZED, "not initialized");
    rbusError_String(RBUS_ERROR_OUT_OF_RESOURCES, "out of resources");
    rbusError_String(RBUS_ERROR_DESTINATION_NOT_FOUND, "destination not found");
    rbusError_String(RBUS_ERROR_DESTINATION_NOT_REACHABLE, "destination not reachable");
    rbusError_String(RBUS_ERROR_DESTINATION_RESPONSE_FAILURE, "destination response failure");
    rbusError_String(RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION, "invalid response from destination");
    rbusError_String(RBUS_ERROR_INVALID_OPERATION, "invalid operation");
    rbusError_String(RBUS_ERROR_INVALID_EVENT, "invalid event");
    rbusError_String(RBUS_ERROR_INVALID_HANDLE, "invalid handle");
    rbusError_String(RBUS_ERROR_SESSION_ALREADY_EXIST, "session already exists");
    rbusError_String(RBUS_ERROR_COMPONENT_NAME_DUPLICATE, "duplicate component name");
    rbusError_String(RBUS_ERROR_ELEMENT_NAME_DUPLICATE, "duplicate element name");
    rbusError_String(RBUS_ERROR_ELEMENT_NAME_MISSING, "name missing");
    rbusError_String(RBUS_ERROR_COMPONENT_DOES_NOT_EXIST, "component does not exist");
    rbusError_String(RBUS_ERROR_ELEMENT_DOES_NOT_EXIST, "element name does not exist");
    rbusError_String(RBUS_ERROR_ACCESS_NOT_ALLOWED, "access denied");
    rbusError_String(RBUS_ERROR_INVALID_CONTEXT, "invalid context");
    rbusError_String(RBUS_ERROR_TIMEOUT, "timeout");
    rbusError_String(RBUS_ERROR_ASYNC_RESPONSE, "async operation in progress");
    default:
      s = "unknown error";
  }
  return s;
}

rbusError_t rbusHandle_ClearTraceContext(
    rbusHandle_t  rbus)
{
    if (!rbus)
        return RBUS_ERROR_INVALID_HANDLE;

    rbus_clearOpenTelemetryContext();

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_SetTraceContextFromString(
    rbusHandle_t  rbus,
    char const*   traceParent,
    char const*   traceState)
{
    if (!rbus)
      return RBUS_ERROR_INVALID_HANDLE;

    rbus_setOpenTelemetryContext(traceParent, traceState);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusHandle_GetTraceContextAsString(
    rbusHandle_t  rbus,
    char*         traceParent,
    int           traceParentLength,
    char*         traceState,
    int           traceStateLength)
{
    if (!rbus)
      return RBUS_ERROR_INVALID_HANDLE;

    size_t n;
    char const *s = NULL;
    char const *t = NULL;

    rbus_getOpenTelemetryContext(&s, &t);

    if (traceParent)
    {
        if (s)
        {
            n = RBUS_MIN( (int) strlen(s), traceParentLength - 1 );
            strncpy(traceParent, s, n);
            traceParent[n] ='\0';
        }
        else
            traceParent[0] = '\0';
    }

    if (traceState)
    {
        if (t)
        {
            n = RBUS_MIN( (int) strlen(t), traceStateLength - 1);
            strncpy(traceState, t, n);
            traceState[n] = '\0';
        }
        else
            traceState[0] = '\0';
    }

    return RBUS_ERROR_SUCCESS;
}

void rbusRunnableQueue_Init(rbusRunnableQueue_t *q)
{
    q->head = NULL;
    q->tail = NULL;
    pipe2(q->pipe_fds, O_CLOEXEC);
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void rbusRunnableQueue_PushBack(rbusRunnableQueue_t *q, rbusRunnable_t r)
{
    rbusRunnable_t *item = rt_malloc(sizeof(rbusRunnable_t));
    item->argp = r.argp;
    item->exec = r.exec;
    item->cleanup = r.cleanup;
    item->next = NULL;
    item->id = r.id;

    pthread_mutex_lock(&q->mutex);
    if (!q->head)
    {
        q->head = item;
        q->tail = item;
    }
    else
    {
        q->tail->next = item;
        q->tail = item;
    }
    pthread_mutex_unlock(&q->mutex);

    static const char kQueueIndicator_Runnable = 'r';
    write(q->pipe_fds[1], &kQueueIndicator_Runnable, 1);
}

rbusRunnable_t *
rbusRunnableQueue_PopFront(rbusRunnableQueue_t *q)
{
    rbusRunnable_t *r = NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->head)
    {
        r = q->head;
        q->head = q->head->next;
    }
    pthread_mutex_unlock(&q->mutex);

    if (r)
    {
        char queue_indicator = 'z';
        read(q->pipe_fds[0], &queue_indicator, 1);
    }

    return r;
}

int
rbusHandle_GetEventFD(rbusHandle_t rbus)
{
    return rbus->eventQueue.pipe_fds[0];
}

rbusError_t
rbusHandle_Run(rbusHandle_t rbus)
{
  rbusRunnable_t* curr;
  rbusRunnable_t* next;

  // being very cautious about applications calling rbusHandle_Run from a callback
  // that's being invoked from this context.
  int depth = rtAtomicFetchAdd(&rbus->reentrancyGuard, 1);
  if (depth > 0) {
    RBUSLOG_ERROR("re-entrancy problem. rbusHandle_Run was called from Run or RunOne.");
    abort();
  }

  pthread_mutex_lock(&rbus->eventQueue.mutex);
  curr = rbus->eventQueue.head;
  rbus->eventQueue.head = NULL;
  rbus->eventQueue.tail = NULL;
  pthread_mutex_unlock(&rbus->eventQueue.mutex);

  while (curr) {
    char queue_indicator = 'z';
    read(rbus->eventQueue.pipe_fds[0], &queue_indicator, 1);

    curr->exec(curr->argp);

    if (curr->cleanup)
      curr->cleanup(curr->argp);

    next = curr->next;
    rt_free(curr);
    curr = next;
  }

  rtAtomicFetchSub(&rbus->reentrancyGuard, 1);

  return RBUS_ERROR_SUCCESS;
}

rbusError_t
rbusHandle_RunOne(rbusHandle_t rbus)
{
    // being very cautious about applications calling rbusHandle_Run from a callback
    // that's being invoked from this context.
    int depth = rtAtomicFetchAdd(&rbus->reentrancyGuard, 1);
    if (depth > 0) {
      RBUSLOG_ERROR("re-entrancy problem. rbusHandle_RunOne was called from Run or RunOne.");
      abort();
    }

    rbusRunnable_t* r = rbusRunnableQueue_PopFront(&rbus->eventQueue);
    if (r)
    {
        if (r->exec)
        {
            r->exec(r->argp);
        }
        if (r->cleanup)
        {
            r->cleanup(r->argp);
        }
        rt_free(r);
        rtAtomicFetchSub(&rbus->reentrancyGuard, 1);
        return RBUS_ERROR_SUCCESS;
    }

    rtAtomicFetchSub(&rbus->reentrancyGuard, 1);
    return RBUS_ERROR_TIMEOUT;
}

void rbusMessage_GetStatusCodes(rbusMessage m, rbusError_t* rbus_status, rbusLegacyReturn_t* legacy_status)
{
  int32_t n = -1;
  if (m) {
    rbusMessage_GetInt32(m, &n);
    if (rbus_status)
      *rbus_status = (rbusError_t) n;
    if (legacy_status)
      *legacy_status = (rbusLegacyReturn_t) n;
  }
  else {
    if (rbus_status)
      *rbus_status = RBUS_ERROR_BUS_ERROR;
    if (legacy_status)
      *legacy_status = RBUS_LEGACY_ERR_FAILURE;
  }
}

rbusError_t rbusCoreTranslateStatusFromResponse(rbusMessage m, rbusCoreError_t core_status)
{
  rbusError_t rbus_status = RBUS_ERROR_BUS_ERROR;

  if (core_status == RBUSCORE_SUCCESS) {
    rbusLegacyReturn_t legacy_status = RBUS_LEGACY_ERR_FAILURE;
    rbusMessage_GetStatusCodes(m, &rbus_status, &legacy_status);

    if ((rbus_status == RBUS_ERROR_SUCCESS) || (legacy_status == RBUS_LEGACY_ERR_SUCCESS)) {
      rbus_status = RBUS_ERROR_SUCCESS;
    }
    else {
      if (legacy_status > RBUS_LEGACY_ERR_SUCCESS)
        rbus_status = rbusCoreError_to_rbusError(legacy_status);
    }
  }
  else {
    rbus_status = rbusCoreError_to_rbusError(core_status);
  }

  return rbus_status;
}

static void rbusEventLoop_EnqueueResponse(
  rbusHandle_t                rbus,
  rbusMessage                 res,
  rbusCoreError_t             status,
  struct rbusAsyncRequest*    req,
  void                      (*exec)(void *))
{
  struct rbusEventLoopCallbackContext* ctx = rt_malloc(sizeof(struct rbusEventLoopCallbackContext));
  ctx->res = res;
  ctx->status = status;
  ctx->req = req;

  rbusMessage_Retain(ctx->res);

  rbusRunnable_t r;
  r.argp = ctx;
  r.exec = exec;
  r.cleanup = NULL;
  r.next = NULL;
  r.id = req->rt_request_id;

  rbusRunnableQueue_PushBack(&rbus->eventQueue, r);
}

static void rbusGetCompletionHandlerImpl(rbusMessage res, rbusCoreError_t status, void* user_data);
static void rbusGetCompletionHandlerAdapter(void* user_data) {
  struct rbusEventLoopCallbackContext* ctx = (struct rbusEventLoopCallbackContext *) user_data;
  rbusGetCompletionHandlerImpl(ctx->res, ctx->status, ctx->req);
  rbusMessage_Release(ctx->res);
  rt_free(ctx);
}

static int rbusGetCompletionHandler(rbusMessage res, rbusCoreError_t status, void* user_data)
{
  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;
  if (req->rbus->useEventLoop)
    rbusEventLoop_EnqueueResponse(req->rbus, res, status, req, &rbusGetCompletionHandlerAdapter);
  else
    rbusGetCompletionHandlerImpl(res, status, user_data);
  return 0;
}

void rbusGetCompletionHandlerImpl(rbusMessage res, rbusCoreError_t status, void* user_data)
{
  rbusProperty_t props = NULL;
  rbusError_t rbus_status = RBUS_ERROR_SUCCESS;

  if (res) {
    if (status == RBUSCORE_SUCCESS) {
      // TODO: "legacy" stuff should be encapsulated withing rbus_core
      rbusLegacyReturn_t legacy_status = 0;
      rbusMessage_GetStatusCodes(res, &rbus_status, &legacy_status);
      if ((rbus_status == RBUS_ERROR_SUCCESS) || (legacy_status == RBUS_LEGACY_ERR_SUCCESS)) {
        rbus_status = RBUS_ERROR_SUCCESS;

        const char* name = NULL;
        int32_t name_length = 0;
        rbusValue_t val = NULL;

        rbusMessage_GetInt32(res, &name_length);
        rbusMessage_GetString(res, &name);
        rbusValue_initFromMessage(&val, res);
        rbusProperty_Init(&props, name, val);
        rbusValue_Release(val);
      }
      else {
        // TODO: When is this non-zero? Is this the internal middleware status or is this
        // the application-layer error code?
        if (legacy_status > RBUS_LEGACY_ERR_SUCCESS)
          rbus_status = CCSPError_to_rbusError(legacy_status);
      }
    }
    else {
      rbus_status = rbusCoreError_to_rbusError(status);
      props = NULL;
    }
  }
  else {
    RBUSLOG_WARN("got a null message on asynchronous completion handler");
    rbus_status = RBUS_ERROR_BUS_ERROR;
    props = NULL;
  }

  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;
  struct rbusAsyncResponse rbus_res;
  rbus_res.status = rbus_status;
  rbus_res.props = props;
  rbus_res.user_data = req->user_data;
  if (req->completion_handler)
    req->completion_handler(req->rbus, &rbus_res);
  rbusAsyncRequest_SetCompleted(req, true);
  rbusAsyncRequest_Release(req);
  rbusProperty_Release(rbus_res.props);
}

rbusError_t rbusProperty_GetAsync(rbusHandle_t rbus, rbusAsyncRequest_t req)
{
  // guard here to prevent caller from re-using an rbusAsyncRequest_t that
  // may be still in progress
  if (req->rt_request_id != RT_INVALID_REQUEST_ID)
    return RBUS_ERROR_INVALID_INPUT;

  rbusMessage rbus_req;
  rbusMessage_Init(&rbus_req);
  rbusMessage_SetString(rbus_req, rbus->componentName);
  rbusMessage_SetInt32(rbus_req, (int32_t) 1);
  rbusMessage_SetString(rbus_req, rbusProperty_GetName(req->props));

  rbusAsyncRequest_Retain(req);
  req->rbus = rbus;

  rbusCoreError_t err = rbus_invokeRemoteMethodAsync(
    rbusProperty_GetName(req->props),
    METHOD_GETPARAMETERVALUES,
    rbus_req,
    req->timeout_millis,
    &rbusGetCompletionHandler,
    req,
    &req->rt_request_id);

  rbusMessage_Release(rbus_req);

  return rbusCoreError_to_rbusError(err);
}

static void rbusSetCompletionHandlerImpl(rbusMessage res, rbusCoreError_t status, void* user_data);
static void rbusSetCompletionHandlerAdapter(void* user_data) {
  struct rbusEventLoopCallbackContext* ctx = (struct rbusEventLoopCallbackContext *) user_data;
  rbusSetCompletionHandlerImpl(ctx->res, ctx->status, ctx->req);
  rbusMessage_Release(ctx->res);
  rt_free(ctx);
}

static int rbusSetCompletionHandler(rbusMessage res, rbusCoreError_t status, void* user_data)
{
  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;
  if (req->rbus->useEventLoop)
    rbusEventLoop_EnqueueResponse(req->rbus, res, status, req, &rbusSetCompletionHandlerAdapter);
  else
    rbusSetCompletionHandlerImpl(res, status, user_data);
  return 0;
}

void rbusSetCompletionHandlerImpl(rbusMessage res, rbusCoreError_t status, void* user_data)
{
  rbusError_t rbus_status = rbusCoreTranslateStatusFromResponse(res, status);

  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) user_data;
  struct rbusAsyncResponse rbus_res;
  rbus_res.props = req->props;
  rbus_res.status = rbus_status;
  if (rbus_res.status != RBUS_ERROR_SUCCESS)
    rbusMessage_GetString(res, &rbus_res.error_message);
  rbus_res.user_data = req->user_data;
  if (req->completion_handler)
    req->completion_handler(req->rbus, &rbus_res);
  rbusAsyncRequest_SetCompleted(req, true);
  rbusAsyncRequest_Release(req);
}

rbusError_t
rbusProperty_SetAsync(rbusHandle_t rbus, rbusAsyncRequest_t req)
{
  rbusMessage rbus_req;
  rbusMessage_Init(&rbus_req);
  rbusMessage_SetInt32(rbus_req, 0); // session id
  rbusMessage_SetString(rbus_req, rbus->componentName);
  rbusMessage_SetInt32(rbus_req, 1); // number of params
  rbusValue_appendToMessage(rbusProperty_GetName(req->props), rbusProperty_GetValue(req->props), rbus_req);
  rbusMessage_SetString(rbus_req, "TRUE");

  rbusAsyncRequest_Retain(req);
  req->rbus = rbus;

  rbusCoreError_t err = rbus_invokeRemoteMethodAsync(
    rbusProperty_GetName(req->props),
    METHOD_SETPARAMETERVALUES,
    rbus_req,
    req->timeout_millis,
    &rbusSetCompletionHandler,
    req,
    &req->rt_request_id);

  rbusMessage_Release(rbus_req);

  return rbusCoreError_to_rbusError(err);
}

rbusAsyncRequest_t rbusAsyncRequest_New()
{
  struct rbusAsyncRequest* req = rt_malloc(sizeof(struct rbusAsyncRequest));
  req->retainable.refCount = 1;
  rbusAsyncRequest_Reset(req);
  return req;
}

void rbusAsyncRequest_SetTimeout(rbusAsyncRequest_t req, int timeout_millis)
{
  req->timeout_millis = timeout_millis;
}

void rbusAsyncRequest_Destroy(rtRetainable* r)
{
  struct rbusAsyncRequest* req = (struct rbusAsyncRequest *) r;
  rbusProperty_Release(req->props);
  if (req->method_name)
    free(req->method_name);
  rbusObject_Release(req->method_params);
  rt_free(req);
}

void rbusAsyncRequest_Retain(rbusAsyncRequest_t req)
{
  rtRetainable_retain(req);
}

void rbusAsyncRequest_Release(rbusAsyncRequest_t req)
{
  rtRetainable_release(req, rbusAsyncRequest_Destroy);
}

void rbusAsyncRequest_ReleaseAuto(rbusAsyncRequest_t* req)
{
  if (req && *req) {
    rtRetainable_release(*req, rbusAsyncRequest_Destroy);
    *req = NULL;
  }
}

void rbusAsyncRequest_SetCompletionHandler(rbusAsyncRequest_t req, rbusAsyncResponseHandler_t callback)
{
  req->completion_handler = callback;
}

void rbusAsyncRequest_SetUserData(rbusAsyncRequest_t req, void* user_data)
{
  req->user_data = user_data;
}

void rbusAsyncRequest_Reset(rbusAsyncRequest_t req)
{
  req->timeout_millis = -1;
  req->completion_handler = NULL;
  req->user_data = NULL;
  req->props = NULL;
  req->rbus = NULL;
  req->rt_request_id = RT_INVALID_REQUEST_ID;
  req->method_name = NULL;
  req->method_handler = NULL;
  req->method_params = NULL;
  req->completed = false;
}

rbusError_t rbusAsyncRequest_Cancel(rbusAsyncRequest_t req)
{
  rtError rt_err = rtConnection_CancelAsyncRequest(req->rbus->connection, req->rt_request_id);

  rbusRunnable_t* prev = NULL;
  rbusRunnable_t* curr = NULL;

  pthread_mutex_lock(&req->rbus->eventQueue.mutex);
  curr = req->rbus->eventQueue.head;
  while (curr) {
    if (curr->id == req->rt_request_id) {
      RBUSLOG_INFO("found oustanging request id of %d, removing from runnable queue", curr->id);
      if (curr == req->rbus->eventQueue.head)
        req->rbus->eventQueue.head = curr->next;
      else if (curr == req->rbus->eventQueue.tail)
        prev->next = NULL;
      else
        prev->next = curr->next;
      rt_err = RT_OK;
      rt_free(curr);
      curr = NULL;
    }
    else {
      prev = curr;
      curr = curr->next;
    }
  }
  pthread_mutex_unlock(&req->rbus->eventQueue.mutex);

  rbusAsyncRequest_Reset(req);

  return rbusCoreError_to_rbusError(rt_err);
}

rbusError_t rbusAsyncResponse_GetStatus(rbusAsyncResponse_t res)
{
  return res->status;
}

rbusError_t rbusAsyncRequest_WaitUntil(rbusAsyncRequest_t req, int timeout_millis)
{
  rtTime_t start_time;
  rtTime_Now(&start_time);
  while (!req->completed) {
    rtTime_t now;
    rtTime_Now(&now);
    if (rtTime_Elapsed(&start_time, &now) > timeout_millis)
      return RBUS_ERROR_TIMEOUT;
    // rbusHandle_RunOne returns RBUS_ERROR_TIMEOUT if queue is empty
    rbusError_t internal_error = rbusHandle_RunOne(req->rbus);
    if (internal_error == RBUS_ERROR_TIMEOUT)
      usleep(1000 * req->rbus->busyWaitSleepDurationMillis);
  }
  return RBUS_ERROR_SUCCESS;
}

rbusProperty_t rbusAsyncResponse_GetProperty(rbusAsyncResponse_t res)
{
  return res->props;
}

void rbusAsyncRequest_AddProperty(rbusAsyncRequest_t req, rbusProperty_t prop)
{
  req->props = prop;
}

void* rbusAsyncResponse_GetUserData(rbusAsyncResponse_t res)
{
  return res->user_data;
}

void rbusAsyncRequest_SetMethodName(rbusAsyncRequest_t req, const char* method_name)
{
  if (method_name) {
    if (req->method_name)
      free(req->method_name);
    req->method_name = strdup(method_name);
  }
}

void rbusAsyncRequest_SetMethodParameters(rbusAsyncRequest_t req, rbusProperty_t argv)
{
  if (req->method_params)
    rbusObject_Release(req->method_params);
  rbusObject_Init(&req->method_params, "");
  rbusObject_SetProperties(req->method_params, argv);
}

/* End of File */
