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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static int dump_payload = 0;

static void on_message(rtMessageHeader const* hdr, rtMessage m, void* closure)
{
  (void) closure;

  printf("BEGIN MESSAGE:%s\n", hdr->topic);
  if (dump_payload)
  {
    char* payload;
    uint32_t payload_length;
    rtMessage_ToString(m, &payload, &payload_length);
    printf("%.*s\n", payload_length, payload);
  }
  printf("END MESSAGE\n");

  rtMessage_Release(m);
}

int main(int argc, char* argv[])
{
  int c;
  rtError err;
  rtConnection con;

  char const* router = RTMSG_DEFAULT_ROUTER_LOCATION;

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = 

    {
      { "router", required_argument, 0, 'r' },
      { "verbose", no_argument, 0, 'v' },
      { 0, 0, 0, 0 }
    };

    c = getopt_long(argc, argv, "r:v", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'r':
        router = optarg;
        break;
      case 'v':
        dump_payload = 1;
        break;
      case '?':
        break;
      default:
        fprintf(stderr, "?? getopt returne character code 0%o ??\n", c);
    }
  }

  if (argc == optind)
  {
    printf("no subscription supplied\n");
    exit(0);
  }

  rtLog_SetLevel(RT_LOG_WARN);
  rtConnection_Create(&con, "rtsub", router);
  rtConnection_AddListener(con, argv[argc-1], on_message, NULL);

  while ((err = rtConnection_Dispatch(con)) == RT_OK)
  {
  }

  rtConnection_Destroy(con);
  return 0;
}
