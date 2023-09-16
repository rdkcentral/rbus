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
#include "rtMessage.h"
#include "rtTime.h"
#include "rtCipher.h"
#include "rtDebug.h"
#include "rtLog.h"
#include "rtEncoder.h"
#include "rtError.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"
#include "rtVector.h"
#include "rtConnection.h"
#include "rtAdvisory.h"
#include "rtrouter_diag.h"
#include "rtRoutingTree.h"
#include "rtMemory.h"
#include "rtrouteBase.h"
#include "rtm_discovery_api.h"
#include "local_benchmarking.h"
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <cjson/cJSON.h>

#ifdef ENABLE_RDKLOGGER
#include "rdk_debug.h"
#endif

#ifdef INCLUDE_BREAKPAD
#ifdef RDKC_BUILD
#include "breakpadwrap.h"
#else
#include "breakpad_wrapper.h"
#endif
#endif


rtVector gClients;
rtVector gListeners;
rtVector gRoutes;
rtRoutingTree gRoutingTree;
rtList g_discovery_result = NULL;
int g_enable_traffic_monitor = 0;
int is_running = 1;

#ifdef ENABLE_ROUTER_BENCHMARKING
#define MAX_TIMESTAMP_ENTRIES 2000
static struct timespec g_entry_exit_timestamps[MAX_TIMESTAMP_ENTRIES][2];
static int g_timestamp_index;
#endif

#if WITH_SPAKE2
char* g_spake2_L = NULL;
char* g_spake2_w0 = NULL;
#endif

static void
rtRouted_SendAdvisoryMessage(rtConnectedClient* clnt, rtAdviseEvent event);

#if MSG_ROUNDTRIP_TIME
static void
rtRouted_TransactionTimingDetails(rtMessageHeader header_details)
{
  char time_buff[64] = {0};
  rtTime_t timestamp = {0};
  time_t boottime = 0;
  rtTime_t uptime = {0};

  rtTime_Now(&uptime);
  boottime = time(NULL) - uptime.tv_sec; /* To calculate actual boot time of the device
                                            time(NULL) - Time since Epoch time(1st Jan 1970)
                                            uptime.tv_sec - Time since boot of device */
  rtLog_Info("=======================================================================");
  timestamp.tv_sec = header_details.T1 + boottime;
  rtTime_ToString(&timestamp, time_buff);
  rtLog_Info("Consumer : %s", header_details.topic);
  rtLog_Info("Provider : %s", header_details.reply_topic);
  rtLog_Info("Time at which consumer sends the request to daemon     : %s", time_buff);
  memset(time_buff, 0, sizeof(time_buff));
  timestamp.tv_sec = header_details.T2 + boottime;
  rtTime_ToString(&timestamp, time_buff);
  rtLog_Info("Time at which daemon receives the message from consumer: %s", time_buff);
  memset(time_buff, 0, sizeof(time_buff));
  timestamp.tv_sec = header_details.T3 + boottime;
  rtTime_ToString(&timestamp, time_buff);
  rtLog_Info("Time at which daemon writes to provider socket         : %s", time_buff);
  memset(time_buff, 0, sizeof(time_buff));
  timestamp.tv_sec = header_details.T4 + boottime;
  rtTime_ToString(&timestamp, time_buff);
  rtLog_Info("Time at which provider sends back the response         : %s", time_buff);
  memset(time_buff, 0, sizeof(time_buff));
  timestamp.tv_sec = header_details.T5 + boottime;
  rtTime_ToString(&timestamp, time_buff);
  rtLog_Info("Time at which daemon received the response             : %s", time_buff);
  rtLog_Info("Total duration                                         : %lld seconds", (long long int)(header_details.T5 - header_details.T1));
  rtLog_Info("=======================================================================");
}
#endif

static void
rtRouted_PrintHelp()
{
  printf("rtrouted [OPTIONS]...\n");
  printf("\t-f, --foreground          Run in foreground\n");
  printf("\t-d, --no-delay            Enabled debugging\n");
  printf("\t-l, --log-level <level>   Change logging level\n");
  printf("\t-r, --debug-route         Add a catch all route that dumps messages to stdout\n");
  printf("\t-s, --socket              [tcp://ip:port unix:///path/to/domain_socket]\n");
  printf("\t-h, --help                Print this help\n");
  exit(0);
}

static int validate_string(const char * ptr, int limit)
{
  int i;
  if(NULL == ptr)
    return -1;
  for(i = 0; i < limit; i++)
  {
    if((*ptr >= ' ') && (*ptr <= '~'))
    {
      ptr++;
      continue;
    }
    else if('\0' == *ptr)
      return 0;
    else
      return -1;
  }
  return -1; //string's larger than the limit.
}

static int
rtRouted_FileExists(char const* s)
{
  int ret;
  struct stat buf;

  if (!s || strlen(s) == 0) return 0;

  memset(&buf, 0, sizeof(buf));
  ret = stat(s, &buf);

  return ret == 0 ? 1 : 0;
}

static void
rtRouted_PrintClientInfo(rtConnectedClient* clnt)
{
  size_t i;

  if(NULL == clnt)
  {
    rtLog_Warn("client is NULL");
    return;
  }

  for (i = 0; i < rtVector_Size(gRoutes);)
  {
    rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);
    if (route->subscription && route->subscription->client == clnt)
    {
      rtLog_Warn("client identity:%s name:%s", clnt->ident,route->expression);
      break;
    }
    else
      i++;
  }
}

static rtError
rtRouted_ReadTextFile(char const* fname, char** content)
{
  FILE* pf;
  rtError err = RT_OK;
  size_t sz;
  *content = NULL;
  rtLog_Info("reading file %s", fname);
  pf = fopen(fname, "r");
  if (pf)
  {
    fseek(pf, 0L, SEEK_END);
    sz = (size_t)ftell(pf);
    rewind(pf);
    *content = rt_malloc(sz+1);
    if(fread(*content, 1, sz, pf) != sz)
    {
      free(*content);
      *content = NULL;
      rtLog_Error("failed to read file %s. %s", fname, strerror(errno));
      err = RT_FAIL;
    }
    (*content)[sz] = 0;
    fclose(pf);
  }
  else
  {
    rtLog_Error("failed to open file %s. %s", fname, strerror(errno));
    err = RT_FAIL;
  }
  return err;
}

