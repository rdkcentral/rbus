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

#endif
