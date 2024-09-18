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
#include "rtConnection.h"
#include "rtCipher.h"
#include "rtEncoder.h"
#include "rtError.h"
#include "rtLog.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"
#include "rtList.h"
#include "rtRetainable.h"
#include "rtTime.h"
#include "rtSemaphore.h"
#include "rtMemory.h"
#include <dlfcn.h>

#if defined(__GNUC__)                                                          \
    && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 8)))           \
    && !defined(NO_ATOMICS)
#define C11_ATOMICS_SUPPORTED 1
#include <stdatomic.h>
#else
typedef volatile int atomic_uint_least32_t;
#define _GNU_SOURCE 1
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>

#define RTMSG_LISTENERS_MAX 128
#define RTCONNECTION_CREATE_EXPRESSION_ID 1
#ifdef  RDKC_BUILD
#define RTMSG_SEND_BUFFER_SIZE (1024 * 8)
#else
#define RTMSG_SEND_BUFFER_SIZE (1024 * 64)
#endif /* RDKC_BUILD */
#ifndef SOL_TCP
#define SOL_TCP 6
#endif

#ifdef WITH_SPAKE2
#include "secure_wrapper.h"
#endif

#define DEFAULT_SEND_BUFFER_SIZE 1024

#ifdef  RDKC_BUILD
#define DEFAULT_MAX_RETRIES -1
#else
#define DEFAULT_MAX_RETRIES 3
#endif

extern char* __progname;

struct _rtListener
{
  int                     in_use;
  void*                   closure;
  char*                   expression;
  uint32_t                subscription_id;
  rtMessageCallback       callback;
};

struct _rtConnection
{
  int                     fd;
  struct sockaddr_storage local_endpoint;
  struct sockaddr_storage remote_endpoint;
  uint8_t*                send_buffer;
  int                     send_buffer_in_use;
  uint8_t*                recv_buffer;
  int                     recv_buffer_capacity;
  atomic_uint_least32_t   sequence_number;
  char*                   application_name;
  int                     max_retries;
  rtConnectionState       state;
  char                    inbox_name[RTMSG_HEADER_MAX_TOPIC_LENGTH];
  struct _rtListener      listeners[RTMSG_LISTENERS_MAX];
  pthread_mutex_t         mutex;
  rtList                  pending_requests_list;
  rtList                  callback_message_list;
  rtMessageCallback       default_callback;
  void*                   default_closure;
  unsigned int            run_threads;
  pthread_t               reader_thread;
  pthread_t               callback_thread;
  pthread_mutex_t         callback_message_mutex;
  pthread_cond_t          callback_message_cond;
  pthread_mutex_t         reconnect_mutex;
  rtTime_t                reader_reconnect_time;
  rtTime_t                sender_reconnect_time;
  rtTime_t                reconnect_time;
  rtTime_t                start_time;
  int                     reconnect_in_progress;
#ifdef WITH_SPAKE2
  rtCipher*               cipher;
  uint8_t*                encryption_buffer;
  uint8_t*                decryption_buffer;
  rtMessage               spakeconfig;
  int                     check_remote_router;
#endif
  pid_t                   read_tid;
};

typedef struct _rtMessageInfo
{
  rtRetainable            retainable;
  rtMessageHeader         header; 
  void*                   userData;
  uint32_t                dataLength;
  uint32_t                dataCapacity;
  uint8_t*                data;
  uint8_t                 block1[RTMSG_SEND_BUFFER_SIZE];
} rtMessageInfo;

typedef struct 
{
  uint32_t sequence_number;
  rtSemaphore sem;
  rtMessageInfo* response;
}pending_request;

typedef struct _rtCallbackMessage
{
  rtMessageHeader hdr;
  rtMessage msg;
} rtCallbackMessage;

static int g_taint_packets = 0; 
static int rtConnection_StartThreads(rtConnection con);
static int rtConnection_StopThreads(rtConnection con);
static rtError rtConnection_Read(rtConnection con, int32_t timeout);

static inline bool rtConnection_IsSecure(rtConnection con)
{
#ifdef WITH_SPAKE2
  return (con->cipher != NULL);
#else
  (void)con;
  return false;
#endif
}

static void rtMessageInfo_Init(rtMessageInfo** pim)
{
    (*pim) = rt_try_malloc(sizeof(struct _rtMessageInfo));
    if(!(*pim))
      return;
    (*pim)->retainable.refCount = 0;
    rtMessageHeader_Init(&(*pim)->header);
    (*pim)->userData = NULL;
    (*pim)->dataLength = 0;
    (*pim)->dataCapacity = RTMSG_SEND_BUFFER_SIZE;
    (*pim)->data = (*pim)->block1;
    rtRetainable_retain(*pim);
}

static void rtMessageInfo_Destroy(rtRetainable* r)
{
  rtMessageInfo* mi = (rtMessageInfo*)r;

  if(mi->data && mi->data != mi->block1)
    free(mi->data);

  free(mi);
}

static void rtMessageInfo_Release(rtMessageInfo* mi)
{
  rtRetainable_release(mi, rtMessageInfo_Destroy);
}

static void rtMessageInfo_ListItemFree(void* p)
{
    rtMessageInfo_Release((rtMessageInfo*)p);
}

static inline bool rtMessageInfo_IsEncrypted(rtMessageInfo* msginfo)
{
  return msginfo->header.flags & rtMessageFlags_Encrypted;
}

static void onDefaultMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  struct _rtConnection* con = (struct _rtConnection *) closure;

  if(con->default_callback)
  {
    con->default_callback(hdr, buff, n, con->default_closure);
  }
}

static rtError rtConnection_SendInternal(
  rtConnection con,  
  uint8_t const* buff,
  uint32_t bullLen, 
  char const* topic,
  char const* reply_topic, 
  int flags, 
  uint32_t sequence_number,
  uint32_t T1,
  uint32_t T2,
  uint32_t T3);
  
rtError
rtConnection_SendRequestInternal(
  rtConnection con, 
  uint8_t const* pReq, 
  uint32_t nReq, 
  char const* topic,
  rtMessageInfo** resMsg, 
  int32_t timeout, 
  int flags);

static uint32_t
rtConnection_GetNextSubscriptionId()
{
  static uint32_t next_id = 10000; /* Keeping this number high to avoid conflict with the subscription Id added in rbusSubscriptions_addSubscription() which starts with 1 */
  return next_id++;
}

static int GetRunThreadsSync(rtConnection con)
{
  int run_threads;
  pthread_mutex_lock(&con->mutex);
  run_threads = con->run_threads;
  pthread_mutex_unlock(&con->mutex);
  return run_threads;
}

static void SetRunThreadsSync(rtConnection con, int run_threads)
{
  pthread_mutex_lock(&con->mutex);
  con->run_threads = run_threads;
  pthread_mutex_unlock(&con->mutex);
}

static int
rtConnection_ShouldReregister(rtError e)
{
  if (rtErrorFromErrno(ENOTCONN) == e) return 1;
  if (rtErrorFromErrno(EPIPE) == e) return 1;
  return 0;
}

