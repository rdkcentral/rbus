#define _GNU_SOURCE 1
#include "rtMessage.h"
#include "rtTime.h"
#include "rtDebug.h"
#include "rtLog.h"
#include "rtEncoder.h"
#include "rtError.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"
#include "rtVector.h"
#include "rtConnection.h"
#include "rtAdvisory.h"
#include "rtMemory.h"
#include "rtrouteBase.h"
#include "rtRoutingTree.h"
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

#include <pthread.h>

static pthread_once_t _rtDirectRoute_once = PTHREAD_ONCE_INIT;
static pthread_key_t  _rtDirectRoute_key;


rtError
//rtRouted_BindListener(char const* socket_name, int no_delay, int indefinite_retry, rtListener **pListener)
rtRouteBase_BindListener(char const* socket_name, int no_delay, int indefinite_retry, rtListener **pListener)
{
  int ret;
  rtError err;
  socklen_t socket_length;
  rtListener* listener;
  unsigned int num_retries = 1;

  listener = (rtListener *)rt_malloc(sizeof(rtListener));
  listener->fd = -1;
  memset(&listener->local_endpoint, 0, sizeof(struct sockaddr_storage));

  err = rtSocketStorage_FromString(&listener->local_endpoint, socket_name);
  if (err != RT_OK)
    return err;

  rtLog_Debug("binding listener:%s", socket_name);

  listener->fd = socket(listener->local_endpoint.ss_family, SOCK_STREAM, 0);
  if (listener->fd == -1)
  {
    rtLog_Fatal("socket:%s", rtStrError(errno));
    exit(1);
  }

  rtSocketStorage_GetLength(&listener->local_endpoint, &socket_length);

  if (listener->local_endpoint.ss_family != AF_UNIX)
  {
    uint32_t one = 1;
    if (no_delay)
      setsockopt(listener->fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));

    setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    if(indefinite_retry == 1)
    {
      /* assigning maximum value of unsigned integer(0xFFFFFFFF - 4294967295) to num_retries */
      num_retries = 0;
      num_retries = ~num_retries;
    }
    else
      num_retries = 18; //Special handling for TCP sockets: keep retrying for 3 minutes, 10s after each failure. This helps if networking is slow to come up.
  }

    while(0 != num_retries)
    {
      ret = bind(listener->fd, (struct sockaddr *)&listener->local_endpoint, socket_length);
      if (ret == -1)
      {
        rtError err = rtErrorFromErrno(errno);
        rtLog_Warn("failed to bind socket. %s.  num_retries=%u", rtStrError(err), num_retries);
        if(0 == --num_retries)
        {
          rtLog_Warn("exiting app on bind socket failure");
          exit(1);
        }
        else
          sleep(10);
      }
      else
        break;
    }

  ret = listen(listener->fd, 4);
  if (ret == -1)
  {
    rtLog_Warn("failed to set socket to listen mode. %s", rtStrError(errno));
    exit(1);
  }

  *pListener = listener;
  return RT_OK;
}


rtError rtRouteBase_CloseListener(rtListener *pListener)
{
    (void) pListener;

    if (pListener)
    {
        shutdown(pListener->fd, SHUT_RDWR);
        close(pListener->fd);
        rtLog_Warn("Shutdown the connection");
    }
    return RT_OK;
}

#define RTROUTER_DIRECT_DIAG_DESTINATION "_DIRECTROUTE.INBOX.DIAG"

static void rtRouted_OnMessageHello(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
  rtMessage m;

  if(RT_OK != rtMessage_FromBytes(&m, buff, n))
  {
    rtLog_Warn("Bad Hello message");
    rtLog_Warn("Sender %s", sender->ident);
    return;
  }

  {
    char* buff = NULL;
    uint32_t buff_length = 0;

    rtMessage_ToString(m, &buff, &buff_length);
    rtLog_Info("Message Received =>:%.*s", buff_length, buff);
    free(buff);
  }

  rtMessage_Release(m);
  
  (void)hdr;
}

static void rtRouted_OnMessageSubscribe(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n)
{
    rtRouted_OnMessageHello(sender, hdr, buff, n);
}

