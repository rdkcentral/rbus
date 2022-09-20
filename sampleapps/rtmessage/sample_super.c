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
#include "rtAdvisory.h"
#include "rtVector.h"
#include "rtm_discovery_api.h"
#if WITH_SPAKE2
#include "password.h"
#endif
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#define MAX_TOPICS 20
#define MAX_ALIAS 20
#define MAX_TOPIC_LEN 256

static rtConnection g_connection;
static int messageReceived = 0;

void printHelp()
{
  printf("sample_super: this app allows you to send a message to a topic and/or receive a message on a topic.\n");
  printf("\t -S --send  send a message\n");
  printf("\t -R --request  send a request and get response\n");
  printf("\t -L --listen for messages\n");
  printf("\t -A --listen for advisory messages from rtrouted\n");
  printf("\t -W --disc_wildcarddest  discover wildcard destinations\n");
  printf("\t -O --disc_objelem  discover object elements\n"); 
  printf("\t -E --disc_elemobjs  discover element objects\n");
  printf("\t -C --disc_regcomps  discover registered components\n");
  printf("\t -t --topic  topic path (default: \"A.B.C\")\n");
  printf("\t -m --msg  text message to send (default: \"Hello World!\")\n");
  printf("\t -w --wait  seconds to wait before exiting (default: 30)\n");
  printf("\t -b --broker uri (default: \"tcp://127.0.0.1:10001\")\n");
  printf("\t                  for uds: unix:///tmp/rtrouted\n");
  printf("\t -r --max_retries max connection retries to attempt\n");
#ifdef WITH_SPAKE2
  printf("\t -s --secure enable encryption (default: false)\n");
#endif
  printf("\t -l --log-level log level (default: RT_LOG_DEBUG)\n");
  printf("\t -h --help show help info\n");
  fflush(stdout);
}

void onMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  rtConnection con = (rtConnection) closure;
  char* buff2 = NULL;
  uint32_t buff_length = 0;

  rtMessage req;
  rtMessage_FromBytes(&req, buff, n);

  rtMessage_ToString(req, &buff2, &buff_length);
  printf("sample_super got message:%.*s\n", buff_length, buff2);
  free(buff2);

  rtMessage_Release(req);

  if (rtMessageHeader_IsRequest(hdr))
  {
    rtMessage res;
    static int response_index = 1;
    char buff[200];
    snprintf(buff, 200, "Response message: What's up! (%d)", response_index++); 
    printf("sample_super sendinging response.\n");
    rtMessage_Create(&res);
    rtMessage_SetString(res, "reply", buff);
    rtConnection_SendResponse(con, hdr, res, 1000);
    rtMessage_Release(res);
  }

  messageReceived = 1;
}

void discoverWildcardDestinations(const char * expression)
{
    rtError err = RT_OK;
    rtMessage msg, rsp;

    rtMessage_Create(&msg);
    rtMessage_SetString(msg, RTM_DISCOVERY_EXPRESSION, expression);

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_WILDCARD_DESTINATIONS, &rsp, 3000);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int result;
        const char * value = NULL;

        if((RT_OK == rtMessage_GetInt32(msg, RTM_DISCOVERY_RESULT, &result)) && (RT_OK == result))
        {
            int32_t size, length, i;

            rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
            rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

            printf("discoverWildcardDestinations %s has %d items:\n", expression, length);

            for (i = 0; i < length; i++)
            {
                if (RT_OK == rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value))
                {
                    printf("%d: %s\n", i, value);
                }
            }

            rtMessage_Release(msg);

        }
        else
        {
            rtLog_Error("discoverWildcardDestinations failed to read result");
            rtMessage_Release(msg);
        }
    }
    else
    {
        rtLog_Error("discoverWildcardDestinations SendRequest failed err:%d", err);
    }
}