static rtError
rtConnection_ConnectAndRegister(rtConnection con, rtTime_t* reconnect_time)
{
  int i = 1;
  int ret = 0;
  int fdManip = 0;
  uint16_t remote_port = 0;
  uint16_t local_port = 0;
  char remote_addr[128] = {0};
  char local_addr[128] = {0};
  socklen_t socket_length;
  int first_to_handle = false;
  int is_first_connect = true;
  int reconnect_in_progress = 0;
  rtTime_t last_reconnect_time = con->reconnect_time;
  char tbuff1[100];
  char tbuff2[100];
  char tbuff3[100];

  if (!con)
    return rtErrorFromErrno(EINVAL);

  if(con->fd != -1)
    is_first_connect = false;

  if(is_first_connect)
  {
    rtLog_Debug("ConnectAndRegister creating first connection");
    rtTime_Now(&con->reconnect_time);
  }
  else
  {
    pthread_mutex_lock(&con->reconnect_mutex);

    reconnect_in_progress = con->reconnect_in_progress;

    if(!con->reconnect_in_progress)
    {
      if(rtTime_Compare(reconnect_time, &con->reconnect_time) > 0)
      {
        first_to_handle = true;
        con->reconnect_in_progress = true;
        rtTime_Now(&con->reconnect_time);
      }
    }
    pthread_mutex_unlock(&con->reconnect_mutex);  

    rtLog_Debug("ConnectAndRegister reconnect state first_to_handle=%d reconnect_in_progress=%d threads_time=[%s] last_reconnect_time=[%s] new_reconnect_time=[%s]", 
      first_to_handle, reconnect_in_progress, 
      rtTime_ToString(&con->start_time, tbuff1), 
      rtTime_ToString(&last_reconnect_time, tbuff2),
      rtTime_ToString(&con->reconnect_time, tbuff3));

    if(!first_to_handle)
    {
      if(con->reconnect_in_progress)
      {
        rtLog_Debug("ConnectAndRegister waiting for reconnect start");
        while(con->reconnect_in_progress)
          usleep(100000);
        rtLog_Debug("ConnectAndRegister waiting for reconnect end");
      }
      else
      {
        rtLog_Debug("ConnectAndRegister reconnect previously complete");
      }
      return RT_OK;
    }
  }

  rtSocketStorage_GetLength(&con->remote_endpoint, &socket_length);

  if (con->fd != -1)
    close(con->fd);

  rtLog_Debug("connecting to router");
  con->fd = socket(con->remote_endpoint.ss_family, SOCK_STREAM, 0);
  if (con->fd == -1)
    return rtErrorFromErrno(errno);
  rtLog_Debug("router connection up");

  fdManip = fcntl(con->fd, F_GETFD);
  if (fdManip < 0)
    return rtErrorFromErrno(errno);

  fdManip = fcntl(con->fd, F_SETFD, fdManip | FD_CLOEXEC);
  if (fdManip < 0)
    return rtErrorFromErrno(errno);

  setsockopt(con->fd, SOL_TCP, TCP_NODELAY, &i, sizeof(i));

  rtSocketStorage_ToString(&con->remote_endpoint, remote_addr, sizeof(remote_addr), &remote_port);

  int retry = 0;

  while (retry < con->max_retries || con->max_retries < 0 /*unlimited retries*/)
  {
    rtLog_Debug("trying connect");
    ret = connect(con->fd, (struct sockaddr *)&con->remote_endpoint, socket_length);
    if (ret == -1)
    {
      int err = errno;
      if (err == ECONNREFUSED || (err == ENOENT && con->remote_endpoint.ss_family == AF_UNIX))
      {
        rtLog_Debug("no connection yet. will retry");
      }
      else
      {
        rtLog_Warn("unexpected error %s:%d. err:%d=%s. will retry", remote_addr, remote_port, err, strerror(err));
      }
#if 0
      rtConnection_EnsureRoutingDaemon();
#endif
      sleep(1);
      retry++;
    }
    else
    {
      rtLog_Debug("connection made");
      break;
    }
  }

  if(-1 == ret) {
    rtLog_Warn("failed to connected after %d attempts", retry);
    con->reconnect_in_progress = 0;
    return RT_NO_CONNECTION;
  }

  rtSocket_GetLocalEndpoint(con->fd, &con->local_endpoint);

  rtSocketStorage_ToString(&con->local_endpoint, local_addr, sizeof(local_addr), &local_port);
  rtLog_Debug("connect %s:%d -> %s:%d", local_addr, local_port, remote_addr, remote_port);

  /*first connect return here*/
  if(is_first_connect)
  {
    con->reconnect_in_progress = 0;
    return RT_OK;
  }

  /*on reconnect, add back all listeners */
  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if (con->listeners[i].in_use)
    {
      rtMessage m;
      rtMessage_Create(&m);
      rtMessage_SetInt32(m, "add", 1);
      rtMessage_SetString(m, "topic", con->listeners[i].expression);
      rtMessage_SetInt32(m, "route_id", con->listeners[i].subscription_id);
      rtConnection_SendMessage(con, m, "_RTROUTED.INBOX.SUBSCRIBE");
      rtMessage_Release(m);

      /*TODO: we need to readd all the aliases too -- would this allow rbus to recover from broker crash ?*/
    }
  }

#ifdef WITH_SPAKE2
  /*on reconnect, rerun key exchange since broker would have lost our key if it restarted */
  if(con->cipher)
  {
     rtLog_Debug("Recreating cipher");
     rtCipher_Destroy(con->cipher);
      ret = rtCipher_CreateCipherSpake2Plus(&con->cipher, con->spakeconfig);
      if(ret != RT_OK)
      {
        rtLog_Error("failed to initialize spake2 based cipher");
        //rtConnection_Destroy(con);
        con->reconnect_in_progress = 0;
        return ret;
      }

    ret = rtCipher_RunKeyExchangeClient(con->cipher, con);
    if (ret != RT_OK)
    {
      rtLog_Error("failed to do key exchange");
      //rtConnection_Destroy(con);
      con->reconnect_in_progress = 0;
      return ret;
    }
  }
#endif
  con->reconnect_in_progress = 0;
  return RT_OK;
}

static rtError
rtConnection_ReadUntil(rtConnection con, uint8_t* buff, int count, int32_t timeout)
{
  ssize_t bytes_read = 0;
  ssize_t bytes_to_read = count;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  (void) timeout;

  while (bytes_read < bytes_to_read)
  {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(con->fd, &read_fds);

    if ((0 < timeout) && (timeout != INT32_MAX))
    {
      // TODO: include suseconds_t tv_usecs;
      time_t seconds = (timeout / 1000);
      struct timeval tv = { seconds, 0 };
      select(con->fd + 1, &read_fds, NULL, NULL, &tv);
      if (!FD_ISSET(con->fd, &read_fds))
        return RT_ERROR_TIMEOUT;
    }

    ssize_t n = recv(con->fd, buff + bytes_read, (bytes_to_read - bytes_read), MSG_NOSIGNAL);
    if (n == 0)
    {
      if(0 != con->run_threads)
        rtLog_Error("Failed to read error : %s", rtStrError(rtErrorFromErrno(ENOTCONN)));
      return rtErrorFromErrno(ENOTCONN);
    }

    if (n == -1)
    {
      if (errno == EINTR)
        continue;
      rtError e = rtErrorFromErrno(errno);
      rtLog_Error("failed to read from fd %d. %s.  errno=%d", con->fd, rtStrError(e), errno);
      return e;
    }
    bytes_read += n;
  }
  return RT_OK;
}


