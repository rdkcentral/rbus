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
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"
#include "rtrouter_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
    if(1 == argc)
    {
        printf( "----------\nHelp:\n"
                "Syntax: rtrouted_diag <command>\n"
                "Following commands are supported:\n"
                "%-20s - Enable debug level logs in router.\n"
                "%-20s - Disable debug level logs in router.\n"
                "%-20s - Ask router to additionally listen to new socket.\n"
                "%-20s - Heart beat monitoring for rtrouted\n"
                "%-20s - Log routing tree stats.\n"
                "%-20s - Log routing tree topics.\n"
                "%-20s - Log routing tree routes.\n"
                "%-20s - Enable bus traffic logging.\n"
                "%-20s - Disable bus traffic logging.\n"
                "%-20s - Dump raw benchmark data to rtrouted logs.\n"
                "%-20s - Reset data collected so far for benchmarking.\n"
                "%-20s - Shutdown the server.\n"
                "----------\n",
                RTROUTER_DIAG_CMD_ENABLE_VERBOSE_LOGS,
                RTROUTER_DIAG_CMD_DISABLE_VERBOSE_LOGS,
                RTROUTER_DIAG_CMD_ADD_NEW_LISTENER,
                RTROUTER_DIAG_CMD_HEARTBEAT,
                RTROUTER_DIAG_CMD_LOG_ROUTING_STATS,
                RTROUTER_DIAG_CMD_LOG_ROUTING_TOPICS,
                RTROUTER_DIAG_CMD_LOG_ROUTING_ROUTES,
                RTROUTER_DIAG_CMD_ENABLE_TRAFFIC_MONITOR,
                RTROUTER_DIAG_CMD_DISABLE_TRAFFIC_MONITOR,
                RTROUTER_DIAG_CMD_DUMP_BENCHMARKING_DATA,
                RTROUTER_DIAG_CMD_RESET_BENCHMARKING_DATA,
                RTROUTER_DIAG_CMD_SHUTDOWN
                    );
    }
    else
    {
        rtConnection con;
        rtLog_SetLevel(RT_LOG_INFO);
        rtConnection_Create(&con, "APP2", "unix:///tmp/rtrouted");
        rtMessage out;
        rtMessage_Create(&out);
        rtMessage_SetString(out, RTROUTER_DIAG_CMD_KEY, argv[1]);

        /* This is usually used when you want to pass additional information to the broker */
        if (argv[2] != NULL)
            rtMessage_SetString(out, RTROUTER_DIAG_CMD_VALUE, argv[2]);

        rtConnection_SendMessage(con, out, RTROUTER_DIAG_DESTINATION);

        sleep(1);

        //just exit so rtConnection doesn't try to reconnect to broker
        if(strncmp(argv[1], RTROUTER_DIAG_CMD_SHUTDOWN, sizeof(RTROUTER_DIAG_CMD_SHUTDOWN))==0)
            exit(0);

        /* Connection Destroy */
        rtConnection_Destroy(con);
    }
    return 0;
}