void discoverObjectElements(const char * object)
{
    rtError err = RT_OK;
    rtMessage msg, rsp;

    rtMessage_Create(&msg);
    rtMessage_SetString(msg, RTM_DISCOVERY_EXPRESSION, object);

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_OBJECT_ELEMENTS, &rsp, 3000);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int32_t size, length, i;
        const char * value = NULL;

        rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
        rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

        printf("discoverObjectElements %s has %d items:\n", object, length);

        for (i = 0; i < length; i++)
        {
            if (RT_OK == rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value))
            {
              printf("%d: %s\n", i, value);
            }
        }

        rtMessage_Release(msg);

    }
    else
    {
        rtLog_Error("discoverObjectElements SendRequest failed err:%d", err);
    }
}

void discoverElementObjects(const char* elements)
{
    rtError err = RT_OK;
    rtMessage msg, rsp;

    rtMessage_Create(&msg);
    rtMessage_SetInt32(msg, RTM_DISCOVERY_COUNT, 1);
    rtMessage_AddString(msg, RTM_DISCOVERY_ITEMS, elements);

    err = rtConnection_SendRequest(g_connection, msg, RTM_DISCOVER_ELEMENT_OBJECTS, &rsp, 3000);

    rtMessage_Release(msg);
    msg = rsp;

    if(RT_OK == err)
    {
        int result;
        const char * value = NULL;

        if((RT_OK == rtMessage_GetInt32(msg, RTM_DISCOVERY_RESULT, &result)) && (RT_OK == result))
        {
            int num_elements = 0;
            int i;
            rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &num_elements);

            printf("discoverElementObjects %s has %d items:\n", elements, num_elements);

            for (i = 0; i < num_elements; i++)
            {
                if (RT_OK == rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value))
                {
                  printf("%d: %s\n", i, value);
                }
            }
        }
        else
        {
            rtLog_Error("discoverElementObjects failed to read result");
        }
        rtMessage_Release(msg);
    }
    else
    {
        rtLog_Error("discoverElementObjects SendRequest failed err:%d", err);
    }
}

void discoverRegisteredComponents()
{
    rtError err = RT_OK;
    rtMessage msg;
    rtMessage out;
    rtMessage_Create(&out);
    rtMessage_SetInt32(out, "dummy", 0);
    
    err = rtConnection_SendRequest(g_connection, out, RTM_DISCOVER_REGISTERED_COMPONENTS, &msg, 3000);

    if(RT_OK == err)
    {
        int32_t size, length, i;
        const char * value = NULL;

        rtMessage_GetInt32(msg, RTM_DISCOVERY_COUNT, &size);
        rtMessage_GetArrayLength(msg, RTM_DISCOVERY_ITEMS, &length);

        printf("discoverRegisteredComponents has %d items:\n", length);

        for (i = 0; i < length; i++)
        {
            if (RT_OK == rtMessage_GetStringItem(msg, RTM_DISCOVERY_ITEMS, i, &value))
            {
                printf("%d: %s\n", i, value);
            }
        }

        rtMessage_Release(msg);
    }
    else
    {
        rtLog_Error("discoverRegisteredComponents SendRequest failed err:%d", err);
    }
}


