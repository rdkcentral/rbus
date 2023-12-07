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
#include "rbus.h"
#include "rbus_message.h"
#include "rbus_handle.h"
#include "rbuscore.h"
#include <rtMemory.h>
#include <string.h>

#define VERIFY_NULL(T) if(NULL == T){ return RBUS_ERROR_INVALID_INPUT; }

#define RBUS_MESSAGE_MUTEX_LOCK()     \
{                                                                         \
  int err;                                                                \
  if((err = pthread_mutex_lock(&gMutex)) != 0)                            \
  {                                                                       \
    RBUSLOG_ERROR("Error @ mutex lock.. Err=%d:%s ", err, strerror(err)); \
  }                                                                       \
}

#define RBUS_MESSAGE_MUTEX_UNLOCK()   \
{                                                                           \
  int err;                                                                  \
  if((err = pthread_mutex_unlock(&gMutex)) != 0)                            \
  {                                                                         \
    RBUSLOG_ERROR("Error @ mutex unlock.. Err=%d:%s ", err, strerror(err)); \
  }                                                                         \
}

static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    rbusHandle_t          handle;
    char*                 expression;
    rbusMessageHandler_t  handler;
    void*                 userData;
    rtConnection          connection;
    uint32_t              subscriptionId;
} rbusMessageHandlerContext_t;

rbusError_t rtError_to_rBusError(rtError e)
{
    rbusError_t err;
    switch (e)
    {
    case RT_OBJECT_NO_LONGER_AVAILABLE:
        err = RBUS_ERROR_DESTINATION_NOT_FOUND;
        break;
    default:
        err = RBUS_ERROR_BUS_ERROR;
        break;
    }
    return err;
}

static void rtMessage_CallbackHandler(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* userData)
{
    rbusMessageHandlerContext_t* ctx = (rbusMessageHandlerContext_t*)userData;
    if((!ctx)||(!hdr))
        return;
    // if this is request, the sender wants confirmation of receipt
    // do that before dispatching application callback
    if (rtMessageHeader_IsRequest(hdr))
    {
        uint8_t res = 0;
        uint32_t resLength = (uint32_t) sizeof(res);

        rtError e = rtConnection_SendBinaryResponse(ctx->connection, hdr, &res, resLength, 2000);
        if (e != RT_OK)
        {
            RBUSLOG_WARN("error sending response for confirmed receipt. %s", rtStrError(e));
        }
    }

    if (ctx->handler)
    {
        rbusMessage_t message;
        message.topic = hdr->topic;
        message.data = buff;
        message.length = n;

        ctx->handler(ctx->handle, &message, ctx->userData);
    }
}

static void cleanupContext(void * p)
{
    rbusMessageHandlerContext_t* ctx = (rbusMessageHandlerContext_t*)p;
    if(ctx){
        if(ctx->expression)
            free(ctx->expression);
        free(ctx);
    }
}

static int compareContextExpression(const void *left, const void *right)
{
    if((!left)||(!right))
        return -1;
    return strcmp(((const rbusMessageHandlerContext_t*)left)->expression, (const char*)right);
}