static rtError
rtConnection_CreateInternal(rtConnection* con, char const* application_name, char const* router_config, int max_retries)
{
  int i = 0;
  rtError err = RT_OK;

  rtConnection c = (rtConnection) rt_try_malloc(sizeof(struct _rtConnection));
  if (!c)
    return rtErrorFromErrno(ENOMEM);

  memset(c, 0, sizeof(struct _rtConnection));

  pthread_mutexattr_t mutex_attribute;
  pthread_mutexattr_init(&mutex_attribute);
  pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_ERRORCHECK);
  if (0 != pthread_mutex_init(&c->mutex, &mutex_attribute) ||
      0 != pthread_mutex_init(&c->callback_message_mutex, &mutex_attribute) ||
      0 != pthread_mutex_init(&c->reconnect_mutex, &mutex_attribute))
  {
    rtLog_Error("Could not initialize mutex. Cannot create connection.");
    free(c);
    return RT_ERROR;
  }
  pthread_cond_init(&c->callback_message_cond, NULL);
  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    c->listeners[i].in_use = 0;
    c->listeners[i].closure = NULL;
    c->listeners[i].callback = NULL;
    c->listeners[i].subscription_id = 0;
  }
  c->send_buffer_in_use = 0;
  c->send_buffer = (uint8_t *) rt_try_malloc(RTMSG_SEND_BUFFER_SIZE);
  if(!c->send_buffer)
    return rtErrorFromErrno(ENOMEM);
  c->recv_buffer = (uint8_t *) rt_try_malloc(RTMSG_SEND_BUFFER_SIZE);
  if(!c->recv_buffer)
    return rtErrorFromErrno(ENOMEM);
  c->recv_buffer_capacity = RTMSG_SEND_BUFFER_SIZE;
  c->sequence_number = 1;
#ifdef C11_ATOMICS_SUPPORTED
  atomic_init(&(c->sequence_number), 1);
#endif
  c->application_name = strdup(application_name);
  c->max_retries = max_retries;
  /*if user config tries to set 0, we just override with 1, as 0 makes no sense currently*/
  if(c->max_retries == 0)
    c->max_retries = 1;
  c->fd = -1;
  rtList_Create(&c->pending_requests_list);
  rtList_Create(&c->callback_message_list);
  c->default_callback = NULL;
  c->run_threads = 0;
  c->read_tid = 0;
  c->reconnect_in_progress = 0;
  rtTime_Now(&c->start_time);
#ifdef WITH_SPAKE2
  c->cipher = NULL;
  c->encryption_buffer = NULL;
  c->decryption_buffer = NULL;
#endif
  memset(c->inbox_name, 0, RTMSG_HEADER_MAX_TOPIC_LENGTH);
  memset(&c->local_endpoint, 0, sizeof(struct sockaddr_storage));
  memset(&c->remote_endpoint, 0, sizeof(struct sockaddr_storage));
  memset(c->send_buffer, 0, RTMSG_SEND_BUFFER_SIZE);
  memset(c->recv_buffer, 0, RTMSG_SEND_BUFFER_SIZE);
  snprintf(c->inbox_name, RTMSG_HEADER_MAX_TOPIC_LENGTH, "%s.%s.INBOX.%d", c->application_name, __progname, (int) getpid());
  err = rtSocketStorage_FromString(&c->remote_endpoint, router_config);
  if (err != RT_OK)
  {
    rtLog_Warn("failed to parse:%s. %s", router_config, rtStrError(err));
    free(c->send_buffer);
    free(c->recv_buffer);
    free(c->application_name);
    rtList_Destroy(c->pending_requests_list,NULL);
    rtList_Destroy(c->callback_message_list, NULL);
    free(c);
    return err;
  }
  err = rtConnection_ConnectAndRegister(c, 0);
  if (err != RT_OK)
  {
    // TODO: at least log this
    rtLog_Warn("rtConnection_ConnectAndRegister(1):%d", err);
    free(c->send_buffer);
    free(c->recv_buffer);
    free(c->application_name);
    rtList_Destroy(c->pending_requests_list,NULL);
    rtList_Destroy(c->callback_message_list, NULL);
    free(c);
  }

  if (err == RT_OK)
  {
    rtConnection_AddListenerWithId(c, c->inbox_name, RTCONNECTION_CREATE_EXPRESSION_ID, onDefaultMessage, c);
    rtConnection_StartThreads(c);
    *con = c;
  }

  return err;
}

rtError
rtConnection_Create(rtConnection* con, char const* application_name, char const* router_config)
{
  rtError rt_err = rtConnection_CreateInternal(con, application_name, router_config, DEFAULT_MAX_RETRIES);
  if (rt_err)
    *con = NULL;
  return rt_err;
}

rtError
rtConnection_CreateWithConfig(rtConnection* con, rtMessage const conf)
{
  rtError err = RT_OK;
  char const* application_name = NULL;
  char const* router_config = NULL;
  int start_router = 0;
  int max_retries = DEFAULT_MAX_RETRIES;
  int remote_router=0;

  rtMessage_GetString(conf, "appname", &application_name);
  rtMessage_GetString(conf, "uri", &router_config);
  rtMessage_GetInt32(conf, "start_router", &start_router);
  rtMessage_GetInt32(conf, "max_retries", &max_retries);
  rtMessage_GetInt32(conf, "check_remote_router",&remote_router);

  err = rtConnection_CreateInternal(con, application_name, router_config, max_retries);
  #ifdef WITH_SPAKE2
  (*con)->check_remote_router=remote_router;
  #endif

#ifdef WITH_SPAKE2
  if(err == RT_OK)
  {
    char const* spake2_psk = NULL;

    rtMessage_GetString(conf, "spake2_psk", &spake2_psk);

    if(spake2_psk)
    {
      rtLog_Info("enabling secure messaging");

      rtMessage_Clone(conf, &(*con)->spakeconfig);

      err = rtCipher_CreateCipherSpake2Plus(&(*con)->cipher, conf);
      if(err != RT_OK)
      {
        rtLog_Error("failed to initialize spake2 based cipher");
        rtConnection_Destroy(*con);
        return err;
      }

      err = rtCipher_RunKeyExchangeClient((*con)->cipher, *con);
      if (err != RT_OK)
      {
        rtLog_Error("failed to do key exchange");
        rtConnection_Destroy(*con);
        return err;
      }

      (*con)->encryption_buffer = rt_try_malloc(RTMSG_SEND_BUFFER_SIZE);
      if(!(*con)->encryption_buffer)
        return rtErrorFromErrno(ENOMEM);
      memset((*con)->encryption_buffer, 0, RTMSG_SEND_BUFFER_SIZE);

      (*con)->decryption_buffer = rt_try_malloc(RTMSG_SEND_BUFFER_SIZE);
      if(!(*con)->decryption_buffer)
        return rtErrorFromErrno(ENOMEM);
      memset((*con)->decryption_buffer, 0, RTMSG_SEND_BUFFER_SIZE);

    }
  }
#endif

  return err;
}

