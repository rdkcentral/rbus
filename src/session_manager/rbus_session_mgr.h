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
#ifndef __RBUS_SESSION_MANAGER_H__
#define __RBUS_SESSION_MANAGER_H__

#define RBUS_SMGR_DESTINATION_NAME "_rbus_session_mgr" //Where to send the RPC calls.
/* Supported RPC methods */
#define RBUS_SMGR_METHOD_GET_CURRENT_SESSION_ID "get_curr_s" //Returns session id as int32, key MESSAGE_FIELD_PAYLOAD 
#define RBUS_SMGR_METHOD_REQUEST_SESSION_ID "req_new_s" //Returns new session id as int32, key MESSAGE_FIELD_PAYLOAD
#define RBUS_SMGR_METHOD_END_SESSION "end_of_s" // Requires session id as input argument, key MESSAGE_FIELD_PAYLOAD

#endif