static rtError reply_to_client(rtConnectedClient* client, rtMessageHeader * request_hdr, uint8_t* rspData, uint32_t rspDataLength)
{
    rtError ret = RT_OK;
    ssize_t bytes_sent;
    request_hdr->payload_length = rspDataLength;

    if(1)
    {
        rtLog_Debug("SendMessage topic=%s expression...", request_hdr->topic);

        rtMessageHeader_Encode(request_hdr, client->send_buffer);
        struct iovec send_vec[] = {{client->send_buffer, request_hdr->header_length}, {(void *)rspData, rspDataLength}};
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
              ret = RT_FAIL;
            }
            break;
          }

        } while(0);
    }
    return ret;
}

static void prep_reply_header_from_request(rtMessageHeader *reply, const rtMessageHeader *request)
{

  rtMessageHeader_Init(reply);
  reply->version = request->version;
  reply->header_length = request->header_length;
  reply->sequence_number = request->sequence_number;
  reply->flags = rtMessageFlags_Response;
  reply->control_data = 0;//subscription->id;

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

static rtError rtRouted_OnMessage(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n, rtSubscription* mySubs)
{
  (void) mySubs;
  if (strcmp(hdr->topic, "_RTROUTED.INBOX.SUBSCRIBE") == 0)
  {
    rtRouted_OnMessageSubscribe(sender, hdr, buff, n);
  }
  else if (strcmp(hdr->topic, RTROUTER_DIRECT_DIAG_DESTINATION) == 0)
  {
    rtRouted_OnMessageHello(sender, hdr, buff, n);
  }
  else
  {
    rtLog_Info("default handler for message:%s", hdr->topic);

    rtDriectClientHandler dr_ctx = (rtDriectClientHandler) pthread_getspecific(_rtDirectRoute_key);
    if (dr_ctx)
    {
        uint8_t* rspData = NULL;
        uint32_t rspDataLength = 0;
        dr_ctx(hdr, buff, n, &rspData, &rspDataLength);

        rtMessageHeader new_header;
        prep_reply_header_from_request(&new_header, hdr);
        if(RT_OK != reply_to_client(sender, &new_header, rspData, rspDataLength))
          rtLog_Info("%s() Response couldn't be sent.", __func__);
    }
    else
        rtLog_Error("no callback handler found :%s", hdr->topic);
  }
  return RT_OK;
}

static void rtRouted_PushFd(fd_set* fds, int fd, int* maxFd)
{
  if (fd != RTMSG_INVALID_FD)
  {
    FD_SET(fd, fds);
    if (maxFd && fd > *maxFd)
      *maxFd = fd;
  }
}


static void rtConnectedClient_Init(rtConnectedClient* clnt, int fd, struct sockaddr_storage* remote_endpoint)
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

static void rtRouterBase_DispatchMessageFromClient(rtConnectedClient* clnt, rtRouteEntry* pDirectRoute)
{
  rtRouteEntry * route;
#ifdef MSG_ROUNDTRIP_TIME
  static rtTime_t ts = {0};
#endif
  route = pDirectRoute;
  if(route)
  {
    rtError err = RT_OK;
#ifdef MSG_ROUNDTRIP_TIME
    if((clnt->header.flags & rtMessageFlags_Request))
    {
      rtTime_Now(&ts);
      clnt->header.T3 = ts.tv_sec;
    }
#endif
    rtLog_Debug("DispatchMessage topic=%s expression=%s", clnt->header.topic, route->expression);
    err = route->message_handler(clnt, &clnt->header, clnt->read_buffer + clnt->header.header_length, clnt->header.payload_length, route->subscription);
    if (err != RT_OK)
    {
      if (err == rtErrorFromErrno(EBADF))
      {
        //FIXME: KARUNA:: when a client closed the connection, we get error out here..
        //rtRouted_ClearClientRoutes(clnt);
      }
    }
  }
}

static inline void rtConnectedClient_Reset(rtConnectedClient *clnt)
{
  clnt->bytes_to_read = RTMESSAGEHEADER_PREAMBLE_LENGTH;
  clnt->bytes_read = 0;
  clnt->state = rtConnectionState_ReadHeaderPreamble;
  rtMessageHeader_Init(&clnt->header);
}

static rtError rtConnectedClient_Read(rtConnectedClient* clnt, rtRouteEntry* myDirectRoute)
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
          rtLog_Warn("Bad header in message from %s", clnt->ident);
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
          rtLog_Warn("Bad header in message from %s", clnt->ident);
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
        rtRouterBase_DispatchMessageFromClient(clnt, myDirectRoute);
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


static rtConnectedClient* rtRouted_RegisterNewClient(int fd, struct sockaddr_storage* remote_endpoint)
{
  char remote_address[108];
  uint16_t remote_port;
  rtConnectedClient* new_client = NULL;

  remote_address[0] = '\0';
  remote_port = 0;
  new_client = (rtConnectedClient *)rt_malloc(sizeof(rtConnectedClient));
  new_client->fd = -1;
  new_client->inbox[0] = '\0';

  rtConnectedClient_Init(new_client, fd, remote_endpoint);
  rtSocketStorage_ToString(&new_client->endpoint, remote_address, sizeof(remote_address), &remote_port);
  snprintf(new_client->ident, RTMSG_ADDR_MAX, "%s:%d/%d", remote_address, remote_port, fd);

  rtLog_Debug("new client:%s", new_client->ident);
  return new_client;
}

static rtConnectedClient* rtRouted_AcceptClientConnection(rtListener* listener)
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
    return NULL;
  }
  
  uint32_t one = 1;
  setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));

  return rtRouted_RegisterNewClient(fd, &remote_endpoint);
}