rtError
rtConnection_Destroy(rtConnection con)
{
  if (con)
  {
    unsigned int i;
    SetRunThreadsSync(con, 0);
    
    if (con->fd != -1)
      shutdown(con->fd, SHUT_RDWR);
    
    rtConnection_StopThreads(con);
    
    if (con->fd != -1)
      close(con->fd);
    if (con->send_buffer)
      free(con->send_buffer);
    if (con->recv_buffer)
      free(con->recv_buffer);
    if (con->application_name)
      free(con->application_name);
#ifdef WITH_SPAKE2
    if (con->cipher)
      rtCipher_Destroy(con->cipher);
    if (con->encryption_buffer)
      free(con->encryption_buffer);
    if (con->decryption_buffer)
      free(con->decryption_buffer);
#endif
    for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
    {
      if (con->listeners[i].in_use)
        free(con->listeners[i].expression);
    }
    /*Unblock all threads waiting for RPC responses.*/
    pthread_mutex_lock(&con->mutex);
    int found_pending_requests = 0;

    rtListItem listItem;
    for(rtList_GetFront(con->pending_requests_list, &listItem); 
        listItem != NULL; 
        rtListItem_GetNext(listItem, &listItem))
    {
      pending_request *entry;
      rtListItem_GetData(listItem, (void**)&entry);

      found_pending_requests = 1;
      rtSemaphore_Post(entry->sem);
    }
    rtList_Destroy(con->pending_requests_list,NULL);
    rtList_Destroy(con->callback_message_list, rtMessageInfo_ListItemFree);
    pthread_mutex_unlock(&con->mutex);
    if(0 != found_pending_requests)
    {
      rtLog_Error("Warning! Found pending requests while destroying connection.");
      sleep(1); /* ugly hack to allow all sendRequest() calls to return and stop using con->* data members. Hopefully, this will never be 
      executed in practice. Revisit if necessary. */
    }

    pthread_mutex_destroy(&con->mutex);
    pthread_mutex_destroy(&con->callback_message_mutex);
    pthread_cond_destroy(&con->callback_message_cond);
    pthread_mutex_destroy(&con->reconnect_mutex);

    free(con);
  }
  return 0;
}

rtError
rtConnection_SendMessage(rtConnection con, rtMessage msg, char const* topic)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  return rtConnection_SendMessageDirect(con, msg, topic, NULL);
}

rtError
rtConnection_SendMessageDirect(rtConnection con, rtMessage msg, char const* topic, char const* listener)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  rtTime_Now(&con->sender_reconnect_time);
  while(1)
  {
    uint8_t* p;
    uint32_t n;
    rtError err;
    uint32_t sequence_number;
    rtMessage_ToByteArrayWithSize(msg, &p, DEFAULT_SEND_BUFFER_SIZE, &n);  /*FIXME unification is this needed ? rtMessage_FreeByteArray(p);*/

    pthread_mutex_lock(&con->mutex);
#ifdef C11_ATOMICS_SUPPORTED
    sequence_number = atomic_fetch_add_explicit(&con->sequence_number, 1, memory_order_relaxed);
#else
    sequence_number = __sync_fetch_and_add(&con->sequence_number, 1);
#endif
    err = rtConnection_SendInternal(con, p, n, topic, listener, 0, sequence_number, 0, 0, 0);
    pthread_mutex_unlock(&con->mutex);
    rtMessage_FreeByteArray(p);

    if(err == RT_NO_CONNECTION)
    {
      err = rtConnection_ConnectAndRegister(con, &con->sender_reconnect_time);
      if(err != RT_OK)
        return err;
      else
        continue;
    }
    return err;
  }
}

rtError
rtConnection_SendRequest(rtConnection con, rtMessage const req, char const* topic,
  rtMessage* res, int32_t timeout)
{
  uint8_t* p;
  uint32_t n;
  rtMessageInfo* resMsg;
  rtError err;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  rtMessage_ToByteArrayWithSize(req, &p, DEFAULT_SEND_BUFFER_SIZE, &n);
  err = rtConnection_SendRequestInternal(con, p, n, topic, &resMsg, timeout, 0);
  rtMessage_FreeByteArray(p);
  if(err == RT_OK)
  {
    rtMessage_FromBytes(res, resMsg->data, resMsg->dataLength);
    rtMessageInfo_Release(resMsg);
  }
  return err;
}

rtError
rtConnection_SendResponse(rtConnection con, rtMessageHeader const* request_hdr, rtMessage const res, int32_t timeout)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  rtTime_Now(&con->sender_reconnect_time);
  while(1)
  {
    (void)timeout;
    rtError err;
    uint8_t* p;
    uint32_t n;

    rtMessage_ToByteArrayWithSize(res, &p, DEFAULT_SEND_BUFFER_SIZE, &n);
    pthread_mutex_lock(&con->mutex);
  //TODO: should we send response on reconnect ?
    err = rtConnection_SendInternal(con, p, n, request_hdr->reply_topic, request_hdr->topic, rtMessageFlags_Response, request_hdr->sequence_number, 0, 0, 0);
    pthread_mutex_unlock(&con->mutex);
    rtMessage_FreeByteArray(p);

    if(err == RT_NO_CONNECTION)
    {
      err = rtConnection_ConnectAndRegister(con, &con->sender_reconnect_time);
      if(err != RT_OK)
        return err;
      else
        continue;
    }

    return err;
  }
}

rtError
rtConnection_SendBinary(rtConnection con, uint8_t const* p, uint32_t n, char const* topic)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  return rtConnection_SendBinaryDirect(con, p, n, topic, NULL);
}

rtError
rtConnection_SendBinaryDirect(rtConnection con, uint8_t const* p, uint32_t n, char const* topic,
  char const* listener)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  rtTime_Now(&con->sender_reconnect_time);
  while(1)
  {
    rtError err;
    uint32_t sequence_number;

#ifdef C11_ATOMICS_SUPPORTED
    sequence_number = atomic_fetch_add_explicit(&con->sequence_number, 1, memory_order_relaxed);
#else
    sequence_number = __sync_fetch_and_add(&con->sequence_number, 1);
#endif

    pthread_mutex_lock(&con->mutex);
    err = rtConnection_SendInternal(con, p, n, topic, listener, rtMessageFlags_RawBinary, sequence_number, 0, 0, 0);
    pthread_mutex_unlock(&con->mutex);

    if(err == RT_NO_CONNECTION)
    {
      err = rtConnection_ConnectAndRegister(con, &con->sender_reconnect_time);
      if(err != RT_OK)
        return err;
      else
        continue;
    }

    return err;
  }
}

rtError
rtConnection_SendBinaryRequest(rtConnection con, uint8_t const* pReq, uint32_t nReq, char const* topic,
  uint8_t** pRes, uint32_t* nRes, int32_t timeout)
{
  rtError err;
  rtMessageInfo* mi;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  err = rtConnection_SendRequestInternal(con, pReq, nReq, topic, &mi, timeout, rtMessageFlags_RawBinary);
  if(err == RT_OK)
  {
    if(mi->data)
    {
      if(mi->data != mi->block1)
      {
        *nRes = mi->dataLength;
        *pRes = mi->data;
        mi->data = NULL; /*pass heap ownership to caller*/
      }
      else
      {
        /*alloc on heap b/c data will be freed by rtMessageInfo_Release*/
        *nRes = mi->dataLength;
        *pRes = rt_try_malloc(mi->dataLength);
        if(!*pRes)
          return rtErrorFromErrno(ENOMEM);
        memcpy(*pRes, mi->data, mi->dataLength);
      }
    }
    rtMessageInfo_Release(mi);
  }
  return err;
}

rtError
rtConnection_SendBinaryResponse(rtConnection con, rtMessageHeader const* request_hdr, uint8_t const* p, uint32_t n,
  int32_t timeout)
{
  (void) timeout;
  rtError err;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  pthread_mutex_lock(&con->mutex);
#ifdef MSG_ROUNDTRIP_TIME
  err = rtConnection_SendInternal(con, p, n, request_hdr->reply_topic, request_hdr->topic,
    rtMessageFlags_Response|rtMessageFlags_RawBinary, request_hdr->sequence_number, request_hdr->T1, request_hdr->T2, request_hdr->T3);
#else
  err = rtConnection_SendInternal(con, p, n, request_hdr->reply_topic, request_hdr->topic,
    rtMessageFlags_Response|rtMessageFlags_RawBinary, request_hdr->sequence_number, 0, 0, 0);
#endif

  pthread_mutex_unlock(&con->mutex);
  return err;
}

