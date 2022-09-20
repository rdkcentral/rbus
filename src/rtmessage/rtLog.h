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
# This file contains material from pxCore which is Copyright 2005-2018 John Robinson
# Licensed under the Apache-2.0 license.
*/
#ifndef RT_LOG_H_
#define RT_LOG_H_
#include <stdint.h>

#ifdef ENABLE_RDKLOGGER
#include "rdk_debug.h"
#endif

#ifdef __APPLE__
typedef uint64_t rtThreadId;
#define RT_THREADID_FMT PRIu64
#elif defined WIN32
typedef unsigned long rtThreadId;
#define RT_THREADID_FMT "l"
#else
typedef int32_t rtThreadId;
#define RT_THREADID_FMT "d"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  RT_LOG_DEBUG = 0,
  RT_LOG_INFO  = 1,
  RT_LOG_WARN  = 2,
  RT_LOG_ERROR = 3,
  RT_LOG_FATAL = 4
} rtLogLevel;

typedef enum
{
  RT_USE_RTLOGGER,
  RT_USE_RDKLOGGER
}rtLoggerSelection;

/*rdkc compatibility fix*/
#define rdkLog RT_USE_RDKLOGGER
#define rtLog RT_USE_RTLOGGER

typedef void (*rtLogHandler)(rtLogLevel level, const char* file, int line, int threadId, char* message);

void rtLog_SetLevel(rtLogLevel l);
rtLogLevel rtLog_GetLevel();

void rtLogSetLogHandler(rtLogHandler logHandler);
rtLogHandler rtLogGetLogHandler();

const char* rtLogLevelToString(rtLogLevel level);
rtLogLevel  rtLogLevelFromString(const char* s);

void rtLog_SetOption(rtLoggerSelection option);
rtLoggerSelection rtLog_GetOption();

#ifdef __GNUC__
#define RT_PRINTF_FORMAT(IDX, FIRST) __attribute__ ((format (printf, IDX, FIRST)))
#else
#define RT_PRINTF_FORMAT(IDX, FIRST)
#endif

void rtLogPrintf(rtLogLevel level, const char* file, int line, const char* format, ...) RT_PRINTF_FORMAT(4, 5);

#ifdef ENABLE_RDKLOGGER
#define rtLog_Debug(FORMAT,...)   do{if(rtLogGetLogHandler() || rtLog_GetOption() == RT_USE_RTLOGGER){rtLogPrintf(RT_LOG_DEBUG,__FILE__,__LINE__,FORMAT, ##__VA_ARGS__);}else{RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.RTMESSAGE",FORMAT"\n", ##__VA_ARGS__);}}while(0)
#define rtLog_Info(FORMAT, ...)   do{if(rtLogGetLogHandler() || rtLog_GetOption() == RT_USE_RTLOGGER){rtLogPrintf(RT_LOG_INFO, __FILE__,__LINE__,FORMAT, ##__VA_ARGS__);}else{RDK_LOG(RDK_LOG_INFO, "LOG.RDK.RTMESSAGE",FORMAT"\n", ##__VA_ARGS__);}}while(0)
#define rtLog_Warn(FORMAT, ...)   do{if(rtLogGetLogHandler() || rtLog_GetOption() == RT_USE_RTLOGGER){rtLogPrintf(RT_LOG_WARN, __FILE__,__LINE__,FORMAT, ##__VA_ARGS__);}else{RDK_LOG(RDK_LOG_WARN, "LOG.RDK.RTMESSAGE",FORMAT"\n", ##__VA_ARGS__);}}while(0)
#define rtLog_Error(FORMAT, ...)  do{if(rtLogGetLogHandler() || rtLog_GetOption() == RT_USE_RTLOGGER){rtLogPrintf(RT_LOG_ERROR,__FILE__,__LINE__,FORMAT, ##__VA_ARGS__);}else{RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.RTMESSAGE",FORMAT"\n", ##__VA_ARGS__);}}while(0)
#define rtLog_Fatal(FORMAT, ...)  do{if(rtLogGetLogHandler() || rtLog_GetOption() == RT_USE_RTLOGGER){rtLogPrintf(RT_LOG_FATAL,__FILE__,__LINE__,FORMAT, ##__VA_ARGS__);}else{RDK_LOG(RDK_LOG_FATAL,"LOG.RDK.RTMESSAGE",FORMAT"\n", ##__VA_ARGS__);}}while(0)
#else
#define rtLog_Debug(FORMAT...) rtLogPrintf(RT_LOG_DEBUG, __FILE__, __LINE__, FORMAT)
#define rtLog_Info(FORMAT...)  rtLogPrintf(RT_LOG_INFO,  __FILE__, __LINE__, FORMAT)
#define rtLog_Warn(FORMAT...)  rtLogPrintf(RT_LOG_WARN,  __FILE__, __LINE__, FORMAT)
#define rtLog_Error(FORMAT...) rtLogPrintf(RT_LOG_ERROR, __FILE__, __LINE__, FORMAT)
#define rtLog_Fatal(FORMAT...) rtLogPrintf(RT_LOG_FATAL, __FILE__, __LINE__, FORMAT)
#endif

#ifdef __cplusplus
}
#endif
#endif 

