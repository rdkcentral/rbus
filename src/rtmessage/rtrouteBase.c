#define _GNU_SOURCE 1
#include "rtMessage.h"
#include "rtString.h"
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

rtError
rtRouteBase_CloseListener(rtListener *pListener)
{
    if (pListener)
    {
        shutdown(pListener->fd, SHUT_RDWR);
        close(pListener->fd);
        rtLog_Debug("Shutdown the connection");
    }
    return RT_OK;
}


rtError
rtRouteBase_BindListener(char const* socket_name, int no_delay, int indefinite_retry, rtListener **pListener)
{
  int ret;
  rtError err;
  socklen_t socket_length;
  rtListener* listener;
  unsigned int num_retries = 1;

  listener = (rtListener *)rt_malloc(sizeof(rtListener));
  if (!listener)
       return rtErrorFromErrno(ENOMEM);

  listener->fd = -1;
  memset(&listener->local_endpoint, 0, sizeof(struct sockaddr_storage));

  err = rtSocketStorage_FromString(&listener->local_endpoint, socket_name);
  if (err != RT_OK)
  {
      free(listener);
      return err;
  }

  rtLog_Debug("binding listener:%s", socket_name);

  listener->fd = socket(listener->local_endpoint.ss_family, SOCK_STREAM, 0);
  if (listener->fd == -1)
  {
    rtLog_Fatal("socket:%s", rtStrError(errno));
    free(listener);
    return RT_FAIL;
  }

  rtSocketStorage_GetLength(&listener->local_endpoint, &socket_length);

  if (listener->local_endpoint.ss_family != AF_UNIX)
  {
    uint32_t one = 1;
    int socRet = 0;
    if (no_delay)
    {
      socRet = setsockopt(listener->fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
      if (socRet < 0)
        rtLog_Warn("Failed to set TCP_NODELAY: %s", rtStrError(errno));
    }
      
    socRet = setsockopt(listener->fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    if (socRet < 0)
      rtLog_Warn("Failed to set SO_REUSEADDR: %s", rtStrError(errno));
      
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
    if (listener->local_endpoint.ss_family == AF_UNIX)
    {
      /* umask is process-wide; do not overlap this bind with other file creation. */
      mode_t saved_umask = umask(S_IXUSR | S_IXGRP | S_IXOTH);
      ret = bind(listener->fd, (struct sockaddr *)&listener->local_endpoint, socket_length);
      int bind_errno = errno;
      umask(saved_umask);
      errno = bind_errno;
    }
    else
      ret = bind(listener->fd, (struct sockaddr *)&listener->local_endpoint, socket_length);
    if (ret == -1)
    {
      rtError err = rtErrorFromErrno(errno);
      rtLog_Warn("failed to bind socket. %s.  num_retries=%u", rtStrError(err), num_retries);
      if(0 == --num_retries)
      {
        rtLog_Warn("exiting app on bind socket failure");
        rtRouteBase_CloseListener(listener);
        free(listener);
        return RT_FAIL;
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
    rtRouteBase_CloseListener(listener);
    free(listener);
    return RT_FAIL;
  }

  *pListener = listener;
  return RT_OK;
}

rtError
rtRouteDirect_SendMessage(const rtPrivateClientInfo* pClient, uint8_t const* pInBuff, int inLength, uint32_t subscriptionId, bool rawData)
{
    rtError ret = RT_OK;
    rtMessageHeader new_header;
    ssize_t bytes_sent;

    if (pClient && pInBuff && (inLength > 0))
    {
        rtMessageHeader_Init(&new_header);
        new_header.sequence_number = 1;
        new_header.flags = rtMessageFlags_RawBinary;

        if(rawData)
            new_header.control_data = subscriptionId; /* Rawdata unique subscription ID */
        else
            new_header.control_data = pClient->clientID;

        rtString_Copy(new_header.topic, pClient->clientTopic, RTMSG_HEADER_MAX_TOPIC_LENGTH);
        new_header.topic_length = strlen(pClient->clientTopic);
        new_header.reply_topic[0] = '\0';
        new_header.reply_topic_length = 0;

        new_header.payload_length = inLength;


        rtLog_Debug("SendMessage topic=%s expression...", new_header.topic);
        static uint8_t buffer[RTMSG_CLIENT_READ_BUFFER_SIZE];

        memset(buffer, 0, RTMSG_CLIENT_READ_BUFFER_SIZE);
        rtMessageHeader_Encode(&new_header, buffer);
        struct iovec send_vec[] = {{buffer, new_header.header_length}, {(void *)pInBuff, inLength}};
        struct msghdr send_hdr = {NULL, 0, send_vec, 2, NULL, 0, 0};
        do
        {
          bytes_sent = sendmsg(pClient->clientFD, &send_hdr, MSG_NOSIGNAL);
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
    else
        ret = RT_ERROR;

    return ret;
}

rtError
rtRouteDirect_StartInstance(const char* socket_name, rtDriectClientHandler messageHandler)
{
  (void)socket_name;
  (void)messageHandler;
  /* Direct connections are removed; keep the exported API but never bind. */
  rtLog_Warn("Direct listener startup rejected: direct connections are disabled");
  return RT_ERROR_INVALID_OPERATION;
}