rtError
rtConnection_SendRequestInternal(rtConnection con, uint8_t const* pReq, uint32_t nReq, char const* topic,
  rtMessageInfo** res, int32_t timeout, int flags)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  rtTime_Now(&con->sender_reconnect_time);
  while(1)
  {
    rtError ret = RT_OK;
    uint8_t const* p = pReq;
    uint32_t n = nReq;
    rtError err;
    uint32_t sequence_number;
    rtListItem listItem;

    pid_t tid = syscall(__NR_gettid);

    pthread_mutex_lock(&con->mutex);
#ifdef C11_ATOMICS_SUPPORTED
    sequence_number = atomic_fetch_add_explicit(&con->sequence_number, 1, memory_order_relaxed);
#else
    sequence_number = __sync_fetch_and_add(&con->sequence_number, 1);
#endif
    /*Populate the pending request and enqueue it.*/
    pending_request queue_entry;
    queue_entry.sequence_number = sequence_number;
    rtSemaphore_Create(&queue_entry.sem);
    queue_entry.response = NULL;

    rtList_PushFront(con->pending_requests_list, (void*)&queue_entry, &listItem);
    err = rtConnection_SendInternal(con, p, n, topic, con->inbox_name, rtMessageFlags_Request | flags, sequence_number, 0, 0, 0);
    if (err != RT_OK)
    {
      ret = err;
      goto dequeue_and_continue;
    }
    pthread_mutex_unlock(&con->mutex);

    if(tid != con->read_tid)
    {
      rtTime_t timeout_time;
      rtTime_Later(NULL, timeout, &timeout_time);
      ret = rtSemaphore_TimedWait(queue_entry.sem, &timeout_time); //TODO: handle wake triggered by signals
    }
    else
    {
      //Handle nested dispatching.
      rtTime_t start_time;
      rtTime_Now(&start_time);
      do
      {
        if((err = rtConnection_Read(con, timeout)) == RT_OK)
        {
          int sem_value = 0;
          rtSemaphore_GetValue(queue_entry.sem, &sem_value);
          if(0 < sem_value)
          {
            ret = RT_OK;
            break;
          }
          else
          {
            //It's a response to a different message. Adjust the timeout value and try again.
            int diff_ms = rtTime_Elapsed(&start_time, NULL);
            if(timeout <= diff_ms)
            {
              ret = RT_ERROR_TIMEOUT;
              break;
            }
            else
            {
              timeout -= diff_ms;
              //rtLog_Info("Retry nested call with timeout of %d ms", timeout);
            }
          }
        }
        else if(err == RT_NO_CONNECTION)
        {
          ret = err;
          goto dequeue_and_continue;
        }
        else
        {
          rtLog_Error("Nested read failed.");
          ret = RT_ERROR;
          break;
        }
      } while(RT_OK == err);
    }
    if(RT_OK == ret)
    {
      /*Sem posted*/
      pthread_mutex_lock(&con->mutex);

      if(queue_entry.response)
      {
        if(queue_entry.response->header.flags & rtMessageFlags_Undeliverable)
        {
          rtMessageInfo_Release(queue_entry.response);

          ret = RT_OBJECT_NO_LONGER_AVAILABLE;
        }
        else
        {
          /*caller must call rtMessageInfo_Release on the response*/

          *res = queue_entry.response; 
        }
      }
      else
      {
        /*For some reason, we unblocked, but there's no data.*/
        ret = RT_ERROR;
      }
    }

dequeue_and_continue:
    rtList_RemoveItem(con->pending_requests_list, listItem, NULL);
    pthread_mutex_unlock(&con->mutex);
    rtSemaphore_Destroy(queue_entry.sem);

    if(ret == RT_NO_CONNECTION)
    {
      ret = rtConnection_ConnectAndRegister(con, &con->sender_reconnect_time);
      if(ret == RT_OK)
        continue;
    }

    if(ret == RT_ERROR_TIMEOUT)
    {
      rtLog_Info("rtConnection_SendRequest TIMEOUT");
    }

    return ret;
  }
}

rtError
rtConnection_SendInternal(rtConnection con, uint8_t const* buff, uint32_t n, char const* topic,
  char const* reply_topic, int flags, uint32_t sequence_number, uint32_t T1, uint32_t T2, uint32_t T3)
{
  rtError err;
  int num_attempts;
  int max_attempts;
  ssize_t bytes_sent;
  rtMessageHeader header;
  uint8_t const* message =NULL;
  uint32_t message_length;

  if (!con)
    return rtErrorFromErrno(EINVAL);

#ifdef MSG_ROUNDTRIP_TIME
  rtTime_t send_time;
  rtTime_Now(&send_time);
#endif

#ifdef WITH_SPAKE2
  /*encrypte all non-internal messages*/
  if (rtConnection_IsSecure(con) && topic[0] != '_')
  { 
    rtLog_Debug("encrypting message");

    if ((err = rtCipher_Encrypt(con->cipher, buff, n, con->encryption_buffer, RTMSG_SEND_BUFFER_SIZE, &message_length)) != RT_OK)
    {
      rtLog_Error("failed to encrypt payload, not sending message. %s", rtStrError(err));
      return err;
    }

    message = (uint8_t *)con->encryption_buffer;
    flags |= rtMessageFlags_Encrypted;
  }
  else
#endif
  {
    message = (uint8_t *) buff;
    message_length = n;
  }

  max_attempts = 3;
  num_attempts = 0;

  rtMessageHeader_Init(&header);
  header.payload_length = message_length;

  if(topic)
  {
    strncpy(header.topic, topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
    header.topic_length = strlen(header.topic);
  }
  if (reply_topic)
  {
    strncpy(header.reply_topic, reply_topic, RTMSG_HEADER_MAX_TOPIC_LENGTH-1);
    header.reply_topic_length = strlen(reply_topic);
  }
  else
  {
    header.reply_topic[0] = '\0';
    header.reply_topic_length = 0;
  }
  header.sequence_number = sequence_number; 
  header.flags = flags;
#ifdef MSG_ROUNDTRIP_TIME
  if(header.flags & rtMessageFlags_Request)
  {
       header.T1 = send_time.tv_sec;
  }
  if(header.flags & rtMessageFlags_Response)
  {
       header.T1 = T1;
       header.T2 = T2;
       header.T3 = T3;
       header.T4 = send_time.tv_sec;
  }
#else
  (void)T1;
  (void)T2;
  (void)T3;
#endif
#ifdef ENABLE_ROUTER_BENCHMARKING
  if(1 == g_taint_packets)
    header.flags |= rtMessageFlags_Tainted;
#endif
  if(con->send_buffer_in_use)
    rtLog_Error("send_buffer in use!");

  con->send_buffer_in_use=1;
  err = rtMessageHeader_Encode(&header, con->send_buffer);
  if (err != RT_OK)
  {
    con->send_buffer_in_use=0;
    return err;
  }

  struct iovec send_vec[] = {{con->send_buffer, header.header_length}, {(void *)message, header.payload_length}};
  struct msghdr send_hdr = {NULL, 0, send_vec, 2, NULL, 0, 0};

  do
  {
    err = RT_OK;
    bytes_sent = sendmsg(con->fd, &send_hdr, MSG_NOSIGNAL);
    if (bytes_sent != (ssize_t)(header.header_length + header.payload_length))
    {
      if (bytes_sent == -1)
        err = rtErrorFromErrno(errno);
      else
        err = RT_FAIL;
    }

    if (err != RT_OK && rtConnection_ShouldReregister(err))
    {
      con->send_buffer_in_use=0;
      return RT_NO_CONNECTION;
    }
  }
  while ((err != RT_OK) && (num_attempts++ < max_attempts));
  con->send_buffer_in_use=0;

  return err;
}

rtError
rtConnection_AddListener(rtConnection con, char const* expression, rtMessageCallback callback, void* closure)
{
    return rtConnection_AddListenerWithId(con, expression, rtConnection_GetNextSubscriptionId(), callback, closure);
}

rtError
rtConnection_AddListenerWithId(rtConnection con, char const* expression, uint32_t expressionId, rtMessageCallback callback, void* closure)
{
  int i;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  pthread_mutex_lock(&con->mutex);

  /*find an open listener to use*/
  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if (!con->listeners[i].in_use)
      break;
  }

  if (i >= RTMSG_LISTENERS_MAX)
  {
    pthread_mutex_unlock(&con->mutex);
    return rtErrorFromErrno(ENOMEM);
  }

  con->listeners[i].in_use = 1;
  con->listeners[i].subscription_id = expressionId;
  con->listeners[i].closure = closure;
  con->listeners[i].callback = callback;
  con->listeners[i].expression = strdup(expression);
  pthread_mutex_unlock(&con->mutex);
  
  rtMessage m;
  rtMessage_Create(&m);
  rtMessage_SetInt32(m, "add", 1);
  rtMessage_SetString(m, "topic", expression);
  rtMessage_SetInt32(m, "route_id", con->listeners[i].subscription_id); 
  rtConnection_SendMessage(con, m, "_RTROUTED.INBOX.SUBSCRIBE");
  rtMessage_Release(m);

  return RT_OK;
}

