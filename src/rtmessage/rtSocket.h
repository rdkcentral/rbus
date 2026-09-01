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
#ifndef __RT_SOCKET_H__
#define __RT_SOCKET_H__

#include "rtError.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

rtError rtSocket_GetLocalEndpoint(int fd, struct sockaddr_storage* endpoint);
rtError rtSocketStorage_GetLength(struct sockaddr_storage* endpoint, socklen_t* len);
rtError rtSocketStorage_ToString(struct sockaddr_storage* endpoint, char* buff, int n, uint16_t* port);
rtError rtSocketStorage_FromString(struct sockaddr_storage* soc, char const* path);

#ifdef __cplusplus
}
#endif
#endif