static rtError
rtRouted_ParseConfig(char const* fname)
{
  int       i;
  int       n = 0;
  char*     buff = NULL;
  cJSON*    json = NULL;
  cJSON*    listeners = NULL;
  cJSON*    loglevel = NULL;
#if WITH_SPAKE2
  cJSON*    spake2plus = NULL;
#endif
  if (!fname || strlen(fname) == 0)
  {
    rtLog_Warn("cannot prase empty configuration file");
    return RT_FAIL;
  }

  rtLog_Debug("parsing configuration from file %s", fname);

  if(rtRouted_ReadTextFile(fname, &buff) != RT_OK)
    return RT_FAIL;

  json = cJSON_Parse(buff);
  free(buff);

  if (!json)
  {
    rtLog_Error("error parising configuration file");

    char const* p = cJSON_GetErrorPtr();
    if (p)
    {
      char const* end = (buff + strlen(buff));
      int n = (int) (end - p);
      if (n > 64)
        n = 64;
      rtLog_Error("%.*s\n", n, p);
    }

    exit(1);
  }
  else
  {
    listeners = cJSON_GetObjectItem(json, "listeners");
    if (listeners)
    {
      int indefinite_retry = 0;
      for (i = 0, n = cJSON_GetArraySize(listeners); i < n; ++i)
      {
        cJSON* item = cJSON_GetArrayItem(listeners, i);
        if (item)
        {
          cJSON* uri = cJSON_GetObjectItem(item, "uri");
          if (uri)
          {
            rtListener* listener = NULL;
            rtRouteBase_BindListener(uri->valuestring, 1, indefinite_retry, &listener);
            rtVector_PushBack(gListeners, listener);
            indefinite_retry  = 1;
          }
        }
      }
    }
#if WITH_SPAKE2
    spake2plus = cJSON_GetObjectItem(json, "spake2plus");
    if (spake2plus)
    {
      cJSON* item;

      rtLog_Info("parsing spake2 config");

      item = cJSON_GetObjectItem(spake2plus, "L");
      if(item)
      {
        if(strncmp(item->valuestring, "file:", 5) == 0)
        {
          if(rtRouted_ReadTextFile(item->valuestring+5, &g_spake2_L) != RT_OK)
            g_spake2_L = NULL;
        }
        else
        {
          g_spake2_L = rt_malloc(strlen(item->valuestring)+1);
          strcpy(g_spake2_L, item->valuestring);
        }
      }

      item = cJSON_GetObjectItem(spake2plus, "w0");
      if(item)
      {
        if(strncmp(item->valuestring, "file:", 5) == 0)
        {
          if(rtRouted_ReadTextFile(item->valuestring+5, &g_spake2_w0) != RT_OK)
            g_spake2_w0 = NULL;

        }
        else
        {
          g_spake2_w0 = rt_malloc(strlen(item->valuestring)+1);
          strcpy(g_spake2_w0, item->valuestring);
        }
      }

      rtLog_Debug("g_spake2_L=%s", g_spake2_L ? g_spake2_L : "(null)");
      rtLog_Debug("g_spake2_w0=%s", g_spake2_w0 ? g_spake2_w0 : "(null)");

      if(g_spake2_L && g_spake2_w0)
      {
        rtLog_Info("spake2plus config valid");
      }
      else
      {
        rtLog_Warn("cannot enable spake2plus due to missing config param(s): %s", 
          (!g_spake2_L && !g_spake2_w0) ? "L and w0" :
          (!g_spake2_L) ? "L" : "w0");
        if(g_spake2_L)
        {
          free(g_spake2_L);
          g_spake2_L = NULL;
        }
        if(g_spake2_w0)
        {
          free(g_spake2_w0);
          g_spake2_w0 = NULL;
        }
      }
    }
#endif

    loglevel = cJSON_GetObjectItem(json, "log_level");
    if(loglevel)
      rtLog_SetLevel(rtLogLevelFromString(loglevel->valuestring));

    cJSON_Delete(json);
  }
  return RT_OK;
}

static rtError
rtRouted_AddRoute(rtRouteMessageHandler handler, char const* exp, rtSubscription* subscription)
{
  rtRouteEntry* route = (rtRouteEntry *) rt_malloc(sizeof(rtRouteEntry));
  route->subscription = subscription;
  route->message_handler = handler;
  strncpy(route->expression, exp, RTMSG_MAX_EXPRESSION_LEN);
  rtVector_PushBack(gRoutes, route);
  rtLog_Debug("AddRoute route=[%p] address=[%s] expression=[%s]", route, subscription->client->ident, exp);
  rtRoutingTree_AddTopicRoute(gRoutingTree, exp, (void *)route, 0/*ignfore duplicate entry*/);
  return RT_OK;
}

static bool
rtRouted_ShouldLimitLog(char const* pTopicExpression)
{
    static const int LOGGING_TIMELIMIT_DUP_ENTRY = 60000; /* 60s interval */
    static char storedTopicExp[RTMSG_MAX_EXPRESSION_LEN] = "";
    static rtTime_t storedTime = {0};
    rtTime_t currentTime = {0};

    if (strncmp(storedTopicExp, pTopicExpression, RTMSG_MAX_EXPRESSION_LEN) == 0)
    {
        int timeDiff = -2;
        rtTime_Now(&currentTime);
        timeDiff = rtTime_Elapsed (&storedTime, &currentTime);

        if (timeDiff >= LOGGING_TIMELIMIT_DUP_ENTRY)
        {
            rtTime_Now(&storedTime);
            return true;
        }

        return false;
    }
    else
    {
        rtTime_Now(&storedTime);
        snprintf(storedTopicExp, RTMSG_MAX_EXPRESSION_LEN - 1, "%s", pTopicExpression);
        return true;
    }
}

static rtError
rtRouted_AddAlias(char const* exp, rtRouteEntry * route)
{
  rtError rc = RT_OK;
  rtLog_Debug("AddAlias route=[%p] address=[%s] expression=[%s] alias=[%s]", route, route->subscription->client->ident, route->expression, exp);
  rc = rtRoutingTree_AddTopicRoute(gRoutingTree, exp, (void *)route, 1/*error if duplicate entry*/);
  if (RT_ERROR_DUPLICATE_ENTRY == rc)
      if (rtRouted_ShouldLimitLog(exp))
          rtLog_Warn("Rejecting Duplicate Registration of [%s] by [%s] thro [%s]", exp, route->expression, route->subscription->client->ident);

  return rc;
}

  static rtError
rtRouted_ClearRoute(rtRouteEntry * route)
{
  rtVector_RemoveItem(gRoutes, route, NULL);
  free(route->subscription);
  rtLog_Debug("Clearing route %s", route->expression);
  rtRoutingTree_RemoveRoute(gRoutingTree, (void*)route);
  free(route);
  return RT_OK;
}

static rtError
rtRouted_ClearClientRoutes(rtConnectedClient* clnt)
{
  size_t i;
  for (i = 0; i < rtVector_Size(gRoutes);)
  {
    rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);
    if (route->subscription && route->subscription->client == clnt)
      rtRouted_ClearRoute(route);
    else
      i++;
  }
  //rtRoutingTree_LogStats();
  return RT_OK;
}

static void
rtConnectedClient_Destroy(rtConnectedClient* clnt)
{
  rtRouted_ClearClientRoutes(clnt);

  if (clnt->fd != -1)
    close(clnt->fd);

  if (clnt->read_buffer)
    free(clnt->read_buffer);

  if (clnt->send_buffer)
    free(clnt->send_buffer);

#if WITH_SPAKE2
  if (clnt->cipher)
    rtCipher_Destroy(clnt->cipher);
  if (clnt->encryption_key)
    free(clnt->encryption_key);
  if (clnt->encryption_buffer)
    free(clnt->encryption_buffer);
#endif

  free(clnt);
}

static rtError
rtRouted_SendMessage(rtMessageHeader * request_hdr, rtMessage message, rtConnectedClient* skipClient)
{
  rtError ret = RT_OK;
  ssize_t bytes_sent;
  uint8_t* buffer = NULL;
  uint32_t size;
  rtConnectedClient * client = NULL;
  rtList list;
  rtListItem item;
  int found_dest = 0;

  rtMessage_ToByteArray(message, &buffer, &size);
  request_hdr->payload_length = size;

  /*Find the route to populate control_id field.*/
  rtRouteEntry *route = NULL;
  rtRoutingTree_GetTopicRoutes(gRoutingTree, request_hdr->topic, &list);
  if(list)
  {
    rtList_GetFront(list, &item);
    while(item)
    {
      rtTreeRoute* treeRoute;
      rtListItem_GetData(item, (void**)&treeRoute);
      rtListItem_GetNext(item, &item);
      route = treeRoute->route;
      if(route)
      {
        rtLog_Debug("SendMessage topic=%s expression=%s", request_hdr->topic, route->expression);
        found_dest = 1;
        request_hdr->control_data = route->subscription->id;
        client = route->subscription->client;
        if(client != skipClient)
        {
          rtMessageHeader_Encode(request_hdr, client->send_buffer);
          struct iovec send_vec[] = {{client->send_buffer, request_hdr->header_length}, {(void *)buffer, size}};
          struct msghdr send_hdr = {NULL, 0, send_vec, 2, NULL, 0, 0};
          do
          {
            bytes_sent = sendmsg(client->fd, &send_hdr, MSG_NOSIGNAL);
            if (bytes_sent == -1)
            {
              if (errno == EBADF)
                ret = rtErrorFromErrno(errno);
              else
              {
                rtLog_Warn("error forwarding message to client. %d %s", errno, strerror(errno));
                rtRouted_PrintClientInfo(client);
                if(skipClient)
                    rtRouted_PrintClientInfo(skipClient);
                ret = RT_FAIL;
              }
              break;
            }

          } while(0);
        }
      }
    }
  }
  if(!found_dest)
  {
    if(strcmp(request_hdr->topic, "_RTROUTED.ADVISORY"))
    {
        ret = RT_FAIL;
        rtLog_Warn("Could not find route to destination. Topic=%s ", request_hdr->topic);
    }
  }
  rtMessage_FreeByteArray(buffer);
  return ret;
}


