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
#ifndef __RTMSG_CONNECTION_H__
#define __RTMSG_CONNECTION_H__

#include "rtError.h"
#include "rtMessage.h"
#include "rtMessageHeader.h"

#define RTMSG_DEFAULT_ROUTER_LOCATION "tcp://127.0.0.1:10001"
#define RTROUTED_TRANSACTION_TIME_INFO "TransactionTime"

#ifdef __cplusplus
extern "C" {
#endif

struct _rtConnection;
typedef struct _rtConnection* rtConnection;

typedef void (*rtMessageCallback)(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);

typedef enum
{
  rtConnectionState_ReadHeaderPreamble,
  rtConnectionState_ReadHeader,
  rtConnectionState_ReadPayload
} rtConnectionState;

/**
 * Creates an rtConnection
 * @param con
 * @param application_name
 * @param router_config
 * @return error
 */
rtError
rtConnection_Create(rtConnection* con, char const* application_name, char const* router_config);

/**
 * Creates an rtConnection from a Config file
 * @param con
 * @param conf
 * @return error
 */
rtError
rtConnection_CreateWithConfig(rtConnection* con, rtMessage const conf);

/**
 * Destroy an rtConnection
 * @param con
 * @return error
 */
rtError
rtConnection_Destroy(rtConnection con);

/******************************************************************************************
 *  rtMessage(struct) based API
 */

/**
 * Sends a message on a topic
 * @param con
 * @param msg
 * @param topic
 * @return error
 */
rtError
rtConnection_SendMessage(rtConnection con, rtMessage msg, char const* topic);

/**
 * Send a message to a specific listener on a topic
 * @param con
 * @param msg
 * @param topic
 * @param sender 
 * @return error
 */
rtError
rtConnection_SendMessageDirect(rtConnection con, rtMessage msg, char const* topic, char const* listener);

/**
 * Sends a request and receive a response
 * @param con
 * @param req
 * @param topic
 * @param response
 * @param timeout
 * @return error
 */
rtError
rtConnection_SendRequest(rtConnection con, rtMessage const req, char const* topic,
  rtMessage* res, int32_t timeout);

/**
 * Sends a response to a request
 * @param con
 * @param hdr
 * @param response
 * @param timeout
 * @return error
 */
rtError
rtConnection_SendResponse(rtConnection con, rtMessageHeader const* request_hdr, rtMessage const res,
  int32_t timeout);

/******************************************************************************************
 *  Binary based API
 */

/**
 * Send a message with binary payload
 * @param con
 * @param topic
 * @param pointer to buffer
 * @param length of buffer
 * @return error
 */
rtError
rtConnection_SendBinary(rtConnection con, uint8_t const* p, uint32_t n, char const* topic);

/**
 * Send a message to a specific listener on a topic
 * @param con
 * @param msg
 * @param topic
 * @param sender 
 * @return error
 */
rtError
rtConnection_SendBinaryDirect(rtConnection con, uint8_t const* p, uint32_t n, char const* topic, char const* listener);

/**
 * Sends a request and receive a response
 * @param con
 * @param req
 * @param topic
 * @param response
 * @param timeout
 * @return error
 */
rtError
rtConnection_SendBinaryRequest(rtConnection con, uint8_t const* pReq, uint32_t nReq, char const* topic,
  uint8_t** pRes, uint32_t* nRes, int32_t timeout);

/**
 * Sends a response to a request
 * @param con
 * @param hdr
 * @param response
 * @param timeout
 * @return error
 */
rtError
rtConnection_SendBinaryResponse(rtConnection con, rtMessageHeader const* request_hdr, uint8_t const* p, uint32_t n,
  int32_t timeout);

/*
 *  End Binary based API
 *******************************************************************************************/

/**
 * Register a callback for message receipt
 * @param con
 * @param topic expression
 * @param callback handler
 * @param closure
 * @return error
 */
rtError
rtConnection_AddListener(rtConnection con, char const* expression,
  rtMessageCallback callback, void* closure);

/**
 * Remove a callback listener
 * @param con
 * @param topic expression
 * @return error
 */
rtError
rtConnection_RemoveListener(rtConnection con, char const* expression);

/**
 * Add an alias to an existing listener
 * @param con
 * @param existing listener 
 * @param alias 
 * @return error
 */
rtError
rtConnection_AddAlias(rtConnection con, char const* existing, const char *alias);

/**
 * Remove an alias to an existing listener
 * @param con
 * @param existing listener 
 * @param alias 
 * @return error
 */
rtError
rtConnection_RemoveAlias(rtConnection con, char const* existing, const char *alias);

/**
 * Register a callback for message receipt
 * @param con
 * @param topic expression
 * @param callback handler
 * @return error
 */
rtError
rtConnection_AddDefaultListener(rtConnection con, rtMessageCallback callback, void* closure);

/**
 * Get return address for this connection. 
 * @param con
 * return address 
 */
const char *
rtConnection_GetReturnAddress(rtConnection con);

/**
 * Leftover after merging rdkb to rdkc.  Will now just sleep.  TODO: Must update several rdkc components to remove this method
 * @param con
 * @return error
 */
rtError
rtConnection_Dispatch(rtConnection con);

/*Private declarations below*/
rtError
_rtConnection_ReadAndDropBytes(int fd, unsigned int bytes_to_read);

void
_rtConnection_TaintMessages(int i);

#ifdef __cplusplus
}
#endif
#endif
