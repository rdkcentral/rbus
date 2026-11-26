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
#include <openssl/crypto.h>

int main(int argc, char * argv[])
{
    const char* address = "unix:///tmp/rtrouted";
    int option;

    while ((option = getopt(argc, argv, "s:")) != -1)
    {
        switch (option)
        {
            case 's': address = optarg; break;
            default: return 1;
        }
    }

    if(optind == argc)
    {
        printf( "----------\nHelp:\n"
                "Syntax: rtrouted_diag [-s unix://path|tcp://ip:port] <command> [value]\n"
                "Default socket: unix:///tmp/rtrouted\n"
                "Unix and TCP require a shared key: " RTROUTER_DIAG_KEY_FILE "\n"
                "Router creates a unique owner-only key on first start; the key directory must be writable.\n"
                "Override the key path with RTROUTER_DIAG_KEY_FILE on router and client.\n"
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
        return argc == 1 ? 0 : 1;
    }
    else
    {
        rtConnection con = NULL;
        rtMessage out = NULL;
        rtMessage response = NULL;
        unsigned char key[32] = {0};
        const char* key_file = getenv("RTROUTER_DIAG_KEY_FILE");
        const char* nonce = NULL;
        const char* command = argv[optind];
        const char* value = optind + 1 < argc ? argv[optind + 1] : NULL;
        char mac[65];
        int32_t result = RT_ERROR;
        int status = 1;

        rtLog_SetLevel(RT_LOG_INFO);
        if (rtDiagAuth_LoadKey(key_file ? key_file : RTROUTER_DIAG_KEY_FILE, key) != 0 ||
            rtConnection_Create(&con, "APP2", address) != RT_OK ||
            rtMessage_Create(&out) != RT_OK)
            goto done;
        rtMessage_SetString(out, RTROUTER_DIAG_CMD_KEY, RTROUTER_DIAG_CHALLENGE);
        if (rtConnection_SendRequest(con, out, RTROUTER_DIAG_DESTINATION, &response, 5000) != RT_OK ||
            rtMessage_GetInt32(response, "result", &result) != RT_OK || result != RT_OK ||
            rtMessage_GetString(response, RTROUTER_DIAG_NONCE, &nonce) != RT_OK ||
            rtDiagAuth_Sign(key, nonce, command, value, mac) != 0)
            goto done;
        rtMessage_Release(response);
        response = NULL;
        rtMessage_Release(out);
        out = NULL;
        if (rtMessage_Create(&out) != RT_OK)
            goto done;
        rtMessage_SetString(out, RTROUTER_DIAG_CMD_KEY, command);
        rtMessage_SetString(out, RTROUTER_DIAG_MAC, mac);
        if (value)
            rtMessage_SetString(out, RTROUTER_DIAG_CMD_VALUE, value);
        if (rtConnection_SendRequest(con, out, RTROUTER_DIAG_DESTINATION, &response, 5000) == RT_OK &&
            rtMessage_GetInt32(response, "result", &result) == RT_OK && result == RT_OK)
            status = 0;
done:
        OPENSSL_cleanse(key, sizeof(key));
        if (status != 0)
            rtLog_Error("Diagnostic request failed: check shared key and router authorization");
        if (out)
            rtMessage_Release(out);
        if (response)
            rtMessage_Release(response);
        /* Do not reconnect to a broker that has just acknowledged shutdown. */
        if (con && !(status == 0 && strcmp(command, RTROUTER_DIAG_CMD_SHUTDOWN) == 0))
            rtConnection_Destroy(con);
        return status;
    }
    return 0;
}