rtError rtRouted_TrafficMonitorLog(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n, rtSubscription* subscription)
{
  static FILE* file = NULL;
  static int counter = 0;

  (void) sender;
  (void) subscription;

  if(!file)
  {
    file = fopen("/tmp/rtrouted_traffic_monitor", "a");
    if(!file)
    {
      rtLog_Warn("Failed to open traffix monitor log");
      return RT_FAIL;
    }
  }

  fprintf(file, "%d %s %s %s %s %d [", 
    counter++, 
    sender->inbox, 
    subscription->client->inbox, 
    hdr->topic, 
    hdr->reply_topic, 
    n);

  if(buff && n)
  {
    int i;
    int isprint = 0;
    for(i = 0; i < n; ++i)
    {
      if(buff[i] >= 32 && buff[i] <= 127)
      {
        if(!isprint)
        {
          isprint = 1;
          fprintf(file, "\"");
        }
        fprintf(file, "%c", buff[i]);
      }
      else
      {
        if(isprint)
        {
          isprint = 0;
          fprintf(file, "\"");
        }
        fprintf(file, " 0x%x ", buff[i]);
      }
    }
    if(isprint)
    {
      isprint = 0;
      fprintf(file, "\"");
    }
  }
  fprintf(file, "]\n");
  fflush(file);
  return RT_OK;
}

static rtError
rtRouted_ForwardMessage(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n, rtSubscription* subscription)
{
  ssize_t bytes_sent;

  (void) sender;

  if(1 == g_enable_traffic_monitor)
    rtRouted_TrafficMonitorLog(sender, hdr, buff, n, subscription);

  rtMessageHeader new_header;
  rtMessageHeader_Init(&new_header);
  new_header.version = hdr->version;
  new_header.header_length = hdr->header_length;
  new_header.sequence_number = hdr->sequence_number;
  new_header.control_data = subscription->id;
  new_header.payload_length = hdr->payload_length;
  new_header.topic_length = hdr->topic_length;
  new_header.reply_topic_length = hdr->reply_topic_length;
  new_header.flags = hdr->flags;
  strncpy(new_header.topic, hdr->topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
  strncpy(new_header.reply_topic, hdr->reply_topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
#ifdef MSG_ROUNDTRIP_TIME
  new_header.T1 = hdr->T1;
  new_header.T2 = hdr->T2;
  new_header.T3 = hdr->T3;
  new_header.T4 = hdr->T4;
  new_header.T5 = hdr->T5;
#endif

#ifdef WITH_SPAKE2
  if(hdr->flags & rtMessageFlags_Encrypted)
  {
    uint32_t decryptedLength;

    rtLog_Debug("received encrypted message: key=%p topic=%s", sender->encryption_key, hdr->topic);

    if(!sender->encryption_key)
    {
        rtLog_Info("no encryption key found, cannot decrypt message");
        return RT_FAIL;
    }

    if(rtCipher_DecryptWithKey( sender->encryption_key, 
                                buff, 
                                n, 
                                sender->encryption_buffer, 
                                RTMSG_CLIENT_READ_BUFFER_SIZE, 
                                &decryptedLength ) != RT_OK)
    { 
      rtLog_Error("failed to decrypt message");
      return RT_FAIL;
    }

    buff = sender->encryption_buffer;
    n = decryptedLength;
    new_header.payload_length = decryptedLength;
    new_header.flags &= ~rtMessageFlags_Encrypted;
  }

  /*since each client has a unique key we must re-encrypt using its key*/
  if(subscription->client->encryption_key)
  {
    uint32_t encryptedLength;

    rtLog_Debug("sending encrypted message");

    if(rtCipher_EncryptWithKey( subscription->client->encryption_key, 
                                buff, 
                                n, 
                                subscription->client->encryption_buffer, 
                                RTMSG_CLIENT_READ_BUFFER_SIZE, 
                                &encryptedLength ) != RT_OK)
    { 
      rtLog_Error("failed to encrypt message");
      return RT_FAIL;
    }
    buff = subscription->client->encryption_buffer;
    n = encryptedLength;
    new_header.payload_length = encryptedLength;
    new_header.flags |= rtMessageFlags_Encrypted;
  }
#endif

  rtMessageHeader_Encode(&new_header, subscription->client->send_buffer);

  //rtDebug_PrintBuffer("fwd header", (uint8_t*) buff, n);
  struct iovec send_vec[] = {{subscription->client->send_buffer, new_header.header_length}, {(void *)buff, (size_t)n}};
  struct msghdr send_hdr = {NULL, 0, send_vec, 2, NULL, 0, 0};

  bytes_sent = sendmsg(subscription->client->fd, &send_hdr, MSG_NOSIGNAL);
  if (bytes_sent == -1)
  {
    if (errno == EBADF)
    {
      return rtErrorFromErrno(errno);
    }
    else
    {
      rtLog_Warn("error forwarding message to client. %d %s", errno, strerror(errno));
      rtRouted_PrintClientInfo(subscription->client);
    }
    return RT_FAIL;
  }
  return RT_OK;
}

static void prep_reply_header_from_request(rtMessageHeader *reply, const rtMessageHeader *request)
{
  rtMessageHeader_Init(reply);
  reply->version = request->version;
  reply->header_length = request->header_length;
  reply->sequence_number = request->sequence_number;
  reply->flags = rtMessageFlags_Response;

  strncpy(reply->topic, request->reply_topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
  strncpy(reply->reply_topic, request->topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
  reply->topic_length = request->reply_topic_length;
  reply->reply_topic_length = request->topic_length;
#ifdef MSG_ROUNDTRIP_TIME
  reply->T1 = request->T1;
  reply->T2 = request->T2;
  reply->T3 = request->T3;
  reply->T4 = request->T4;
  reply->T5 = request->T5;
#endif
}

static void
rtRouted_OnMessageSubscribe(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  char const* expression = NULL;
  uint32_t route_id = 0;
  uint32_t i = 0;
  int32_t add_subscrption = 0;
  rtMessage m;
  rtMessage response = NULL;
  rtError rc = RT_OK;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad Subscribe message");
    rtLog_Warn("Sender %s", sender->ident);
    rc = RT_ERROR;
  }
  else
  {
    if((RT_OK == rtMessage_GetInt32(m, "add", &add_subscrption)) &&
       (RT_OK == rtMessage_GetString(m, "topic", &expression)) &&
       (RT_OK == rtMessage_GetInt32(m, "route_id", (int32_t *)&route_id)) &&
       (0 == validate_string(expression, RTMSG_MAX_EXPRESSION_LEN)))
    {
      if(1 == add_subscrption)
      {
        for (i = 0; i < rtVector_Size(gRoutes); i++)
        {
          rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);
          if (route->subscription && (route->subscription->client == sender) && (route->subscription->id == route_id))
          {
            rc = rtRouted_AddAlias(expression, route);
            break;
          }
        }
        if(i == rtVector_Size(gRoutes))
        {
          rtSubscription* subscription = (rtSubscription *) rt_malloc(sizeof(rtSubscription));
          subscription->id = route_id;
          subscription->client = sender;
          rc = rtRouted_AddRoute(rtRouted_ForwardMessage, expression, subscription);

          if(strstr(expression, ".INBOX.") && sender->inbox[0] == '\0')
          {
              strncpy(sender->inbox, expression, RTMSG_HEADER_MAX_TOPIC_LENGTH);
              rtLog_Debug("init client inbox to %s", sender->inbox);
              rtRouted_SendAdvisoryMessage(sender, rtAdviseClientConnect);
          }
        }
      }
      else
      {
        int route_removed = 0;
        for (i = 0; i < rtVector_Size(gRoutes); i++)
        {
          rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);

          if((route->subscription) && (0 == strncmp(route->expression, expression, RTMSG_MAX_EXPRESSION_LEN)) && (route->subscription->client == sender))
          {
            rtRouted_ClearRoute(route);
            route_removed = 1;
            break;
          }
        }
        if(0 == route_removed)
        {
          //Not a route. Is it an alias?
          rtLog_Debug("Removing alias %s", expression);
          rtRoutingTree_RemoveTopic(gRoutingTree, expression);
        }
      }
    }
    else
    {
      rtLog_Warn("Bad subscription message from %s", sender->ident);
      rc = RT_ERROR_INVALID_ARG;
    }
  }
  rtMessage_Release(m);

  /* Send Response */
  if(hdr->flags & rtMessageFlags_Request)
  {
      rtMessage_Create(&response);
      rtMessage_SetInt32(response, "result", rc);
      rtMessageHeader new_header;
      prep_reply_header_from_request(&new_header, hdr);
      if(RT_OK != rtRouted_SendMessage(&new_header, response, NULL))
          rtLog_Info("%s() Response couldn't be sent.", __func__);
      rtMessage_Release(response);
  }
}

static void
rtRouted_OnMessageHello(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  char const* inbox = NULL;
  rtMessage m;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad Hello message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }
  rtMessage_GetString(m, "inbox", &inbox);

  rtSubscription* subscription = (rtSubscription *) rt_malloc(sizeof(rtSubscription));
  subscription->id = 0;
  subscription->client = sender;
  rtRouted_AddRoute(rtRouted_ForwardMessage, inbox, subscription);

  rtMessage_Release(m);
  
  (void)hdr;
}

