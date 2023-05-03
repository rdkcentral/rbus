#ifndef __RTROUTEBASE_H__
#define __RTROUTEBASE_H__

#include "rtError.h"
#include "rtCipher.h"
#include "rtMessageHeader.h"
#include "rtSocket.h"
#include "rtConnection.h"


#ifndef SOL_TCP
#define SOL_TCP 6
#endif

#define RTMSG_MAX_CONNECTED_CLIENTS 64
#define RTMSG_CLIENT_MAX_TOPICS 64

#ifdef  RDKC_BUILD
    #define RTMSG_CLIENT_READ_BUFFER_SIZE (1024 * 8)
#else
    #define RTMSG_CLIENT_READ_BUFFER_SIZE (1024 * 64)
#endif /* RDKC_BUILD */



#define RTMSG_INVALID_FD -1
#define RTMSG_MAX_EXPRESSION_LEN 128
#define RTMSG_ADDR_MAX 128
#define RTMSG_MAX_LISTENING_SOCKETS 5

typedef struct
{
  int                       fd;
  struct sockaddr_storage   endpoint;
  char                      ident[RTMSG_ADDR_MAX];
  char                      inbox[RTMSG_HEADER_MAX_TOPIC_LENGTH];
  uint8_t*                  read_buffer;
  uint8_t*                  send_buffer;
  rtConnectionState         state;
  int                       bytes_read;
  int                       bytes_to_read;
  int                       read_buffer_capacity;
  rtMessageHeader           header;
#ifdef WITH_SPAKE2
  rtCipher*                 cipher;
  uint8_t*                  encryption_key;
  uint8_t*                  encryption_buffer;
#endif
} rtConnectedClient;

typedef struct
{
  uint32_t id;
  rtConnectedClient* client;
} rtSubscription;

typedef struct
{
  int   clientFD;
  int   clientID;
  char  clientTopic[RTMSG_HEADER_MAX_TOPIC_LENGTH];
} rtPrivateClientInfo;

typedef rtError (*rtRouteMessageHandler) (rtConnectedClient* sender, rtMessageHeader* hdr, uint8_t const* buff, int n, rtSubscription* subscription);
typedef struct
{
  rtSubscription*       subscription;
  rtRouteMessageHandler message_handler;
  char                  expression[RTMSG_MAX_EXPRESSION_LEN];
} rtRouteEntry;

typedef struct
{
  int fd;
  struct sockaddr_storage local_endpoint;
} rtListener;

typedef rtError (*rtDriectClientHandler) (uint8_t isClientRequest, rtMessageHeader* hdr, uint8_t const* pInbuff, int inLength, uint8_t** pOutBuff, uint32_t* pOutLength);

rtError rtRouteBase_BindListener(char const* socket_name, int no_delay, int indefinite_retry, rtListener **pListener);
rtError rtRouteBase_CloseListener(rtListener *pListener);

rtError rtRouteDirect_StartInstance(const char* socket_name, rtDriectClientHandler messageHandler);
rtError rtRouteDirect_SendMessage(const rtPrivateClientInfo* pClient, uint8_t const* pInBuff, int inLength);

#endif /* __RTROUTEBASE_H__ */