int main(int argc, char* argv[])
{
  rtError err;
  rtMessage config;
  int sending = 0;
  int requesting = 0;
  int listening = 0;
  int listening_advisory = 0;
  int numTopics = 0;
  int numAlias = 0;
#if WITH_SPAKE2
  int isSecure = 0;
#endif  
  char topics[MAX_TOPICS][MAX_TOPIC_LEN] = { "A.B.C", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0" };
  char alias[MAX_ALIAS][MAX_TOPIC_LEN] = { "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0", "\0" };
  char const* message = "Hello World";
  int wait = 30;
  char const* broker_uri = "tcp://127.0.0.1:10001";
  int max_retries = 0;
  int disc_wildcarddest=0, disc_objelems=0, disc_elemobjs=0, disc_regcomps=0;
  char *disc_wildcarddest_name=NULL, *disc_objelems_name=NULL, *disc_elemobjs_name=NULL;
  int message_index = 1;

  printf("logfile=/opt/logs/rtmessage_%d.log\n", getpid());

  rtLog_SetLevel(RT_LOG_DEBUG);

  while (1)
  {
    int option_index = 0;
    int c;

    static struct option long_options[] = 
    {
      {"send",              no_argument,        0, 'S' },
      {"request",           no_argument,        0, 'R' },
      {"listen",            no_argument,        0, 'L' },
      {"listen_advisory",   no_argument,        0, 'A' },
      {"disc_wildcarddest", required_argument,  0, 'W' },
      {"disc_objelems",     required_argument,  0, 'O' },
      {"disc_elemobjs",     required_argument,  0, 'E' },
      {"disc_regcomps",     no_argument,        0, 'C' },
      {"topic",             required_argument,  0, 't' },
      {"alias",             required_argument,  0, 'a' },
      {"msg",               required_argument,  0, 'm' },
      {"wait",              required_argument,  0, 'w' },
      {"broker",            required_argument,  0, 'b' },
      {"max_retries",       required_argument,  0, 'r' },
#ifdef WITH_SPAKE2
      {"secure",            required_argument,  0, 's' },
#endif
      {"log-level",         required_argument,  0, 'l' },
      {"help",              no_argument,        0, 'h' },
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "SRLAW:O:E:Ct:a:m:w:b:r:sl:h", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'S':
        sending = 1;
        printf("Argument: Send=true\n");
        break;
      case 'R':
        requesting = 1;
        printf("Argument: Request=true\n");
        break;
      case 'L':
        listening = 1;
        printf("Argument: Listen=true\n");
        break;
      case 'A':
        listening_advisory = 1;
        printf("Argument: Listen Advisory=true\n");
        break;
      case 'W':
        disc_wildcarddest = 1;
        disc_wildcarddest_name = optarg;
        break;
      case 'O':
        disc_objelems = 1;
        disc_objelems_name = optarg;
        break;
      case 'E':
        disc_elemobjs = 1;
        disc_elemobjs_name = optarg;
        break;
      case 'C':
        disc_regcomps = 1;
        break;
      case 't':
        if(numTopics < MAX_TOPICS)
        {
            strncpy(topics[numTopics], optarg, MAX_TOPIC_LEN-1);
            topics[numTopics][255] = 0;
            printf("Argument: Topic=%s\n", topics[numTopics]);
            numTopics++;
        }
        else
        {
            printf("Argument: max supported topics %d reached.  Ignoring %s\n", MAX_TOPICS, optarg);
        }
        break;
      case 'a':
        if(numAlias < MAX_ALIAS)
        {
            strncpy(alias[numAlias], optarg, MAX_TOPIC_LEN-1);
            alias[numAlias][255] = 0;
            printf("Argument: Alias=%s\n", alias[numAlias]);
            numAlias++;
        }
        else
        {
            printf("Argument: max supported aliases %d reached.  Ignoring %s\n", MAX_ALIAS, optarg);
        }
        break;      case 'm':
        message = optarg;
        printf("Argument: Message=%s\n", message);
        break;
      case 'w':
        wait = atoi(optarg);
        printf("Argument: Wait=%d\n", wait);
        break;
      case 'l':
        {
            rtLog_SetLevel(rtLogLevelFromString(optarg));
            printf("Argument: Log level=%s\n", rtLogLevelToString(rtLog_GetLevel()));
        }
        break;
      case 'b':
        broker_uri = optarg;
        printf("Argument: broker_uri=%s\n", broker_uri);
        break;
      case 'r':
        max_retries = atoi(optarg);
        printf("Argument: Max Connect Retries=%d\n", max_retries);
        break;
#ifdef WITH_SPAKE2
      case 's':
        isSecure = 1;
        printf("Argument: secure enabled\n");
        break;
#endif
      case 'h':
        printHelp();
        exit(0);
        break;
      default:
        fprintf(stderr, "?? getopt returned character code 0%o ??\n\trun sample_super -h for help", c);
    }
  }

  if(!sending && !requesting && !listening && !listening_advisory && !disc_wildcarddest && !disc_objelems && !disc_elemobjs && !disc_regcomps)
  {
    printHelp();
    exit(0);
  }

  rtMessage_Create(&config);
  rtMessage_SetString(config, "appname", "sample_super");
  rtMessage_SetString(config, "uri", broker_uri);
  rtMessage_SetInt32(config, "start_router", 0);
  if(max_retries != 0)
    rtMessage_SetInt32(config, "max_retries", max_retries);

#ifdef WITH_SPAKE2
  if(isSecure)
  {
    char psk[LAF_PSK_LEN]={0};
    int ret = get_psk(psk,
#ifdef WITH_SPAKE2_TEST_PIN
      PSK_TEST
#else
      PSK_1
#endif
      );
    if(ret)
    {
      printf("failed to get spake2 psk: %d\n", ret);
      exit(5);
    }
    rtMessage_SetString(config, "spake2_psk", psk);
  }
#endif

  printf("connecting to router at %s\n", broker_uri);

  err = rtConnection_CreateWithConfig(&g_connection, config);

  rtMessage_Release(config);

  if(err != RT_OK)
  {
    printf("rtConnection_Create failed with error %s trying to connect to %s. Exiting.\n", rtStrError(err), broker_uri);
    exit(0);
  }

  if(sending)
  {
    while(wait > 0)
    {
      int i;
      for(i = 0; i < numTopics; ++i)
      {
          rtMessage m;
          char buff[200];
          snprintf(buff, 200, "%s (%d)", message, message_index++);
          printf("sending on topic %s\n", topics[i]);
          rtMessage_Create(&m);
          rtMessage_SetString(m, "msg", buff);
          err = rtConnection_SendMessage(g_connection, m, topics[i]);
          rtMessage_Release(m);
          if(err != RT_OK)
          {
            printf("rtConnection_SendMessage failed with error %s trying to send message to topic %s.\n", rtStrError(err), topics[i]);
          }
      }

      printf("waiting %d seconds\n", wait*5);
      sleep(5);
      wait--;
    }
  }
  
  if(requesting)
  {
    while(wait > 0)
    {
      int i;
      for(i = 0; i < numTopics; ++i)
      {
        rtMessage req;
        rtMessage res;
        char buff[200];
        snprintf(buff, 200, "%s (%d)", message, message_index++);
        rtMessage_Create(&req);
        rtMessage_SetString(req, "msg", buff);
        err = rtConnection_SendRequest(g_connection, req, topics[i], &res, 10000);
        rtMessage_Release(req);
        if (err == RT_OK)
        {
          char* p = NULL;
          uint32_t len = 0;
          rtMessage_ToString(res, &p, &len);
          printf("\tGot response::%.*s\n", len, p);
          free(p);
          rtMessage_Release(res);
        }
        else
        {
          printf("rtConnection_SendRequest failed with error %s trying to send message to topic %s.\n", rtStrError(err), topics[i]);
        }
      }
      printf("waiting %d seconds\n", wait*5);
      sleep(5);
      wait--;
    }
  }

  if(disc_wildcarddest)
  {
    discoverWildcardDestinations(disc_wildcarddest_name);
  }

  if(disc_objelems)
  {
    discoverObjectElements(disc_objelems_name);
  }

  if(disc_elemobjs)
  {
    discoverElementObjects(disc_elemobjs_name);
  }

  if(disc_regcomps)
  {
    discoverRegisteredComponents();
  }
  
  if(listening || listening_advisory)
  {
    int i;
    for(i = 0; i < numTopics; ++i)
    {
        if(topics[i][0] != '\0')
        {
            printf("listening on topic %s\n", topics[i]);
            rtConnection_AddListener(g_connection, topics[i], onMessage, g_connection);

            /*if aliases are asked for, apply all aliases to the first topic*/
            if(i == 0)
            {
              int j = 0;
              for(j = 0; j < numAlias; ++j)
                rtConnection_AddAlias(g_connection, topics[i], alias[j]);
            }
        }
        else
        {
            break;
        }
    }

    if(listening_advisory)
    {
        printf("listening on advisory topic %s\n", RTMSG_ADVISORY_TOPIC);
        rtConnection_AddListener(g_connection, RTMSG_ADVISORY_TOPIC, onMessage, g_connection);
    }

    while(wait > 0)
    {
      printf("waiting %d seconds\n", wait);
      sleep(1);
      wait--;
    }
  }

  sleep(1);

  rtConnection_Destroy(g_connection);

  printf("super_sample exiting\n");  

  return 0;
}