#ifdef MSG_ROUNDTRIP_TIME
static void
rtRouted_OnMessageTimeOut(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtMessage m;
  char const* topic = NULL;
  char const* reply_topic = NULL;
  rtMessageHeader header;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }
  rtMessageHeader_Init(&header);
  rtMessage_GetInt32(m, "T1", (int32_t *)&header.T1);
  rtMessage_GetInt32(m, "T2", (int32_t *)&header.T2);
  rtMessage_GetInt32(m, "T3", (int32_t *)&header.T3);
  rtMessage_GetInt32(m, "T4", (int32_t *)&header.T4);
  rtMessage_GetInt32(m, "T5", (int32_t *)&header.T5);
  rtMessage_GetString(m, "topic", &topic);
  rtMessage_GetString(m, "reply_topic", &reply_topic);
  snprintf(header.topic, sizeof(header.topic), "%s", topic);
  snprintf(header.reply_topic, sizeof(header.reply_topic), "%s", reply_topic);
  rtLog_Info("Consumer exist but the request timed out");
  rtRouted_TransactionTimingDetails(header);

  rtMessage_Release(m);
  (void)hdr;
}
#endif

static void
rtRouted_OnMessageDiscoverRegisteredComponents(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  uint32_t i = 0;
  rtMessage response = NULL;

  if((hdr->flags & rtMessageFlags_Request) && (RT_OK == rtMessage_Create(&response)))
  {
      int counter = 0, pass = 0;
      for (pass = 0; pass <= 1; pass ++)
      {
          for (i = 0; i < rtVector_Size(gRoutes); i++)
          {
              rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);
              if((route) && (strcmp(route->expression, "")) && ('_' != route->expression[0]))
              {
                  if(pass == 0)
                      counter++;
                  else
                      rtMessage_AddString(response, RTM_DISCOVERY_ITEMS, route->expression);
              }
          }
          if (pass == 0)
              rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, counter);
      }

      rtMessageHeader new_header;
      prep_reply_header_from_request(&new_header, hdr);
      if(RT_OK != rtRouted_SendMessage(&new_header, response, NULL))
          rtLog_Info("%s() Response couldn't be sent.", __func__);
      rtMessage_Release(response);
  }
  else
  {
      rtLog_Error("Cannot create response message to registered components.");
  }

  (void)sender;
  (void)buff;
  (void)n;
}

static void
rtRouted_OnMessageDiscoverWildcardDestinations(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  char const* expression = NULL;
  rtMessage m, response = NULL;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad DiscoverWildcard message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }

  if((hdr->flags & rtMessageFlags_Request) && (RT_OK == rtMessage_Create(&response)))
  {
    /*Construct the outbound message.*/
    if(RT_OK == rtMessage_GetString(m, RTM_DISCOVERY_EXPRESSION, &expression) && (NULL != expression) &&
        (0 == validate_string(expression, RTMSG_MAX_EXPRESSION_LEN)))
    {
      size_t count = 0;
      rtListItem item;
      rtMessage_SetInt32(response, RTM_DISCOVERY_RESULT, RT_OK);
      rtRoutingTree_ResolvePartialPath(gRoutingTree, expression, g_discovery_result);
      rtList_GetSize(g_discovery_result, &count);
      rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, (int32_t)count);
      rtList_GetFront(g_discovery_result, &item);
      while(item)
      {
        const char* topic = NULL;
        rtListItem_GetData(item, (void**)&topic);
        rtListItem_GetNext(item, &item);
        if(topic)
          rtMessage_AddString(response, RTM_DISCOVERY_ITEMS, topic);
      }
      rtList_RemoveAllItems(g_discovery_result, NULL);
    }
    else
    {
      rtMessage_SetInt32(response, RTM_DISCOVERY_RESULT, RT_ERROR);
      rtLog_Error("Bad discovery message.");
    }
    /* Send this message back to the requestor.*/ 
    rtMessageHeader new_header;
    prep_reply_header_from_request(&new_header, hdr);
    if(RT_OK != rtRouted_SendMessage(&new_header, response, NULL))
      rtLog_Info("%s() Response couldn't be sent.", __func__);
    rtMessage_Release(response);
  }
  else
    rtLog_Error("Cannot create response message to discovery.");

  rtMessage_Release(m);

  (void)sender;
}

static void
rtRouted_OnMessageDiscoverObjectElements(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtMessage m = NULL;
  rtMessage response = NULL;
  char const* expression = NULL;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad DiscoverObjectElements message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }

  if((hdr->flags & rtMessageFlags_Request) && (RT_OK == rtMessage_Create(&response)))
  {
    if(RT_OK == rtMessage_GetString(m, RTM_DISCOVERY_EXPRESSION, &expression) && (NULL != expression))
    {
      unsigned int i;
      rtList list;
      rtListItem item;
      size_t count = 0;
      int found = 0;
      rtRouteEntry* route = NULL;
      rtLog_Debug("ElementEnumeration expression=%s", expression);
      for (i = 0; i < rtVector_Size(gRoutes); i++)
      {
        route = (rtRouteEntry *) rtVector_At(gRoutes, i);
        if(0 == strncmp(expression, route->expression, RTMSG_MAX_EXPRESSION_LEN))
        {
          //rtLog_Debug("ElementEnumeration found route for expression=%s", expression);
          rtRoutingTree_GetRouteTopics(gRoutingTree, (void *)route, &list);
          //rtLog_Debug("ElementEnumeration route has %s", expression);
          found = 1;
          break;
        }
      }

      if(!found)
      {
        //rtLog_Debug("ElementEnumeration couldn't find route for expression=%s", expression);
        rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, 0);
      }
      else
      {
        rtList_GetSize(list, &count);
        rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, (int32_t)count);
        //rtLog_Debug("ElementEnumeration route has %d elements", (int32_t)count);

        rtList_GetFront(list, &item);
        while(item)
        {
            rtTreeTopic* treeTopic;
            rtListItem_GetData(item, (void**)&treeTopic);
            rtMessage_AddString(response, RTM_DISCOVERY_ITEMS, treeTopic->fullName);
            //rtLog_Debug("ElementEnumeration add element=%s", treeTopic->fullName);
            rtListItem_GetNext(item, &item);
        }
      }
      rtMessageHeader new_header;
      prep_reply_header_from_request(&new_header, hdr);
      if (RT_OK != rtRouted_SendMessage(&new_header, response, NULL))
        rtLog_Info("%s() Response couldn't be sent.", __func__);
      rtMessage_Release(response);   
    }
  }
  else
    rtLog_Error("Cannot create response message to registered components.");
  rtMessage_Release(m);

  (void)sender;
  (void)hdr;
}