rtError
rtConnection_RemoveListener(rtConnection con, char const* expression)
{
  int i;
  int route_id = 0;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  pthread_mutex_lock(&con->mutex);
  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if ((con->listeners[i].in_use) && (0 == strcmp(expression, con->listeners[i].expression)))
    {
        con->listeners[i].in_use = 0;
        route_id = con->listeners[i].subscription_id;
        con->listeners[i].subscription_id = 0;
        con->listeners[i].closure = NULL;
        con->listeners[i].callback = NULL;
        free(con->listeners[i].expression);
        con->listeners[i].expression = NULL;
        break;
    }
  }
  pthread_mutex_unlock(&con->mutex);

  if (i >= RTMSG_LISTENERS_MAX)
    return RT_ERROR_INVALID_ARG;

  rtMessage m;
  rtMessage_Create(&m);
  rtMessage_SetInt32(m, "add", 0);
  rtMessage_SetString(m, "topic", expression);
  rtMessage_SetInt32(m, "route_id", route_id);
  rtConnection_SendMessage(con, m, "_RTROUTED.INBOX.SUBSCRIBE");
  rtMessage_Release(m);
  return 0;
}

rtError
rtConnection_RemoveListenerWithId(rtConnection con, char const* expression, uint32_t expressionId)
{
  int i;
  int route_id = 0;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  pthread_mutex_lock(&con->mutex);
  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if ((con->listeners[i].in_use) && (expressionId == con->listeners[i].subscription_id))
    {
        con->listeners[i].in_use = 0;
        route_id = con->listeners[i].subscription_id;
        con->listeners[i].subscription_id = 0;
        con->listeners[i].closure = NULL;
        con->listeners[i].callback = NULL;
        free(con->listeners[i].expression);
        con->listeners[i].expression = NULL;
        break;
    }
  }
  pthread_mutex_unlock(&con->mutex);

  if (i >= RTMSG_LISTENERS_MAX)
    return RT_ERROR_INVALID_ARG;

  rtMessage m;
  rtMessage_Create(&m);
  rtMessage_SetInt32(m, "add", 0);
  rtMessage_SetString(m, "topic", expression);
  rtMessage_SetInt32(m, "route_id", route_id);
  rtConnection_SendMessage(con, m, "_RTROUTED.INBOX.SUBSCRIBE");
  rtMessage_Release(m);
  return 0;
}

rtError
rtConnection_AddAlias(rtConnection con, char const* existing, const char *alias)
{
  int i;
  rtError ret = RT_OK;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if (1 == con->listeners[i].in_use)
    {
      if(0 == strncmp(con->listeners[i].expression, existing, (strlen(con->listeners[i].expression) + 1)))
      {
        rtMessage m;
        rtMessage res;
        rtMessage_Create(&m);
        rtMessage_SetInt32(m, "add", 1);
        rtMessage_SetString(m, "topic", alias);
        rtMessage_SetInt32(m, "route_id", con->listeners[i].subscription_id); 
        ret = rtConnection_SendRequest(con, m, "_RTROUTED.INBOX.SUBSCRIBE", &res, 6000);
        if(RT_OK == ret)
        {
            int result = 0;
            rtMessage_GetInt32(res, "result", &result);
            ret = result;
            if(RT_ERROR_DUPLICATE_ENTRY == result)
                rtLog_Error("Failed to register %s. Duplicate entry", alias);
            else if (RT_ERROR_PROTOCOL_ERROR == result)
                rtLog_Error("Failed to register %s because the scaler or table is already registered", alias);
            rtMessage_Release(res);
        }
        rtMessage_Release(m);
        break;
      }
    }
  }

  if (i >= RTMSG_LISTENERS_MAX)
    return rtErrorFromErrno(ENOMEM);

  return ret;
}
rtError
rtConnection_RemoveAlias(rtConnection con, char const* existing, const char *alias)
{
  int i;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
  {
    if (1 == con->listeners[i].in_use)
    {
      if(0 == strncmp(con->listeners[i].expression, existing, (strlen(con->listeners[i].expression) + 1)))
      {
        rtMessage m;
        rtMessage_Create(&m);
        rtMessage_SetInt32(m, "add", 0);
        rtMessage_SetString(m, "topic", alias);
        rtMessage_SetInt32(m, "route_id", con->listeners[i].subscription_id); 
        rtConnection_SendMessage(con, m, "_RTROUTED.INBOX.SUBSCRIBE");
        rtMessage_Release(m);
        break;
      }
    }

  }

  if (i >= RTMSG_LISTENERS_MAX)
    return rtErrorFromErrno(ENOMEM);

  return 0;
}
rtError
rtConnection_AddDefaultListener(rtConnection con, rtMessageCallback callback, void* closure)
{
  if (!con)
    return rtErrorFromErrno(EINVAL);

  con->default_callback = callback;
  con->default_closure = closure;
  return 0;
}

rtError
_rtConnection_ReadAndDropBytes(int fd, unsigned int bytes_to_read)
{
  uint8_t buff[512];

  while (0 < bytes_to_read)
  {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    
    ssize_t n = recv(fd, buff, (sizeof(buff) > bytes_to_read ? bytes_to_read : sizeof(buff)), MSG_NOSIGNAL);
    if (n == 0)
    {
      rtLog_Error("Failed to read error : %s", rtStrError(rtErrorFromErrno(ENOTCONN)));
      return rtErrorFromErrno(ENOTCONN);
    }

    if (n == -1)
    {
      if (errno == EINTR)
        continue;
      rtError e = rtErrorFromErrno(errno);
      rtLog_Error("failed to read from fd %d. %s", fd, rtStrError(e));
      return e;
    }
    bytes_to_read -= n;
  }
  return RT_OK;
}

