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
#ifndef __RTROUTER_DIAG_H__
#define __RTROUTER_DIAG_H__
#define RTROUTER_DIAG_DESTINATION "_RTROUTED.INBOX.DIAG"
#define RTROUTER_DIAG_CMD_KEY "_RTROUTED.INBOX.DIAG.KEY"
#define RTROUTER_DIAG_CMD_VALUE "_RTROUTED.INBOX.DIAG.VALUE"

#define RTROUTER_DIAG_CMD_ENABLE_VERBOSE_LOGS       "enableVerboseLogs"
#define RTROUTER_DIAG_CMD_DISABLE_VERBOSE_LOGS      "disableVerboseLogs"

#define RTROUTER_DIAG_CMD_LOG_ROUTING_STATS         "logRoutingStats"
#define RTROUTER_DIAG_CMD_LOG_ROUTING_TOPICS        "logRoutingTopics"
#define RTROUTER_DIAG_CMD_LOG_ROUTING_ROUTES        "logRoutingRoutes"

#define RTROUTER_DIAG_CMD_ENABLE_TRAFFIC_MONITOR    "enableTrafficMonitor"
#define RTROUTER_DIAG_CMD_DISABLE_TRAFFIC_MONITOR   "disableTrafficMonitor"

#define RTROUTER_DIAG_CMD_RESET_BENCHMARKING_DATA   "resetBenchmarkData"
#define RTROUTER_DIAG_CMD_DUMP_BENCHMARKING_DATA    "dumpBenchmarkData"

#define RTROUTER_DIAG_CMD_ADD_NEW_LISTENER          "addNewListener"
#define RTROUTER_DIAG_CMD_SHUTDOWN                  "shutdown"
#define RTROUTER_DIAG_CMD_HEARTBEAT                 "heartbeat"

#include <time.h>

#define RTROUTER_DIAG_KEY_FILE "/etc/rtrouted-diag.key"
#define RTROUTER_DIAG_CHALLENGE "authChallenge"
#define RTROUTER_DIAG_NONCE "_RTROUTED.INBOX.DIAG.NONCE"
#define RTROUTER_DIAG_MAC "_RTROUTED.INBOX.DIAG.MAC"
#define RTROUTER_DIAG_COMMAND_MAX 64
#define RTROUTER_DIAG_VALUE_MAX 127

typedef struct
{
  char nonce[65];
  struct timespec issued;
} rtDiagChallenge;

int rtDiagAuth_LoadKey(const char* path, unsigned char key[32]);
int rtDiagAuth_EnsureKey(const char* path);
int rtDiagAuth_Challenge(rtDiagChallenge* state);
int rtDiagAuth_Sign(const unsigned char key[32], const char* nonce,
    const char* command, const char* value, char mac[65]);
int rtDiagAuth_Verify(rtDiagChallenge* state, const unsigned char key[32],
    const char* command, const char* value, const char* mac);

#endif