static void
rtRouted_OnMessageDiscoverElementObjects(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtMessage msgIn = NULL;
  rtMessage response = NULL;
  char const *expression = NULL;
  int i;

  if(RT_OK != rtMessage_FromBytes(&msgIn, buff, n))
  {
    rtLog_Warn("Bad DiscoverElementObjects message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }

  if ((hdr->flags & rtMessageFlags_Request) && (RT_OK == rtMessage_Create(&response)))
  {
    int length = 0;
    if (RT_OK == rtMessage_GetInt32(msgIn, RTM_DISCOVERY_COUNT, &length) && (0 < length))
    {
      rtMessage_SetInt32(response, RTM_DISCOVERY_RESULT, RT_OK);
      for (i = 0; i < length; i++)
      {
        if (RT_OK == rtMessage_GetStringItem(msgIn, RTM_DISCOVERY_ITEMS, i, &expression) && (NULL != expression))
        {
          rtList routes;
          rtListItem item;
          int set = 0;
          rtRoutingTree_GetTopicRoutes(gRoutingTree, expression, &routes);
          if(routes)
          {
            size_t count;
            rtList_GetSize(routes, &count);

            rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, (int32_t)count);
            rtList_GetFront(routes, &item);
            while(item)
            {
              rtTreeRoute* treeRoute;
              rtRouteEntry *route;
              rtListItem_GetData(item, (void**)&treeRoute);
              rtListItem_GetNext(item, &item);
              route = treeRoute->route;
              if(route)
              {
                rtMessage_AddString(response, RTM_DISCOVERY_ITEMS, route->expression);
                set = 1;
              }
            }
          }
          if(!set)
          {
            rtMessage_SetInt32(response, RTM_DISCOVERY_COUNT, 0);
          }
        }
        else
        {
          rtLog_Warn("Bad trace request. Failed to extract element name.");
          rtMessage_Release(response); //This was contaminated because we already added a 'success' result to this message.
          if (RT_OK == rtMessage_Create(&response))
          {
            rtMessage_SetInt32(response, RTM_DISCOVERY_RESULT, RT_ERROR);
            break;
          }
          else
          {
            rtLog_Error("Cannot create response message to trace request");
            rtMessage_Release(msgIn);
            return;
          }
        }
      }
    }
    else
    {
      rtLog_Warn("Bad trace request. Could not get length / bad length.");
      rtMessage_SetInt32(response, RTM_DISCOVERY_RESULT, RT_ERROR);
    }
    
    rtMessageHeader new_header;
    prep_reply_header_from_request(&new_header, hdr);
    if (RT_OK != rtRouted_SendMessage(&new_header, response, NULL))
      rtLog_Info("Response to trace request couldn't be sent.");
    rtMessage_Release(response);
  }
  else
    rtLog_Error("Cannot create response message to trace request");
  rtMessage_Release(msgIn);

  (void)sender;
}

static void
rtRouted_OnMessageDiagnostics(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtMessage msg;
  const char * cmd;

  rtMessage_FromBytes(&msg, buff, n);

  rtMessage_GetString(msg, RTROUTER_DIAG_CMD_KEY, &cmd);

  if(0 == strncmp(RTROUTER_DIAG_CMD_ENABLE_VERBOSE_LOGS, cmd, sizeof(RTROUTER_DIAG_CMD_ENABLE_VERBOSE_LOGS)))
    rtLog_SetLevel(RT_LOG_DEBUG);
  else if(0 == strncmp(RTROUTER_DIAG_CMD_DISABLE_VERBOSE_LOGS, cmd, sizeof(RTROUTER_DIAG_CMD_DISABLE_VERBOSE_LOGS)))
    rtLog_SetLevel(RT_LOG_INFO);
  else if(0 == strncmp(RTROUTER_DIAG_CMD_LOG_ROUTING_STATS, cmd, sizeof(RTROUTER_DIAG_CMD_LOG_ROUTING_STATS)))
    rtRoutingTree_LogStats(gRoutingTree);
  else if(0 == strncmp(RTROUTER_DIAG_CMD_LOG_ROUTING_TOPICS, cmd, sizeof(RTROUTER_DIAG_CMD_LOG_ROUTING_TOPICS)))
    rtRoutingTree_LogTopicTree(gRoutingTree);
  else if(0 == strncmp(RTROUTER_DIAG_CMD_LOG_ROUTING_ROUTES, cmd, sizeof(RTROUTER_DIAG_CMD_LOG_ROUTING_ROUTES)))
    rtRoutingTree_LogRouteList(gRoutingTree);
  else if(0 == strncmp(RTROUTER_DIAG_CMD_ENABLE_TRAFFIC_MONITOR, cmd, sizeof(RTROUTER_DIAG_CMD_ENABLE_TRAFFIC_MONITOR)))
    g_enable_traffic_monitor = 1;
  else if(0 == strncmp(RTROUTER_DIAG_CMD_DISABLE_TRAFFIC_MONITOR, cmd, sizeof(RTROUTER_DIAG_CMD_DISABLE_TRAFFIC_MONITOR)))
    g_enable_traffic_monitor = 0;
  else if(0 == strncmp(RTROUTER_DIAG_CMD_ADD_NEW_LISTENER, cmd, sizeof(RTROUTER_DIAG_CMD_ADD_NEW_LISTENER)))
  {
    const char* socket = NULL;
    rtListener* listener = NULL;

    /* Get the socket */
    rtMessage_GetString(msg, RTROUTER_DIAG_CMD_VALUE, &socket);

    if (NULL != socket)
    {
      /* Have a log info for ref */
      rtLog_Info("Received request to listen additionally on %s", socket);

      if ((RT_OK == rtRouteBase_BindListener(socket, 1, 0, &listener)) && (NULL != listener))
      {
        rtLog_Warn("Updated rtrouted to listen additionally on %s", socket);
        rtVector_PushBack(gListeners, listener);
      }
    }
  }
  else if(0 == strncmp(RTROUTER_DIAG_CMD_HEARTBEAT, cmd, sizeof(RTROUTER_DIAG_CMD_HEARTBEAT)))
  {
    rtLog_Debug("rtrouted process is running fine");
  }
  else if(0 == strncmp(RTROUTER_DIAG_CMD_DUMP_BENCHMARKING_DATA, cmd, sizeof(RTROUTER_DIAG_CMD_DUMP_BENCHMARKING_DATA)))
  {
    benchmark_print_stats("diagnostics");
#ifdef ENABLE_ROUTER_BENCHMARKING
    printf("--- Start tainted packet timestamp dump (%d entries) ---\n", g_timestamp_index);
    int i;
    for(i = 0; i <= g_timestamp_index; i++)
      printf("Entry:  %ld sec, %ld ns. Exit:  %ld sec, %ld ns.\n",
          g_entry_exit_timestamps[i][0].tv_sec, g_entry_exit_timestamps[i][0].tv_nsec,
          g_entry_exit_timestamps[i][1].tv_sec, g_entry_exit_timestamps[i][1].tv_nsec);
    printf("--- End tainted packet timestamp dump ---\n");
#endif
  }
  else if(0 == strncmp(RTROUTER_DIAG_CMD_RESET_BENCHMARKING_DATA, cmd, sizeof(RTROUTER_DIAG_CMD_RESET_BENCHMARKING_DATA)))
  {
    benchmark_reset();
#ifdef ENABLE_ROUTER_BENCHMARKING
    g_timestamp_index = 0;
#endif
  }
  else if(0 == strncmp(RTROUTER_DIAG_CMD_SHUTDOWN, cmd, sizeof(RTROUTER_DIAG_CMD_SHUTDOWN)))
  {
    is_running = 0;
  }
  else
    rtLog_Error("Unknown diag command: %s", cmd);
  rtMessage_Release(msg);
  (void)sender;
  (void)hdr;
}

static void
rtRouted_SendAdvisoryMessage(rtConnectedClient* clnt, rtAdviseEvent event)
{
  rtMessage msg;
  rtMessageHeader hdr;

  rtMessage_Create(&msg);
  rtMessage_SetInt32(msg, RTMSG_ADVISE_EVENT, event);
  rtMessage_SetString(msg, RTMSG_ADVISE_INBOX, clnt->inbox);

  rtMessageHeader_Init(&hdr);
  hdr.topic_length = strlen(RTMSG_ADVISORY_TOPIC);
  strncpy(hdr.topic, RTMSG_ADVISORY_TOPIC, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);

  rtLog_Debug("Sending advisory message");
  if (RT_OK != rtRouted_SendMessage(&hdr, msg, clnt))
    rtLog_Info("Failed to send advisory");

  rtMessage_Release(msg);
}

#ifdef WITH_SPAKE2

static rtError
rtRouted_CreateSpake2CipherInstance(rtCipher** cipher)
{
  rtError err;
  rtMessage config;

  if(!g_spake2_L || !g_spake2_w0)
  {
    rtLog_Error("cannot create spake2 cipher without L and w0 config values");
    return RT_ERROR;
  }

  rtMessage_Create(&config);
  rtMessage_SetString(config, RT_CIPHER_SPAKE2_VERIFY_L, g_spake2_L);
  rtMessage_SetString(config, RT_CIPHER_SPAKE2_VERIFY_W0, g_spake2_w0);
  rtMessage_SetBool(config, RT_CIPHER_SPAKE2_IS_SERVER, true);

  err = rtCipher_CreateCipherSpake2Plus(cipher, config);

  rtMessage_Release(config);

  if(err != RT_OK)
  {
    *cipher = NULL;
    rtLog_Error("failed to initialize spake2 based cipher");
    //todo return error to client
  }
  return err;
}

static void
rtRouted_OnMessageKeyExchange(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtError err;
  rtMessage msg;
  const char * type = NULL;
  rtMessage response = NULL;

  (void)hdr;

  rtMessage_FromBytes(&msg, buff, n);
  err = rtMessage_GetString(msg, "type", &type);
  if (err != RT_OK || !type)
    return;

  if (strcmp(type, "spake2plus") == 0)
  {
    if (!sender->cipher)
    {
      if(rtRouted_CreateSpake2CipherInstance(&sender->cipher) != RT_OK)
        return;/*TODO: send error message back to client*/
    }

    if(RT_OK == rtCipher_RunKeyExchangeServer(sender->cipher, msg, &response, &sender->encryption_key))
    {
      if(response)
      {
        rtLog_Debug("sending key exchange response");
        rtMessageHeader new_header;
        prep_reply_header_from_request(&new_header, hdr);
        if(rtRouted_SendMessage(&new_header, response, NULL) != RT_OK)
          rtLog_Error("key exchange response couldn't be sent.");
      }

      if(sender->encryption_key)
      {
        if(!sender->encryption_buffer)
        {
          sender->encryption_buffer = (uint8_t *) rt_malloc(RTMSG_CLIENT_READ_BUFFER_SIZE);
          memset(sender->encryption_buffer, 0, RTMSG_CLIENT_READ_BUFFER_SIZE);
          rtLog_Debug("key exchange complete");
        }
      }
    }
  }
}
#endif

static rtError
rtRouted_OnMessage(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff,
  int n, rtSubscription* not_unsed)
{
  (void) not_unsed;

  if (strcmp(hdr->topic, "_RTROUTED.INBOX.SUBSCRIBE") == 0)
  {
    rtRouted_OnMessageSubscribe(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, "_RTROUTED.INBOX.HELLO") == 0)
  {
    rtRouted_OnMessageHello(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, RTM_DISCOVER_REGISTERED_COMPONENTS) == 0)
  {
    rtRouted_OnMessageDiscoverRegisteredComponents(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, RTM_DISCOVER_WILDCARD_DESTINATIONS ) == 0)
  {
    rtRouted_OnMessageDiscoverWildcardDestinations(sender, hdr, buff, n);
  }
  else if(strcmp(hdr->topic, RTM_DISCOVER_OBJECT_ELEMENTS) == 0)
  {
    rtRouted_OnMessageDiscoverObjectElements(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, RTM_DISCOVER_ELEMENT_OBJECTS) == 0)
  {
    rtRouted_OnMessageDiscoverElementObjects(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, RTROUTER_DIAG_DESTINATION) == 0)
  {
    rtRouted_OnMessageDiagnostics(sender, hdr, buff, n);
  }
#ifdef WITH_SPAKE2
  else if (strcmp(hdr->topic, RTROUTED_KEY_EXCHANGE) == 0)
  {
    rtRouted_OnMessageKeyExchange(sender, hdr, buff, n);
  }
#endif 
#ifdef MSG_ROUNDTRIP_TIME 
  else if (strcmp(hdr->topic, RTROUTED_TRANSACTION_TIME_INFO) == 0)
  {
    rtRouted_OnMessageTimeOut(sender, hdr, buff, n);
  }
#endif
  else
  {
    rtLog_Info("no handler for message:%s", hdr->topic);
  }
  return RT_OK;
}

static void
rtConnectedClient_Init(rtConnectedClient* clnt, int fd, struct sockaddr_storage* remote_endpoint)
{
  clnt->fd = fd;
  clnt->state = rtConnectionState_ReadHeaderPreamble;
  clnt->bytes_read = 0;
  clnt->bytes_to_read = RTMESSAGEHEADER_PREAMBLE_LENGTH;
  clnt->read_buffer = (uint8_t *) rt_malloc(RTMSG_CLIENT_READ_BUFFER_SIZE);
  clnt->send_buffer = (uint8_t *) rt_malloc(RTMSG_CLIENT_READ_BUFFER_SIZE);
  memcpy(&clnt->endpoint, remote_endpoint, sizeof(struct sockaddr_storage));
  memset(clnt->read_buffer, 0, RTMSG_CLIENT_READ_BUFFER_SIZE);
  memset(clnt->send_buffer, 0, RTMSG_CLIENT_READ_BUFFER_SIZE);
  clnt->read_buffer_capacity = RTMSG_CLIENT_READ_BUFFER_SIZE;
#ifdef WITH_SPAKE2
  clnt->cipher = NULL;
  clnt->encryption_key = NULL;
  clnt->encryption_buffer = NULL;
#endif
  rtMessageHeader_Init(&clnt->header);
}

static void
rtRouter_DispatchMessageFromClient(rtConnectedClient* clnt)
{
  TRACKING_BOILERPLATE();
  int match_found = 0;
  int loop_safeguard = 0;
  rtRouteEntry * route;
  rtList list;
  rtListItem item;
#ifdef MSG_ROUNDTRIP_TIME
  static rtTime_t ts = {0};
#endif
  START_TRACKING();
dispatch:

  rtRoutingTree_GetTopicRoutes(gRoutingTree, clnt->header.topic, &list);
  if(list)
  {
    rtList_GetFront(list, &item);
    while(item)
    {
      rtTreeRoute* treeRoute;
      rtListItem_GetData(item, (void**)&treeRoute);
      rtListItem_GetNext(item, &item);
      if(treeRoute)
      {
        route = treeRoute->route;
        if(route)
        {
          rtError err = RT_OK;
          match_found = 1;
          STOP_TRACKING_v2();
#ifdef MSG_ROUNDTRIP_TIME
          if((clnt->header.flags & rtMessageFlags_Request))
          {
            rtTime_Now(&ts);
            clnt->header.T3 = ts.tv_sec;
          }
#endif
          rtLog_Debug("DispatchMessage topic=%s expression=%s", clnt->header.topic, route->expression);
          err = route->message_handler(clnt, &clnt->header, clnt->read_buffer +
              clnt->header.header_length, clnt->header.payload_length, route->subscription);
          if (err != RT_OK)
          {
            if (err == rtErrorFromErrno(EBADF))
              rtRouted_ClearClientRoutes(clnt);
          }
        }
      }
    }
  }
  if (!match_found && (0 == loop_safeguard))
  {
    rtLog_Debug("no client found for match:%s", clnt->header.topic);

    if(clnt->header.flags & rtMessageFlags_Request)
    {
      /*Turn this message around without the payload. Set the right error flag.*/
      strncpy(clnt->header.topic, clnt->header.reply_topic, (strlen(clnt->header.reply_topic) + 1));
      clnt->header.flags &= ~rtMessageFlags_Request; 
      clnt->header.flags |= (rtMessageFlags_Response | rtMessageFlags_Undeliverable);
      clnt->header.payload_length = 0;
      loop_safeguard = 1;
      goto dispatch;
      //rtConnection_SendErrorMessageToCaller(clnt->fd, &clnt->header);
    }
#ifdef MSG_ROUNDTRIP_TIME
    if((clnt->header.flags & rtMessageFlags_Request) || (clnt->header.flags & rtMessageFlags_Response))
    {
      rtLog_Info("Consumer does not exist");
      rtRouted_TransactionTimingDetails(clnt->header);
    }
#endif
  }
}

static inline void
rtConnectedClient_Reset(rtConnectedClient *clnt)
{
  clnt->bytes_to_read = RTMESSAGEHEADER_PREAMBLE_LENGTH;
  clnt->bytes_read = 0;
  clnt->state = rtConnectionState_ReadHeaderPreamble;
  rtMessageHeader_Init(&clnt->header);
}

static char*
rtRouted_GetClientName(rtConnectedClient* clnt)
{
  size_t i;
  i = rtVector_Size(gRoutes);
  char *clnt_name = NULL;
  while(i--)
  {
    rtRouteEntry* route = (rtRouteEntry *) rtVector_At(gRoutes, i);
    if(route && (route->subscription) && (route->subscription->client)) {
      if(strcmp( route->subscription->client->ident, clnt->ident ) == 0) {
        clnt_name = route->expression;
        break;
      }
    }
  }
  return clnt_name;
}

static rtError
rtConnectedClient_Read(rtConnectedClient* clnt)
{
  ssize_t bytes_read;
  int bytes_to_read = (clnt->bytes_to_read - clnt->bytes_read);

#ifdef MSG_ROUNDTRIP_TIME
  rtTime_t daemon_request = {0};
  rtTime_t daemon_response = {0};
  if(clnt->header.flags & rtMessageFlags_Request)
  {
     rtTime_Now(&daemon_request);
     clnt->header.T2 = daemon_request.tv_sec;
  }
  if(clnt->header.flags & rtMessageFlags_Response)
  {
     rtTime_Now(&daemon_response);
     clnt->header.T5 = daemon_response.tv_sec;
  }
#endif

  bytes_read = recv(clnt->fd, &clnt->read_buffer[clnt->bytes_read], bytes_to_read, MSG_NOSIGNAL);
  if (bytes_read == -1)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLog_Warn("read:%s", rtStrError(e));
    rtRouted_PrintClientInfo(clnt);
    return e;
  }

  if (bytes_read == 0)
  {
    rtLog_Debug("read zero bytes, stream closed");
    return RT_ERROR_STREAM_CLOSED;
  }

  clnt->bytes_read += bytes_read;

  switch (clnt->state)
  {
    case rtConnectionState_ReadHeaderPreamble:
    {
      // read version/length of header
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        uint8_t const* itr = &clnt->read_buffer[0];
        uint16_t header_start = 0, header_length = 0, header_version = 0;
        rtEncoder_DecodeUInt16(&itr, &header_start);
        rtEncoder_DecodeUInt16(&itr, &header_version);
        if((RTMSG_HEADER_MARKER != header_start) || (RTMSG_HEADER_VERSION != header_version))
        {
          rtLog_Warn("Bad header in message from %s - %s", clnt->ident, rtRouted_GetClientName(clnt));
          rtConnectedClient_Reset(clnt);
          break;
        }
        rtEncoder_DecodeUInt16(&itr, &header_length);
        clnt->bytes_to_read += (header_length - RTMESSAGEHEADER_PREAMBLE_LENGTH);
        clnt->state = rtConnectionState_ReadHeader;
      }
    }
    break;

    case rtConnectionState_ReadHeader:
    {
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        if(RT_OK != rtMessageHeader_Decode(&clnt->header, clnt->read_buffer))
        {
          rtLog_Warn("Bad header in message from %s - %s", clnt->ident, rtRouted_GetClientName(clnt));
          rtConnectedClient_Reset(clnt);
          break;
        }
#ifdef ENABLE_ROUTER_BENCHMARKING
        if(clnt->header.flags & rtMessageFlags_Tainted)
          clock_gettime(CLOCK_MONOTONIC, &g_entry_exit_timestamps[g_timestamp_index][0]);
#endif
        clnt->bytes_to_read += clnt->header.payload_length;
        clnt->state = rtConnectionState_ReadPayload;
        int incoming_data_size = clnt->bytes_to_read + clnt->bytes_read;
        if(clnt->read_buffer_capacity < incoming_data_size)
        {
          uint8_t * ptr = (uint8_t *)rt_try_realloc(clnt->read_buffer, incoming_data_size);
          if(NULL != ptr)
          {
            clnt->read_buffer = ptr;
            clnt->read_buffer_capacity = incoming_data_size;
            rtLog_Info("Reallocated read buffer to %d bytes to accommodate traffic.", incoming_data_size);
          }
          else
          {
            rtLog_Info("Couldn't not reallocate read buffer to accommodate %d bytes. Message will be dropped.", incoming_data_size);
            _rtConnection_ReadAndDropBytes(clnt->fd, clnt->header.payload_length);
            rtConnectedClient_Reset(clnt);
            break;
          }
        }
      }
    }
    break;

    case rtConnectionState_ReadPayload:
    {
      if (clnt->bytes_read == clnt->bytes_to_read)
      {
        rtRouter_DispatchMessageFromClient(clnt);
#ifdef ENABLE_ROUTER_BENCHMARKING
        if(clnt->header.flags & rtMessageFlags_Tainted)
        {
          clock_gettime(CLOCK_MONOTONIC, &g_entry_exit_timestamps[g_timestamp_index][1]);
          if(g_timestamp_index < (MAX_TIMESTAMP_ENTRIES - 1))
            g_timestamp_index++;
        }
#endif
        rtConnectedClient_Reset(clnt);
        
        /* If the read buffer was expanded to deal with an unusually large message, shrink it to normal size to free that memory.*/
        if(RTMSG_CLIENT_READ_BUFFER_SIZE != clnt->read_buffer_capacity)
        {
          free(clnt->read_buffer);
          clnt->read_buffer = (uint8_t *)rt_malloc(RTMSG_CLIENT_READ_BUFFER_SIZE);
          if(NULL == clnt->read_buffer)
            rtLog_Fatal("Out of memory to create read buffer.");
          clnt->read_buffer_capacity = RTMSG_CLIENT_READ_BUFFER_SIZE;
        }
      }
    }
    break;
  }

  return RT_OK;
}