rbusError_t rbusMessage_AddPrivateListener(
    rbusHandle_t handle,
    char const* expression,
    rbusMessageHandler_t handler,
    void* userData,
    uint32_t subscriptionId)
{
    VERIFY_NULL(handle);
    VERIFY_NULL(expression);

    char rawDataTopic[RBUS_MAX_NAME_LENGTH] = {0};

    rtConnection myConn = rbuscore_FindClientPrivateConnection(expression);
    if (NULL == myConn)
    {
        return RBUS_ERROR_DIRECT_CON_NOT_EXIST;
    }

    rbusMessageHandlerContext_t* ctx = rt_malloc(sizeof(rbusMessageHandlerContext_t));
    ctx->handle = handle;
    ctx->expression = strdup(expression);
    ctx->handler = handler;
    ctx->userData = userData;
    ctx->connection = myConn;
    ctx->subscriptionId = subscriptionId;
    RBUS_MESSAGE_MUTEX_LOCK();
    rtVector_PushBack(handle->messageCallbacks, ctx);

    snprintf(rawDataTopic, RBUS_MAX_NAME_LENGTH, "%d.%s", subscriptionId, expression);
    rtError e = rtConnection_AddListenerWithId(myConn, rawDataTopic, subscriptionId, &rtMessage_CallbackHandler, ctx);
    RBUS_MESSAGE_MUTEX_UNLOCK();
    if (e != RT_OK)
    {
        RBUSLOG_WARN("rtConnection_AddListenerWithId:%s", rtStrError(e));
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusMessage_AddListener(
    rbusHandle_t handle,
    char const* expression,
    rbusMessageHandler_t handler,
    void* userData,
    uint32_t subscriptionId)
{
    VERIFY_NULL(handle);
    VERIFY_NULL(expression);
    rtConnection con = ((struct _rbusHandle*)handle)->m_connection;

    rbusMessageHandlerContext_t* ctx = rt_malloc(sizeof(rbusMessageHandlerContext_t));
    ctx->handle = handle;
    ctx->expression = strdup(expression);
    ctx->handler = handler;
    ctx->userData = userData;
    ctx->connection = con;
    ctx->subscriptionId = subscriptionId;
    RBUS_MESSAGE_MUTEX_LOCK();
    rtVector_PushBack(handle->messageCallbacks, ctx);

    rtError e = rtConnection_AddListenerWithId(con, expression, subscriptionId, &rtMessage_CallbackHandler, ctx);
    RBUS_MESSAGE_MUTEX_UNLOCK();
    if (e != RT_OK)
    {
        RBUSLOG_WARN("rtConnection_AddListenerWithId:%s", rtStrError(e));
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusMessage_RemovePrivateListener(
    rbusHandle_t handle,
    char const* expression,
    uint32_t subscriptionId)
{
    VERIFY_NULL(handle);
    char rawDataTopic[RBUS_MAX_NAME_LENGTH] = {0};

    rtConnection myConn = rbuscore_FindClientPrivateConnection(expression);
    if (NULL == myConn)
    {
        return RBUS_ERROR_DIRECT_CON_NOT_EXIST;
    }

    snprintf(rawDataTopic, RBUS_MAX_NAME_LENGTH, "%d.%s", subscriptionId, expression);
    RBUS_MESSAGE_MUTEX_LOCK();
    rtVector_RemoveItemByCompare(handle->messageCallbacks, rawDataTopic, compareContextExpression, cleanupContext);

    rtError e = rtConnection_RemoveListenerWithId(myConn, rawDataTopic, subscriptionId);
    RBUS_MESSAGE_MUTEX_UNLOCK();
    if (e != RT_OK)
    {
        RBUSLOG_WARN("rtConnection_RemoveListenerWithId:%s", rtStrError(e));
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusMessage_RemoveListener(
    rbusHandle_t handle,
    char const* expression,
    uint32_t subscriptionId)
{
    VERIFY_NULL(handle);
    rtConnection con = ((struct _rbusHandle*)handle)->m_connection;

    RBUS_MESSAGE_MUTEX_LOCK();
    rtVector_RemoveItemByCompare(handle->messageCallbacks, expression, compareContextExpression, cleanupContext);

    rtError e = rtConnection_RemoveListenerWithId(con, expression, subscriptionId);
    RBUS_MESSAGE_MUTEX_UNLOCK();
    if (e != RT_OK)
    {
        RBUSLOG_WARN("rtConnection_RemoveListenerWithId:%s", rtStrError(e));
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t rbusMessage_RemoveAllListeners(
    rbusHandle_t handle)
{
    VERIFY_NULL(handle);
    rtConnection con = ((struct _rbusHandle*)handle)->m_connection;
    int i, n;

    RBUS_MESSAGE_MUTEX_LOCK();
    for (i = 0, n = rtVector_Size(handle->messageCallbacks); i < n; ++i)
    {
        rbusMessageHandlerContext_t* ctx = rtVector_At(handle->messageCallbacks, i);
        VERIFY_NULL(ctx);
        rtError e = rtConnection_RemoveListenerWithId(con, ctx->expression, ctx->subscriptionId);
        if (e != RT_OK)
        {
            RBUSLOG_WARN("rbusMessage_RemoveAllListener %s :%s", ctx->expression, rtStrError(e));
        }    
        free(ctx->expression);
        free(ctx);
    }
    RBUS_MESSAGE_MUTEX_UNLOCK();
    return RBUS_ERROR_SUCCESS;
}

int rbusMessage_HasListener(
    rbusHandle_t handle,
    char const* topic)
{
    int ret = 0;
    VERIFY_NULL(handle);
    VERIFY_NULL(topic);
    int i, n;

    RBUS_MESSAGE_MUTEX_LOCK();
    for (i = 0, n = rtVector_Size(handle->messageCallbacks); i < n; ++i)
    {
        rbusMessageHandlerContext_t* ctx = rtVector_At(handle->messageCallbacks, i);
        if(!ctx)
            continue;
        else
        {
            if(!strcmp(ctx->expression, topic))
            {
                ret = 1;
                break;
            }
        }
    }
    RBUS_MESSAGE_MUTEX_UNLOCK();
    return ret;
}

rbusError_t rbusMessage_Send(
    rbusHandle_t handle,
    rbusMessage_t* message,
    rbusMessageSendOptions_t opts)
{
    VERIFY_NULL(handle);
    rtConnection con = ((struct _rbusHandle*)handle)->m_connection;

    VERIFY_NULL(message);
    if (opts & RBUS_MESSAGE_CONFIRM_RECEIPT)
    {
        uint8_t * res = NULL;
        uint32_t  resLength = 0;

        rtError e = rtConnection_SendBinaryRequest(con, message->data, message->length, message->topic, &res, &resLength, 2000);
        if (e != RT_OK)
        {
            RBUSLOG_WARN("rtConnection_SendRequest:%s", rtStrError(e));
            return rtError_to_rBusError(e);
        }
        if(res)
            free(res);
    }
    else
    {
        rtError e = rtConnection_SendBinary(con, message->data, message->length, message->topic);
        if (e != RT_OK)
        {
            RBUSLOG_WARN("rtConnection_SendBinary:%s", rtStrError(e));
            return rtError_to_rBusError(e);
        }
    }

    return RBUS_ERROR_SUCCESS;
}
