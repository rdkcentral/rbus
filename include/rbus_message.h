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

/**
 * @file        rubs_message.h
 * @brief       rbusMessage
 * @defgroup    rbusMessage
 * @brief       rbusMessage is a publish/subscribe api which allows rbus
                apps to send and receive binary data.
 * @{
 */

#ifndef RBUS_MESSAGE_H
#define RBUS_MESSAGE_H

#include "rbus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct      rbusMessage_t
 * @brief       The data associated with a sent message
 */
typedef struct
{
    char const*     topic;      /**< The topic the message is sent to */
    uint8_t const*  data;       /**< The binary data being sent */
    int             length;     /**< The binary data length */
} rbusMessage_t;

/**
 * @enum        rbusMessageOption_t
 * @brief       Option which controls how messages are sendt
 */
typedef enum
{
  RBUS_MESSAGE_NONE = 0, /**< The message is sent non-blocking with no
                                         confirmation of delivery */
  RBUS_MESSAGE_CONFIRM_RECEIPT = 1     /**< The message is sent, blocking until a response
                                         returns indicating whether the message
                                         was received by a listener or not */
} rbusMessageSendOptions_t;

/** @fn typedef void (*rbusMessageHandler_t)(
 *          rbusHandle_t handle, 
 *          rbusMessage_t message, 
 *          void* userData)
 *  @brief A component will receive this API callback when a message is received.
 *  This callback is registered with rbusMessage_AddListener.\n
 *  Used by: Any component that listens for messages.
 *  @param handle Bus Handle
 *  @param message The message being sent to the listener
 *  @param userData The user data set when adding the listener
 *  @return void
 */
typedef void (*rbusMessageHandler_t)(
    rbusHandle_t handle, 
    rbusMessage_t* message, 
    void* userData);

/** @fn rbusError_t rbusMessage_AddListener(
 *          rbusHandle_t handle,
 *          char const* expression,
 *          rbusMessageHandler_t callback,
 *          void * userData)
 *  @brief  Add a message listener.
 *  @param  handle Bus Handle
 *  @param  expression A topic or a topic expression
 *  @param  handler The message callback handler
 *  @param  userData User data to be passed back to the callback handler
 *  @return RBus error code as defined by rbusError_t.
 *  Possible errors are: RBUS_ERROR_BUS_ERROR
 */
rbusError_t rbusMessage_AddListener(
    rbusHandle_t handle,
    char const* expression,
    rbusMessageHandler_t handler,
    void* userData);

/** @fn rbusError_t rbusMessage_RemoveListener(
 *          rbusHandle_t handle,
 *          char const* expression)
 *  @brief  Remove a message listener.
 *  @param  handle Bus Handle
 *  @param  expression A topic or a topic expression
 *  @return RBus error code as defined by rbusError_t.
 *  Possible errors are: RBUS_ERROR_BUS_ERROR
 */
rbusError_t rbusMessage_RemoveListener(
    rbusHandle_t handle,
    char const* expression);

/** @fn rbusError_t rbusMessage_RemoveAllListeners(
 *          rbusHandle_t handle,
 *          char const* expression)
 *  @brief  Remove all message listeners.
 *  @param  handle Bus Handle
 *  @return RBus error code as defined by rbusError_t.
 *  Possible errors are: RBUS_ERROR_BUS_ERROR
 */
rbusError_t rbusMessage_RemoveAllListeners(
    rbusHandle_t handle);

/** @fn rbusError_t rbusMessage_Send(
 *          rbusHandle_t handle,
 *          rbusMessage_t* message,
 *          rbusMessageSendOptions_t opts)
 *  @brief  Send a message.
 *  @param  handle Bus Handle
 *  @param  message The message to send
 *  @param  opts Options to control how the message is sent
 *  @return RBus error code as defined by rbusError_t.
 *  Possible errors are: RBUS_ERROR_BUS_ERROR, RBUS_ERROR_DESTINATION_NOT_FOUND
 */
rbusError_t rbusMessage_Send(
    rbusHandle_t handle,
    rbusMessage_t* message,
    rbusMessageSendOptions_t opts);

#ifdef __cplusplus
}
#endif
#endif

/** @} */