static void
rtRouted_PushFd(fd_set* fds, int fd, int* maxFd)
{
  if (fd != RTMSG_INVALID_FD)
  {
    FD_SET(fd, fds);
    if (maxFd && fd > *maxFd)
      *maxFd = fd;
  }
}

static void
rtRouted_RegisterNewClient(int fd, struct sockaddr_storage* remote_endpoint)
{
  char remote_address[108];
  uint16_t remote_port;
  rtConnectedClient* new_client;

  remote_address[0] = '\0';
  remote_port = 0;
  new_client = (rtConnectedClient *)rt_malloc(sizeof(rtConnectedClient));
  new_client->fd = -1;
  new_client->inbox[0] = '\0';

  rtConnectedClient_Init(new_client, fd, remote_endpoint);
  rtSocketStorage_ToString(&new_client->endpoint, remote_address, sizeof(remote_address), &remote_port);
  snprintf(new_client->ident, RTMSG_ADDR_MAX, "%s:%d/%d", remote_address, remote_port, fd);
  rtVector_PushBack(gClients, new_client);

  rtLog_Debug("new client:%s", new_client->ident);
}

static void
rtRouted_AcceptClientConnection(rtListener* listener)
{
  int                       fd;
  socklen_t                 socket_length;
  struct sockaddr_storage   remote_endpoint;

  socket_length = sizeof(struct sockaddr_storage);
  memset(&remote_endpoint, 0, sizeof(struct sockaddr_storage));

  fd = accept(listener->fd, (struct sockaddr *)&remote_endpoint, &socket_length);
  if (fd == -1)
  {
    rtLog_Warn("accept:%s", rtStrError(errno));
    return;
  }
  
  uint32_t one = 1;
  setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));

  rtRouted_RegisterNewClient(fd, &remote_endpoint);
}