rtError
rtConnection_Read(rtConnection con, int32_t timeout)
{
  int num_attempts;
  int max_attempts;
  uint8_t const*  itr;
  rtMessageInfo* msginfo;
  rtError err;

  num_attempts = 0;
  max_attempts = 4;

  rtMessageInfo_Init(&msginfo);
  if(!msginfo)
    return rtErrorFromErrno(ENOMEM);

  if (!con)
  {
    rtMessageInfo_Release(msginfo);
    return rtErrorFromErrno(EINVAL);
  }

  // TODO: no error handling right now, all synch I/O

  do
  {
    con->state = rtConnectionState_ReadHeaderPreamble;
    err = rtConnection_ReadUntil(con, con->recv_buffer, RTMESSAGEHEADER_PREAMBLE_LENGTH, timeout);

    if (err == RT_ERROR_TIMEOUT)
    {
        rtMessageInfo_Release(msginfo); 
        return err;
    }

    if (err == RT_OK)
    {
      itr = &con->recv_buffer[RTMESSAGEHEADER_HDR_LENGTH_OFFSET];
      rtEncoder_DecodeUInt16(&itr, &msginfo->header.header_length);
      err = rtConnection_ReadUntil(con, con->recv_buffer + RTMESSAGEHEADER_PREAMBLE_LENGTH,
          (msginfo->header.header_length-RTMESSAGEHEADER_PREAMBLE_LENGTH), timeout);
    }
    else
    {
      /* Read failed. If this is due to a connection termination initiated by us, break and return. Retry if anything else.*/
      int run_thread = GetRunThreadsSync(con);
      if (0 == run_thread)
      {
        break;
      }
    }

    if (err == RT_OK)
    {
      err = rtMessageHeader_Decode(&msginfo->header, con->recv_buffer);
    }

    // TODO: need to check for secure message here and decrypt
    // the msginfo->data and capacity might need to adjusted

    if (err == RT_OK)
    {
      if(msginfo->dataCapacity < msginfo->header.payload_length + 1)
      {
        msginfo->data = (uint8_t *)rt_try_malloc(msginfo->header.payload_length + 1);
        if(!msginfo->data){
          rtMessageInfo_Release(msginfo);
          return rtErrorFromErrno(ENOMEM);
        }
        msginfo->dataCapacity = msginfo->header.payload_length + 1;
      }

      err = rtConnection_ReadUntil(con, msginfo->data, msginfo->header.payload_length, timeout);

      if (err == RT_OK)
      {
        msginfo->data[msginfo->header.payload_length] = '\0';
        msginfo->dataLength = msginfo->header.payload_length;

#ifdef WITH_SPAKE2
        if (rtMessageInfo_IsEncrypted(msginfo))
        {
          if (rtConnection_IsSecure(con))
          {
            uint32_t decryptedLength;

            rtLog_Debug("decrypting message");

            err = rtCipher_Decrypt(con->cipher, msginfo->data, msginfo->dataLength, con->decryption_buffer, RTMSG_SEND_BUFFER_SIZE, &decryptedLength);

            if (err != RT_OK)
            {
              rtLog_Error("failed to decrypted payload: %s", rtStrError(err));
            }
            else
            {
              memcpy(msginfo->data, con->decryption_buffer, decryptedLength);
              msginfo->data[decryptedLength] = '\0';
              msginfo->dataLength = decryptedLength;
            }
          }
          else
          {
            rtLog_Error("cannot decrypt message without cipher");
          }
        }
#endif
      }
    }

    if (err != RT_OK && rtConnection_ShouldReregister(err))
    {
        rtMessageInfo_Release(msginfo);
        return RT_NO_CONNECTION;
    }
  }
  while ((err != RT_OK) && (num_attempts++ < max_attempts));

  if (err == RT_OK)
  {
    if (msginfo->header.flags & rtMessageFlags_Response)
    {
      /*response message must be handle right here in this thread
        because rtConnection_SendRequest is waiting on the response.
        We do not queue responses into the callback_message_list
        because this can lead to lock ups such as RDKB-26837
      */
      pthread_mutex_lock(&con->mutex);
      rtListItem listItem;
      for(rtList_GetFront(con->pending_requests_list, &listItem); 
          listItem != NULL; 
          rtListItem_GetNext(listItem, &listItem))
      {
        pending_request *entry;
        rtListItem_GetData(listItem, (void**)&entry);
        if(entry->sequence_number == msginfo->header.sequence_number)
        {
          entry->response = msginfo;
          msginfo = NULL; /*rtConnection_SendRequest thread will release it*/
          rtSemaphore_Post(entry->sem);
          break;
        }
      }
      pthread_mutex_unlock(&con->mutex);
#ifdef MSG_ROUNDTRIP_TIME
      /* The listItem is not present in the pending_requests_list, as it is been removed from the list because of request timeout */
      if(listItem == NULL)
      {
        rtMessage m;
        rtMessage_Create(&m);
        rtMessage_SetInt32(m, "T1", msginfo->header.T1);
        rtMessage_SetInt32(m, "T2", msginfo->header.T2);
        rtMessage_SetInt32(m, "T3", msginfo->header.T3);
        rtMessage_SetInt32(m, "T4", msginfo->header.T4);
        rtMessage_SetInt32(m, "T5", msginfo->header.T5);
        rtMessage_SetString(m, "topic", msginfo->header.topic);
        rtMessage_SetString(m, "reply_topic", msginfo->header.reply_topic);
        rtConnection_SendMessage(con, m, RTROUTED_TRANSACTION_TIME_INFO);
        rtMessage_Release(m);
      }
#endif
    }
    else
    {
      /*request message must be dispatched to the Callback thread*/
      rtListItem listItem;

      pthread_mutex_lock(&con->callback_message_mutex);

      rtList_PushBack(con->callback_message_list, msginfo, &listItem);
      msginfo = NULL; /*the callback thread will release it*/

      /*log something if the callback thread isn't processing fast enough*/
      size_t size;
      rtList_GetSize(con->callback_message_list, &size);
      if(size >= 5)
      {
        if(size == 5 || size == 10 || size == 20 || size == 40 || size == 80)
          rtLog_Debug("callback_message_list has reached %lu", (unsigned long)size);
        else if(size >= 100)
          rtLog_Debug("callback_message_list has reached %lu", (unsigned long)size);
      }

      /*wake the callback thread up to process new message*/
      pthread_cond_signal(&con->callback_message_cond);

      pthread_mutex_unlock(&con->callback_message_mutex);
    }
  }

  /*if the message wasn't sent off to another thread then release it*/
  if(msginfo)
  {
    rtMessageInfo_Release(msginfo);
  }

  return RT_OK;
}

#ifdef WITH_SPAKE2
void check_router(rtConnection con)
{
  char remote_addr[128] = {0};
  uint16_t remote_port = 0;
  int x;
  void *handle;
  char *error;
  int (*fptr)(const char *format, ...);

  if (!con)
    return;

  rtSocketStorage_ToString(&con->remote_endpoint, remote_addr, sizeof(remote_addr), &remote_port);
  rtLog_Info("remote addr is %s \n",remote_addr);
  if(remote_addr != NULL)
  {
    handle = dlopen ("/usr/lib/libsecure_wrapper.so", RTLD_LAZY);
    if (!handle) 
    {
      fputs (dlerror(), stderr);
      exit(1);
    }
    fptr = dlsym(handle, "v_secure_system");
    error = dlerror();
    if (error != NULL) 
    {
      printf( "!!! %s\n", error );
      return;
    }
    x=(*fptr)("ping -c 1 %s > /dev/null",remote_addr);
    if(x != 0)
    {
      rtLog_Info("reconnecting from reader thread \n");
      rtConnection_ConnectAndRegister(con, &con->reader_reconnect_time);
    }
    dlclose(handle);
   }
}
#endif

