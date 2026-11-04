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
#include <sys/un.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

#include <pthread.h>

static pthread_once_t _rtDirectRoute_once = PTHREAD_ONCE_INIT;
static pthread_key_t  _rtDirectRoute_key;

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


static int
rtRouteBase_OpenDirectDirectory(const char* directory)
{
  struct stat st;
  int tmp_fd = open("/tmp", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (tmp_fd < 0)
  {
    rtLog_Error("Failed to open direct socket parent directory: %s", strerror(errno));
    return -1;
  }
  if (fstat(tmp_fd, &st) != 0)
  {
    rtLog_Error("Failed to inspect direct socket parent directory: %s", strerror(errno));
    close(tmp_fd);
    return -1;
  }
  if (st.st_uid != 0 || ((st.st_mode & 0022) != 0 && (st.st_mode & S_ISVTX) == 0))
  {
    rtLog_Error("Rejecting untrusted direct socket parent directory");
    close(tmp_fd);
    return -1;
  }
  bool created = mkdirat(tmp_fd, directory + strlen("/tmp/"), 0711) == 0;
  if (!created && errno != EEXIST)
  {
    rtLog_Error("Failed to create direct socket directory %s: %s", directory, strerror(errno));
    close(tmp_fd);
    return -1;
  }
  int fd = openat(tmp_fd, directory + strlen("/tmp/"),
      O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  close(tmp_fd);
  if (fd < 0)
  {
    rtLog_Error("Failed to open direct socket directory %s: %s", directory, strerror(errno));
    return -1;
  }
  if (fstat(fd, &st) != 0)
  {
    rtLog_Error("Failed to inspect direct socket directory: %s", strerror(errno));
    close(fd);
    return -1;
  }
  if (st.st_uid != geteuid() || (st.st_mode & 0022) != 0)
  {
    rtLog_Error("Rejecting untrusted direct socket directory %s", directory);
    close(fd);
    return -1;
  }
  /* Permit consumer traversal, but only the provider UID can modify entries. */
  if ((st.st_mode & 0777) != 0711 && fchmod(fd, 0711) != 0)
  {
    rtLog_Error("Failed to set direct socket directory permissions: %s", strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int
rtRouteBase_ReclaimDirectLocks(int directory_fd)
{
  int scan_fd = openat(directory_fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (scan_fd < 0)
  {
    rtLog_Error("Failed to open direct lock directory scan: %s", strerror(errno));
    return -1;
  }
  DIR* directory = fdopendir(scan_fd);
  if (!directory)
  {
    rtLog_Error("Failed to start direct lock directory scan: %s", strerror(errno));
    close(scan_fd);
    return -1;
  }
  int result = 0;
  for (;;)
  {
    errno = 0;
    struct dirent* entry = readdir(directory);
    if (!entry)
    {
      if (errno != 0)
      {
        rtLog_Error("Failed to read direct lock directory: %s", strerror(errno));
        result = -1;
      }
      break;
    }
    if (strncmp(entry->d_name, "lock.direct_", strlen("lock.direct_")) != 0)
      continue;
    int fd = openat(directory_fd, entry->d_name,
        O_RDWR | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0)
    {
      rtLog_Error("Failed to open stale direct lock %s: %s", entry->d_name, strerror(errno));
      result = -1;
      break;
    }
    struct stat st;
    if (fstat(fd, &st) != 0)
    {
      rtLog_Error("Failed to inspect stale direct lock: %s", strerror(errno));
      close(fd);
      result = -1;
      break;
    }
    if (!S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        st.st_nlink != 1 || (st.st_mode & 0077) != 0)
    {
      rtLog_Error("Rejecting untrusted stale direct lock %s", entry->d_name);
      close(fd);
      result = -1;
      break;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
      int lock_errno = errno;
      close(fd);
      if (lock_errno == EWOULDBLOCK || lock_errno == EAGAIN)
        continue;
      rtLog_Error("Failed to inspect stale direct lock ownership: %s", strerror(lock_errno));
      result = -1;
      break;
    }
    /* Directory locking excludes all other lock-file openers and removers. */
    if (unlinkat(directory_fd, entry->d_name, 0) != 0)
    {
      rtLog_Error("Failed to reclaim direct lock %s: %s", entry->d_name, strerror(errno));
      result = -1;
    }
    close(fd);
    if (result != 0)
      break;
  }
  if (closedir(directory) != 0)
  {
    rtLog_Error("Failed to close direct lock directory scan: %s", strerror(errno));
    result = -1;
  }
  return result;
}

static int
rtRouteBase_OpenDirectLock(int directory_fd, const char* socket_basename)
{
  char lock_path[sizeof(((struct sockaddr_un*)0)->sun_path) + sizeof(".lock")];
  struct stat opened;
  int length = snprintf(lock_path, sizeof(lock_path), "lock%s", socket_basename);
  if (length < 0 || (size_t)length >= sizeof(lock_path))
  {
    rtLog_Error("Direct socket lock pathname is too long");
    return -1;
  }
  int fd = openat(directory_fd, lock_path,
      O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0600);
  if (fd < 0)
  {
    rtLog_Error("Failed to open direct socket lock %s: %s", lock_path, strerror(errno));
    return -1;
  }
  if (fstat(fd, &opened) != 0)
  {
    rtLog_Error("Failed to inspect direct socket lock: %s", strerror(errno));
    close(fd);
    return -1;
  }
  if (!S_ISREG(opened.st_mode) || opened.st_uid != geteuid() ||
      opened.st_nlink != 1 || (opened.st_mode & 0077) != 0)
  {
    rtLog_Error("Rejecting untrusted direct socket lock %s", lock_path);
    close(fd);
    return -1;
  }
  if (flock(fd, LOCK_EX | LOCK_NB) != 0)
  {
    rtLog_Warn("Cannot acquire direct socket lock %s: %s", lock_path, strerror(errno));
    close(fd);
    return -1;
  }
  return fd;
}

static int
rtRouteBase_ReclaimDirectLocksIfDue(int directory_fd)
{
  /* Shared across processes; the directory gate protects the sweep timestamp. */
  int fd = rtRouteBase_OpenDirectLock(directory_fd, ".maintenance");
  if (fd < 0)
    return -1;

  int result = -1;
  struct stat st;
  time_t now = time(NULL);
  if (now == (time_t)-1)
    rtLog_Error("Failed to read direct lock maintenance time: %s", strerror(errno));
  else if (fstat(fd, &st) != 0)
    rtLog_Error("Failed to inspect direct lock maintenance state: %s", strerror(errno));
  else if (st.st_size != 0 && st.st_size != 1)
    rtLog_Error("Invalid direct lock maintenance state");
  else
  {
    double elapsed = difftime(now, st.st_mtime);
    if (st.st_size == 1 && elapsed >= 0 && elapsed < 60)
      result = 0;
    else if (rtRouteBase_ReclaimDirectLocks(directory_fd) == 0)
    {
      /* Writing also updates mtime, including after a backward clock adjustment. */
      if (pwrite(fd, "1", 1, 0) == 1)
        result = 0;
      else
        rtLog_Error("Failed to record direct lock sweep completion");
    }
  }
  if (close(fd) != 0)
  {
    rtLog_Error("Failed to close direct lock maintenance state: %s", strerror(errno));
    result = -1;
  }
  return result;
}

static int
rtRouteBase_LockDirectSocket(int directory_fd, const char* socket_basename)
{
  if (flock(directory_fd, LOCK_EX) != 0)
  {
    rtLog_Error("Failed to acquire direct lock directory gate: %s", strerror(errno));
    return -1;
  }
  int fd = -1;
  if (rtRouteBase_ReclaimDirectLocksIfDue(directory_fd) == 0)
    fd = rtRouteBase_OpenDirectLock(directory_fd, socket_basename);
  if (flock(directory_fd, LOCK_UN) != 0)
  {
    rtLog_Error("Failed to release direct lock directory gate: %s", strerror(errno));
    if (fd >= 0)
      close(fd);
    return -1;
  }
  return fd;
}

/*
 * Entries are reserved for sockets in the validated owner-writable directory.
 * Hold the lifecycle lock through removal, bind and teardown. Same-UID and
 * privileged processes are trusted and must not replace directory entries.
 */
static rtError
rtRouteBase_RemoveDirectSocket(int directory_fd, const char* socket_basename)
{
  if (unlinkat(directory_fd, socket_basename, 0) == 0 || errno == ENOENT)
    return RT_OK;
  rtLog_Error("Failed to remove direct socket %s: %s", socket_basename, strerror(errno));
  return RT_FAIL;
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
    ret = bind(listener->fd, (struct sockaddr *)&listener->local_endpoint, socket_length);
    if (ret == -1)
    {
      int bind_errno = errno;

      rtError err = rtErrorFromErrno(bind_errno);
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

  if (listener->local_endpoint.ss_family == AF_UNIX)
  {
    //skip the 7 letters from socket name as it points to `unix://`
    if (chmod(&socket_name[7], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0)
      rtLog_Warn("Failed to change socket's write permission for non-root listener");
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

static rtError
_rtdirect_send_reply_to_client(rtConnectedClient* client, rtMessageHeader * request_hdr, const uint8_t* rspData, uint32_t rspDataLength)
{
    rtError ret = RT_OK;
    ssize_t bytes_sent;
    request_hdr->payload_length = rspDataLength;

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
    return ret;
}

static void
_rtdirect_prepare_reply_from_request(rtMessageHeader *reply, const rtMessageHeader *request)
{

  rtMessageHeader_Init(reply);
  reply->version = request->version;
  reply->header_length = request->header_length;
  reply->sequence_number = request->sequence_number;
  reply->flags = rtMessageFlags_Response;
  reply->control_data = 0;//subscription->id;

  rtString_Copy(reply->topic, request->reply_topic, RTMSG_HEADER_MAX_TOPIC_LENGTH);
  rtString_Copy(reply->reply_topic, request->topic, RTMSG_HEADER_MAX_TOPIC_LENGTH);
  reply->topic_length = request->reply_topic_length;
  reply->reply_topic_length = request->topic_length;
}

static void
_rtdirect_route_thread_specific_key()
{
  pthread_key_create(&_rtDirectRoute_key, NULL);
}

static rtError
_rtdirect_OnMessage(rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n, rtSubscription* mySubs)
{
  (void) mySubs;
  if (strcmp(hdr->topic, "_RTROUTED.INBOX.SUBSCRIBE") == 0)
  {
    rtMessage m;
    char const* expression = NULL;
    uint32_t route_id = 0;
    int32_t add_subscrption = 0;

    rtMessage_FromBytes(&m, buff, n);
    if((RT_OK == rtMessage_GetInt32(m, "add", &add_subscrption)) &&
       (RT_OK == rtMessage_GetString(m, "topic", &expression)) &&
       (RT_OK == rtMessage_GetInt32(m, "route_id", (int32_t *)&route_id)))
    {
      if(1 == add_subscrption)
      {
          rtPrivateClientInfo temp;
          temp.clientID = route_id;
          temp.clientFD = sender->fd;
          snprintf(temp.clientTopic, RTMSG_HEADER_MAX_TOPIC_LENGTH, "%s", expression);

          rtDriectClientHandler dr_ctx = (rtDriectClientHandler) pthread_getspecific(_rtDirectRoute_key);
          if (dr_ctx)
          {
              dr_ctx(0, hdr, (const uint8_t *) &temp, sizeof(temp), NULL, NULL);
          }
      }
    }
    rtMessage_Release(m);
  }
  else
  {
    rtLog_Debug("default handler for message:%s", hdr->topic);

    rtDriectClientHandler dr_ctx = (rtDriectClientHandler) pthread_getspecific(_rtDirectRoute_key);
    if (dr_ctx)
    {
        uint8_t* rspData = NULL;
        uint32_t rspDataLength = 0;
        dr_ctx(1, hdr, buff, n, &rspData, &rspDataLength);

        rtMessageHeader new_header;
        _rtdirect_prepare_reply_from_request(&new_header, hdr);
        if(RT_OK != _rtdirect_send_reply_to_client(sender, &new_header, rspData, rspDataLength))
          rtLog_Info("%s() Response couldn't be sent.", __func__);

        if (rspData)
            free(rspData);
    }
    else
        rtLog_Error("no callback handler found :%s", hdr->topic);
  }
  return RT_OK;
}

static void
_rtdirect_pushFd(fd_set* fds, int fd, int* maxFd)
{
  if (fd != RTMSG_INVALID_FD)
  {
    FD_SET(fd, fds);
    if (maxFd && fd > *maxFd)
      *maxFd = fd;
  }
}

static void
_rtdirect_dispatch_message_from_client(rtConnectedClient* clnt, rtRouteEntry* pDirectRoute)
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
      clnt->header.T3 = (uint64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    }
#endif
    rtLog_Debug("DispatchMessage topic=%s expression=%s", clnt->header.topic, route->expression);
    err = route->message_handler(clnt, &clnt->header, clnt->read_buffer + clnt->header.header_length, clnt->header.payload_length, route->subscription);
    if (err != RT_OK)
    {
      if (err == rtErrorFromErrno(EBADF))
      {
        //FIXME: KARUNA:: when a client closed the connection, we get error out here..
        rtLog_Warn("client (%s) exited and the connection does not exist anymore", clnt->header.topic);
      }
    }
  }
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

static inline void
rtConnectedClient_Reset(rtConnectedClient *clnt)
{
  clnt->bytes_to_read = RTMESSAGEHEADER_PREAMBLE_LENGTH;
  clnt->bytes_read = 0;
  clnt->state = rtConnectionState_ReadHeaderPreamble;
  rtMessageHeader_Init(&clnt->header);
}

static rtError
rtConnectedClient_Read(rtConnectedClient* clnt, rtRouteEntry* myDirectRoute)
{
  ssize_t bytes_read;
  if (clnt->bytes_read > clnt->read_buffer_capacity ||
      clnt->bytes_to_read > clnt->read_buffer_capacity)
  {
    rtLog_Warn("Receive size exceeds buffer capacity %zu from %s",
               clnt->read_buffer_capacity, clnt->ident);
    return RT_ERROR_INVALID_ARG;
  }
  if (clnt->bytes_read > clnt->bytes_to_read)
  {
    rtLog_Warn("rtConnectedClient_Read: bytes_read=%zu > bytes_to_read=%zu, clamping to expected size",
               clnt->bytes_read, clnt->bytes_to_read);
    clnt->bytes_read = clnt->bytes_to_read;
  }
  size_t bytes_to_read = clnt->bytes_to_read - clnt->bytes_read;

#ifdef MSG_ROUNDTRIP_TIME
  rtTime_t daemon_request = {0};
  rtTime_t daemon_response = {0};
    if(clnt->header.flags & rtMessageFlags_Request)
    {
      rtTime_Now(&daemon_request);
      clnt->header.T2 = (uint64_t)daemon_request.tv_sec * 1000000000LL + daemon_request.tv_nsec;
    }
    if(clnt->header.flags & rtMessageFlags_Response)
    {
      rtTime_Now(&daemon_response);
      clnt->header.T5 = (uint64_t)daemon_response.tv_sec * 1000000000LL + daemon_response.tv_nsec;
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
        if (header_length < RTMESSAGEHEADER_MIN_LENGTH ||
            (size_t)header_length > clnt->read_buffer_capacity)
        {
          rtLog_Warn("Invalid header length %u from %s (buffer capacity %zu)",
                     header_length, clnt->ident, clnt->read_buffer_capacity);
          return RT_ERROR_INVALID_ARG;
        }
        clnt->bytes_to_read = (size_t)header_length;
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
        size_t payload = (size_t)clnt->header.payload_length;
        if (clnt->bytes_to_read > (SIZE_MAX - payload))
        {
          rtLog_Info("Requested payload_length (%u) would overflow size_t. Message will be dropped.", clnt->header.payload_length);
          _rtConnection_ReadAndDropBytes(clnt->fd, clnt->header.payload_length);
          rtConnectedClient_Reset(clnt);
          break;
        }
        clnt->bytes_to_read += payload;
        clnt->state = rtConnectionState_ReadPayload;
        if (clnt->bytes_to_read > (SIZE_MAX - clnt->bytes_read))
        {
          rtLog_Info("Requested buffer size (%zu + %zu) would overflow size_t. Message will be dropped.",
                     clnt->bytes_to_read, clnt->bytes_read);
          _rtConnection_ReadAndDropBytes(clnt->fd, clnt->header.payload_length);
          rtConnectedClient_Reset(clnt);
          break;
        }
        size_t incoming_data_size = clnt->bytes_to_read + clnt->bytes_read;
        if(clnt->read_buffer_capacity < incoming_data_size)
        {
          uint8_t * ptr = (uint8_t *)rt_try_realloc(clnt->read_buffer, incoming_data_size);
          if(NULL != ptr)
          {
            clnt->read_buffer = ptr;
            clnt->read_buffer_capacity = incoming_data_size;
            rtLog_Info("Reallocated read buffer to %zu bytes to accommodate traffic.", incoming_data_size);
          }
          else
          {
            rtLog_Info("Couldn't reallocate read buffer to accommodate %zu bytes. Message will be dropped.", incoming_data_size);
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
        _rtdirect_dispatch_message_from_client(clnt, myDirectRoute);
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


static rtConnectedClient*
rtRouteDirect_RegisterNewClient(int fd, struct sockaddr_storage* remote_endpoint)
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

static rtConnectedClient*
rtRouteDirect_AcceptClientConnection(rtListener* listener)
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
  if (setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
    rtLog_Warn("Failed to set TCP_NODELAY on accepted socket: %s", rtStrError(errno));

  return rtRouteDirect_RegisterNewClient(fd, &remote_endpoint);
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

typedef struct
{
  rtRouteEntry** route;
  rtConnectedClient** client;
  rtListener** listener;
  int* directoryFD;
  int* lockFD;
  bool* ownsPath;
  const char** basename;
  rtDriectClientHandler messageHandler;
  rtError* result;
} rtDirectInstanceCleanup;

static void
rtRouteDirect_CleanupInstance(void* context)
{
  rtDirectInstanceCleanup* cleanup = context;
  rtConnectedClient* client = *cleanup->client;
  if (client)
  {
    if (client->fd >= 0)
    {
      rtPrivateClientInfo disconnected = {0};
      disconnected.clientFD = client->fd;
      if (cleanup->messageHandler)
      {
        rtError err = cleanup->messageHandler(RT_DIRECT_CLIENT_DISCONNECTED, NULL,
            (uint8_t const*)&disconnected, sizeof(disconnected), NULL, NULL);
        if (err != RT_OK)
          rtLog_Error("Private client disconnect notification failed: %s", rtStrError(err));
      }
      if (close(client->fd) != 0)
        rtLog_Warn("Failed to close private client socket: %s", strerror(errno));
      client->fd = -1;
    }
    free(client->read_buffer);
    free(client->send_buffer);
    free(client);
  }
  free(*cleanup->route);
  if (*cleanup->listener)
  {
    rtRouteBase_CloseListener(*cleanup->listener);
    free(*cleanup->listener);
  }
  if (*cleanup->ownsPath &&
      rtRouteBase_RemoveDirectSocket(*cleanup->directoryFD, *cleanup->basename) != RT_OK)
    *cleanup->result = RT_FAIL;

  bool haveDirectoryGate = false;
  if (*cleanup->lockFD >= 0)
  {
    haveDirectoryGate = flock(*cleanup->directoryFD, LOCK_EX) == 0;
    if (!haveDirectoryGate)
    {
      rtLog_Error("Failed to acquire direct lock cleanup gate: %s", strerror(errno));
      *cleanup->result = RT_FAIL;
    }
    if (haveDirectoryGate)
    {
      char lockPath[sizeof(((struct sockaddr_un*)0)->sun_path) + sizeof(".lock")];
      int length = snprintf(lockPath, sizeof(lockPath), "lock%s", *cleanup->basename);
      if (length < 0 || (size_t)length >= sizeof(lockPath))
      {
        rtLog_Error("Direct socket cleanup lock pathname is too long");
        *cleanup->result = RT_FAIL;
      }
      else if (unlinkat(*cleanup->directoryFD, lockPath, 0) != 0)
      {
        rtLog_Error("Failed to remove direct socket lock: %s", strerror(errno));
        *cleanup->result = RT_FAIL;
      }
    }
    if (close(*cleanup->lockFD) != 0)
      rtLog_Warn("Failed to close direct socket lock: %s", strerror(errno));
  }
  if (haveDirectoryGate)
  {
    if (flock(*cleanup->directoryFD, LOCK_UN) != 0)
    {
      rtLog_Error("Failed to release direct lock cleanup gate: %s", strerror(errno));
      *cleanup->result = RT_FAIL;
    }
  }
  if (*cleanup->directoryFD >= 0 && close(*cleanup->directoryFD) != 0)
    rtLog_Warn("Failed to close direct socket directory: %s", strerror(errno));
}

rtError
rtRouteDirect_StartInstance(const char* socket_name, rtDriectClientHandler messageHandler)
{
  int ret;
  rtRouteEntry* route = NULL;
  rtConnectedClient* myDirectClient = NULL;
  rtListener* myDirectListener = NULL;
  rtError result = RT_OK;
  char socketDirectory[64];
  const char* socketBasename = NULL;
  int socketDirectoryFD = -1;
  bool ownsSocketPath = false;
  int socketLockFD = -1;
  int previousCancelState;

  if (NULL == socket_name)
  {
    rtLog_Warn("NULL Socket Name to listen to");
    return RT_ERROR;
  }

  int cancelError = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previousCancelState);
  if (cancelError != 0)
  {
    rtLog_Error("Failed to disable direct listener cancellation: %s", strerror(cancelError));
    return rtErrorFromErrno(cancelError);
  }
  rtDirectInstanceCleanup cleanup = {&route, &myDirectClient, &myDirectListener,
      &socketDirectoryFD, &socketLockFD, &ownsSocketPath, &socketBasename,
      messageHandler, &result};
  pthread_cleanup_push(rtRouteDirect_CleanupInstance, &cleanup);

  if (strncmp(socket_name, "unix://", 7) == 0)
  {
    char const* socket_path = &socket_name[7];
    int directoryLength = snprintf(socketDirectory, sizeof(socketDirectory),
        RT_DIRECT_SOCKET_DIRECTORY_FORMAT, (unsigned long)geteuid());
    if (directoryLength < 0 || (size_t)directoryLength >= sizeof(socketDirectory) ||
        strncmp(socket_path, socketDirectory, (size_t)directoryLength) != 0 ||
        socket_path[directoryLength] != '/' ||
        strncmp(socket_path + directoryLength + 1, ".direct_", 8) != 0 ||
        socket_path[directoryLength + 9] == '\0' ||
        strchr(socket_path + directoryLength + 1, '/') != NULL ||
        strstr(socket_path + directoryLength + 1, "..") != NULL ||
        strnlen(socket_path, sizeof(((struct sockaddr_un*)0)->sun_path)) >=
            sizeof(((struct sockaddr_un*)0)->sun_path))
    {
      rtLog_Warn("Rejected unsafe private Unix socket path: %s", socket_path);
      result = RT_ERROR_INVALID_ARG;
      goto finish;
    }
    socketBasename = socket_path + directoryLength + 1;
    socketDirectoryFD = rtRouteBase_OpenDirectDirectory(socketDirectory);
    if (socketDirectoryFD < 0)
    {
      result = RT_FAIL;
      goto finish;
    }
    socketLockFD = rtRouteBase_LockDirectSocket(socketDirectoryFD, socketBasename);
    if (socketLockFD < 0)
    {
      result = RT_FAIL;
      goto finish;
    }
    result = rtRouteBase_RemoveDirectSocket(socketDirectoryFD, socketBasename);
    if (result != RT_OK)
      goto finish;
    ownsSocketPath = true;
  }

  pthread_once(&_rtDirectRoute_once, &_rtdirect_route_thread_specific_key);
  pthread_setspecific(_rtDirectRoute_key, messageHandler);

  if (RT_OK != rtRouteBase_BindListener(socket_name, 1, 1, &myDirectListener))
  {
    result = RT_FAIL;
    goto finish;
  }

  route = (rtRouteEntry *)rt_malloc(sizeof(rtRouteEntry));
  if (!route)
  {
    result = rtErrorFromErrno(ENOMEM);
    goto finish;
  }
  route->subscription = NULL;
  rtString_Copy(route->expression, "_RTDIRECT>", RTMSG_MAX_EXPRESSION_LEN);
  route->message_handler = _rtdirect_OnMessage;

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
      _rtdirect_pushFd(&read_fds, myDirectListener->fd, &max_fd);
      _rtdirect_pushFd(&err_fds, myDirectListener->fd, &max_fd);
    }

    if (myDirectClient)
    {
      _rtdirect_pushFd(&read_fds, myDirectClient->fd, &max_fd);
      _rtdirect_pushFd(&err_fds, myDirectClient->fd, &max_fd);
    }

    /* Deliver cancellation only where no callbacks or registry locks are active. */
    pthread_setcancelstate(previousCancelState, NULL);
    ret = select(max_fd + 1, &read_fds, NULL, &err_fds, &timeout);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if (ret == 0)
      continue;

    if (ret == -1)
    {
      rtLog_Warn("select:%s", rtStrError(errno));
      continue;
    }

    if (myDirectListener && FD_ISSET(myDirectListener->fd, &read_fds))
    {
      rtLog_Debug("This should be called only once as there should be only one client");
      myDirectClient = rtRouteDirect_AcceptClientConnection(myDirectListener);
    }

    if (myDirectClient && FD_ISSET(myDirectClient->fd, &read_fds))
    {
      rtError err = rtConnectedClient_Read(myDirectClient, route);
      if (err != RT_OK)
      {
        rtLog_Info("Private Client Exited. Lets shutdown the session");
        break;
      }
    }
  }

finish:
  pthread_cleanup_pop(1);
  pthread_setcancelstate(previousCancelState, NULL);
  return result;
}