void freeListener(void* p)
{
  free(p);
}
void freeClient(void* p)
{
  rtConnectedClient* client = (rtConnectedClient*)p;
  free(client->read_buffer);
  free(client->send_buffer);
  free(client);
}
void freeRoute(void* p)
{
  rtRouteEntry* route = (rtRouteEntry*)p;
  if(route->subscription)
    free(route->subscription);
  free(route);
}


int main(int argc, char* argv[])
{
  int c;
  int i;
  int run_in_foreground;
  int use_no_delay;
  int ret;
  char const* socket_name[RTMSG_MAX_LISTENING_SOCKETS];
  char const* config_file;
  char const* temp;
  int num_listeners = 0;
  rtRouteEntry* route;

  run_in_foreground = 0;
  use_no_delay = 1;
  config_file = "/etc/rtrouted.conf";

#ifdef INCLUDE_BREAKPAD
#ifdef RDKC_BUILD
  sleep(1);
  newBreakPadWrapExceptionHandler();
#else
  breakpad_ExceptionHandler();
#endif
#endif

  rtLog_SetLevel(RT_LOG_INFO);

  rtVector_Create(&gClients);
  rtVector_Create(&gListeners);
  rtVector_Create(&gRoutes);
  rtRoutingTree_Create(&gRoutingTree);
  rtList_Create(&g_discovery_result);

  FILE* pid_file = fopen("/tmp/rtrouted.pid", "w");
  if (!pid_file)
  {
    printf("failed to open pid file. %s\n", strerror(errno));
    return 0;
  }
  
  int fd = fileno(pid_file);
  int retval = flock(fd, LOCK_EX | LOCK_NB);
  if (retval != 0 && errno == EWOULDBLOCK)
  {
    rtLog_Warn("another instance of rtrouted is already running");
    exit(12);
  }

#ifdef ENABLE_RDKLOGGER
    rdk_logger_init("/etc/debug.ini");
#endif

  rtLog_SetOption(RT_USE_RDKLOGGER);
  rtLogSetLogHandler(NULL);

  // add internal route
  {
    route = (rtRouteEntry *)rt_malloc(sizeof(rtRouteEntry));
    route->subscription = NULL;
    strncpy(route->expression, "_RTROUTED.>", RTMSG_MAX_EXPRESSION_LEN-1);
    route->message_handler = rtRouted_OnMessage;
    rtVector_PushBack(gRoutes, route);
    rtRoutingTree_AddTopicRoute(gRoutingTree, "_RTROUTED.INBOX.SUBSCRIBE", (void *)route, 0);
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTM_DISCOVER_WILDCARD_DESTINATIONS, (void *)route, 0);
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTM_DISCOVER_REGISTERED_COMPONENTS, (void *)route, 0);
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTM_DISCOVER_OBJECT_ELEMENTS, (void *)route, 0);
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTM_DISCOVER_ELEMENT_OBJECTS, (void *)route, 0);
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTROUTER_DIAG_DESTINATION, (void *)route, 0);
#ifdef WITH_SPAKE2
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTROUTED_KEY_EXCHANGE, (void *)route, 0);
#endif
#ifdef MSG_ROUNDTRIP_TIME
    rtRoutingTree_AddTopicRoute(gRoutingTree, RTROUTED_TRANSACTION_TIME_INFO, (void *)route, 0);