/*
  RDKB-26837: added rtConnection_CallbackThread to decouple
  reading message from the socket (what rtConnection_ReaderThread does)
  from executing the listener callbacks which can block.
  This prevents rtConnection_ReaderThread from getting blocked by callbacks 
  so that it can continue to read incoming message.
  Importantly, it allows rtConnection_ReaderThread to handle Response messages  
  for threads which have called rtConnection_SendRequest.  In RDKB-26837,
  rtConnection_ReaderThread was executing a callback directly which
  blocked on an application mutex being help by another thread
  attempting to call rtConnection_SendRequest.   Since the reader thread
  was blocked, it could not read the response message the SendRequest 
  was waiting on.  
*/
static void * rtConnection_CallbackThread(void *data)
{
  rtConnection con = (rtConnection)data;
  rtLog_Debug("Callback thread started");

  while (1 == GetRunThreadsSync(con))
  {
    size_t size;
    rtListItem listItem;

    pthread_mutex_lock(&con->callback_message_mutex);

    /*Must check run_threads after lock in order to stay synced with rtConnection_StopThreads*/
    if (0 == GetRunThreadsSync(con))
    {
      pthread_mutex_unlock(&con->callback_message_mutex);
      break;
    }

    rtList_GetSize(con->callback_message_list, &size);

    if (size == 0)
    {
      //rtLog_Error("Callback thread before wait");
      pthread_cond_wait(&con->callback_message_cond, &con->callback_message_mutex);
      //rtLog_Error("Callback thread after wait");
    }

    /*get first item to handle*/
    rtList_GetFront(con->callback_message_list, &listItem);

    pthread_mutex_unlock(&con->callback_message_mutex);

    if (0 == GetRunThreadsSync(con))
    {
      break;
    }
    /*Execute listener callbacks for all messages in callback_message_list.
      Remove messages from list as you go and return once the list is empty.
      Very important to not keep any mutex lock while executing the callback*/
    while(listItem != NULL)
    {
      int i;
      rtMessageInfo* msginfo = NULL;
      rtMessageCallback callback = NULL;

      rtListItem_GetData(listItem, (void**)&msginfo);

      pthread_mutex_lock(&con->mutex);

      /*check for controlled exit*/
      if(0 == con->run_threads)
      {
        pthread_mutex_unlock(&con->mutex);
        break;
      }

      /*find the listener for the msg*/
      for (i = 0; i < RTMSG_LISTENERS_MAX; ++i)
      {
        if (con->listeners[i].in_use && (con->listeners[i].subscription_id == msginfo->header.control_data))
        {
          callback = con->listeners[i].callback;
          msginfo->userData = con->listeners[i].closure;
          break;
        }
      }

      pthread_mutex_unlock(&con->mutex);

      /*process the message without locking any mutex*/
      if(callback)
      {
          //rtLog_Error("rtConnection_CallbackThread before callback");
          callback(&msginfo->header, msginfo->data, msginfo->dataLength, msginfo->userData);
          //rtLog_Error("rtConnection_CallbackThread after callback");
      }
      else
      {
        //rtLog_Error("rtConnection_CallbackThread no callback found for message");
      }

      pthread_mutex_lock(&con->callback_message_mutex);

      /*remove item. pass NULL so data can be reused*/
      rtList_RemoveItem(con->callback_message_list, listItem, rtMessageInfo_ListItemFree);

      /*get next item to handle from front*/
      rtList_GetFront(con->callback_message_list, &listItem);

      size_t size;
      rtList_GetSize(con->callback_message_list, &size);
      //rtLog_Error("Remove callback_message_list size=%d", size);

      pthread_mutex_unlock(&con->callback_message_mutex);
    }
  }
  rtLog_Debug("Callback thread exiting");
  return NULL;
}

static void * rtConnection_ReaderThread(void *data)
{
  rtError err = RT_OK;
  rtConnection con = (rtConnection)data;
  con->read_tid = syscall(__NR_gettid);
  rtLog_Debug("Reader thread started");
  while (1 == GetRunThreadsSync(con))
  {
    rtTime_Now(&con->reader_reconnect_time);
    if((err = rtConnection_Read(con, -1)) != RT_OK)
    {
      if (0 == GetRunThreadsSync(con))
      {
        break;
      }
      #ifdef WITH_SPAKE2
      rtLog_Debug("Reader failed with error 0x%x.", err);
      #else
      rtLog_Error("Reader failed with error 0x%x.", err);
      #endif
      if(err == RT_NO_CONNECTION)
      {
        /*
         rtConnection_ConnectAndRegister is also called from all the send api functions,
         and if it fails to connect in those functions, they simply return error to the caller.
         However, here in the Reader thread, we are forced to just keep retrying indefinitely.
         On rdkc its fine because they want unlimited retries anyways.
         On rdkb if rtrouted crashes rbus is hosed irregardless of reconnect.
         need to check rdkv
        */
        while (1 == GetRunThreadsSync(con))
        {
            err = rtConnection_ConnectAndRegister(con, &con->reader_reconnect_time);
            if (err == RT_OK || 1 != GetRunThreadsSync(con))
                break;
            sleep(5);
            rtTime_Now(&con->reader_reconnect_time);//set this time to take ownership of reconnect
        }
      }
      else
      {
        sleep(5); //Avoid tight loops if we have an unrecoverable situation.
      }
      #ifdef WITH_SPAKE2
      if(con->check_remote_router==1)
      {
       check_router(con);
      }
      #endif
    }
  }
  rtLog_Debug("Reader thread exiting");
  return NULL;
}

static int rtConnection_StartThreads(rtConnection con)
{
  int ret = 0;

  if (!con)
    return rtErrorFromErrno(EINVAL);

  if(0 == con->run_threads)
  {
    con->run_threads = 1;
    if(0 != pthread_create(&con->reader_thread, NULL, rtConnection_ReaderThread, (void *)con))
    {
      rtLog_Error("Unable to launch reader thread.");
      ret = RT_ERROR;
    }

    if(0 != pthread_create(&con->callback_thread, NULL, rtConnection_CallbackThread, (void *)con))
    {
      rtLog_Error("Unable to launch callback thread.");
      ret = RT_ERROR;
    }
  }
  return ret;
}

static int rtConnection_StopThreads(rtConnection con)
{
  rtLog_Info("Stopping threads");

  SetRunThreadsSync(con, 0);

  pthread_mutex_lock(&con->callback_message_mutex);
  pthread_cond_signal(&con->callback_message_cond);
  pthread_mutex_unlock(&con->callback_message_mutex);

  pthread_join(con->reader_thread, NULL);
  pthread_join(con->callback_thread, NULL);
  return 0;
}


const char *
rtConnection_GetReturnAddress(rtConnection con)
{
  return con->inbox_name;
}

rtError
rtConnection_Dispatch(rtConnection con)
{
  (void)con;
  usleep(250000); /*sleep a bit but return to allow rdkc apps to terminate if necessary*/
  return RT_OK;
}

void
_rtConnection_TaintMessages(int i)
{
  g_taint_packets = i;
}