static void _rtdirect_route_thread_specific_key()
{
  pthread_key_create(&_rtDirectRoute_key, NULL);
}


rtError rtRouted_StartDirectRouting(const char* socket_name, rtDriectClientHandler messageHandler)
{
  int ret;
  rtRouteEntry* route = NULL;
  rtConnectedClient* myDirectClient = NULL;
  rtListener* myDirectListener = NULL;

  if (NULL == socket_name)
  {
    rtLog_Warn("NULL Socket Name to listen to");
    return RT_ERROR;
  }

  pthread_once(&_rtDirectRoute_once, &_rtdirect_route_thread_specific_key);
  pthread_setspecific(_rtDirectRoute_key, messageHandler);


  rtRouteBase_BindListener(socket_name, 1, 1, &myDirectListener);

  route = (rtRouteEntry *)rt_malloc(sizeof(rtRouteEntry));
  route->subscription = NULL;
  strncpy(route->expression, "_RTDIRECT>", RTMSG_MAX_EXPRESSION_LEN-1);
  route->message_handler = rtRouted_OnMessage;

  int ltIsrunning = 1;
  while (ltIsrunning)
  {
    int                         max_fd;
    fd_set                      read_fds;
    fd_set                      err_fds;
    struct timeval              timeout;

    max_fd= -1;
    FD_ZERO(&read_fds);
    FD_ZERO(&err_fds);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (myDirectListener)
    {
      rtRouted_PushFd(&read_fds, myDirectListener->fd, &max_fd);
      rtRouted_PushFd(&err_fds, myDirectListener->fd, &max_fd);
    }

    if (myDirectClient)
    {
      rtRouted_PushFd(&read_fds, myDirectClient->fd, &max_fd);
      rtRouted_PushFd(&err_fds, myDirectClient->fd, &max_fd);
    }

    ret = select(max_fd + 1, &read_fds, NULL, &err_fds, &timeout);
    if (ret == 0)
      continue;

    if (ret == -1)
    {
      rtLog_Warn("select:%s", rtStrError(errno));
      continue;
    }

    if (FD_ISSET(myDirectListener->fd, &read_fds))
    {
      rtLog_Error("This should be called only once as there should be only one client");
      myDirectClient = rtRouted_AcceptClientConnection(myDirectListener);
    }

    if (FD_ISSET(myDirectClient->fd, &read_fds))
    {
      rtError err = rtConnectedClient_Read(myDirectClient, route);
      if (err != RT_OK)
      {
        rtLog_Warn("Client Exited..\n");
        break;
      }
    }
  }

  rtRouteBase_CloseListener(myDirectListener);
  return RT_OK;
}