#endif
  }

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = 
    {
      {"foreground",  no_argument,        0, 'f'},
      {"no-delay",    no_argument,        0, 'd' },
      {"log-level",   required_argument,  0, 'l' },
      {"debug-route", no_argument,        0, 'r' },
      {"socket",      required_argument,  0, 's' },
      { "config",     required_argument,  0, 'c' },
      { "help",       no_argument,        0, 'h' },
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "c:dfl:rhs:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'c':
        config_file = optarg;
        break;
      case 's':
        socket_name[num_listeners++] = optarg;
        break;
      case 'd':
        use_no_delay = 0;
        break;
      case 'f':
        run_in_foreground = 1;
        break;
      case 'l':
        rtLog_SetLevel(rtLogLevelFromString(optarg));
        break;
      case 'h':
        rtRouted_PrintHelp();
        break;
      case 'r':
      {
        route = (rtRouteEntry *)rt_malloc(sizeof(rtRouteEntry));
        route->subscription = NULL;
        route->message_handler = &rtRouted_TrafficMonitorLog;
        strncpy(route->expression, ">", RTMSG_MAX_EXPRESSION_LEN-1);
        rtVector_PushBack(gRoutes, route);
      }
      case '?':
        break;
      default:
        fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
    }
  }
  /* Array manipulation for making unix socket as the first element of the socket_name array */
  for (i = 0; (num_listeners > 0 && i < num_listeners); i++)
  {
    if(!strncmp(socket_name[i], "unix", 4))
    {
      temp = socket_name[i];
      socket_name[i] = socket_name[0];
      socket_name[0] = temp;
      break;
    }
  }

  if (!run_in_foreground)
  {
    ret = daemon(0 /*chdir to "/"*/, 1 /*redirect stdout/stderr to /dev/null*/ );
    if (ret == -1)
    {
      rtLog_Fatal("failed to fork off daemon. %s", rtStrError(errno));
      exit(1);
    }
  }
  else
  {
    rtLog_Debug("running in foreground");
  }

  if (config_file && rtRouted_FileExists(config_file))
  {
    rtRouted_ParseConfig(config_file);
  }
  else
  {
    if(0 == num_listeners)
    {
	    socket_name[0] = "unix:///tmp/rtrouted";
	    num_listeners = 1;
    }

    int indefinite_retry = 0;
    for(i = 0; i < num_listeners; i++)
    {
        rtListener* listener = NULL;
	    rtRouteBase_BindListener(socket_name[i], use_no_delay, indefinite_retry, &listener);
        rtVector_PushBack(gListeners, listener);
        indefinite_retry  = 1;
    }
  }

  if(access("/nvram/rtrouted_traffic_monitor", F_OK) == 0)
    g_enable_traffic_monitor = 1;

  while (is_running)
  {
    int n;
    int                         max_fd;
    fd_set                      read_fds;
    fd_set                      err_fds;
    struct timeval              timeout;

    max_fd= -1;
    FD_ZERO(&read_fds);
    FD_ZERO(&err_fds);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    for (i = 0, n = rtVector_Size(gListeners); i < n; ++i)
    {
      rtListener* listener = (rtListener *) rtVector_At(gListeners, i);
      if (listener)
      {
        rtRouted_PushFd(&read_fds, listener->fd, &max_fd);
        rtRouted_PushFd(&err_fds, listener->fd, &max_fd);
      }
    }

    for (i = 0, n = rtVector_Size(gClients); i < n; ++i)
    {
      rtConnectedClient* clnt = (rtConnectedClient *) rtVector_At(gClients, i);
      if (clnt)
      {
        rtRouted_PushFd(&read_fds, clnt->fd, &max_fd);
        rtRouted_PushFd(&err_fds, clnt->fd, &max_fd);
      }
    }

    ret = select(max_fd + 1, &read_fds, NULL, &err_fds, &timeout);
    if (ret == 0)
      continue;

    if (ret == -1)
    {
      rtLog_Warn("select:%s", rtStrError(errno));
      continue;
    }

    for (i = 0, n = rtVector_Size(gListeners); i < n; ++i)
    {
      rtListener* listener = (rtListener *) rtVector_At(gListeners, i);
      if (FD_ISSET(listener->fd, &read_fds))
        rtRouted_AcceptClientConnection(listener);
    }

    for (i = 0, n = rtVector_Size(gClients); i < n;)
    {
      rtConnectedClient* clnt = (rtConnectedClient *) rtVector_At(gClients, i);
      if (FD_ISSET(clnt->fd, &read_fds))
      {
        rtError err = rtConnectedClient_Read(clnt);
        if (err != RT_OK)
        {
          rtVector_RemoveItem(gClients, clnt, NULL);
          rtRouted_SendAdvisoryMessage(clnt, rtAdviseClientDisconnect);
          rtConnectedClient_Destroy(clnt);
          n--;
          continue;
        }
      }
      i++;
    }
    while(1)
    {
        if(!(access("/tmp/heartbeat", F_OK)))
            continue;
        else
            break;
    }
  }

  rtVector_Destroy(gListeners, freeListener);
  rtVector_Destroy(gClients, freeClient);
  rtVector_Destroy(gRoutes, freeRoute);
  rtRoutingTree_Destroy(gRoutingTree);
  rtList_Destroy(g_discovery_result, NULL);
  fclose(pid_file);
#if WITH_SPAKE2
  if(g_spake2_L)
    free(g_spake2_L);
  if(g_spake2_w0)
    free(g_spake2_w0);
#endif

  return 0;
}
