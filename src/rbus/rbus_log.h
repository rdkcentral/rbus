/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file        rbus_log.h
 * @brief       rbusLog 
 * @defgroup    rbusLog
 * @brief       An rbusLog is a method to print the logs and also providers the users of the library to override log handler.
 * @{
 */

#ifndef RBUS_LOG_H 
#define RBUS_LOG_H

#include <stdarg.h>
#include "rtLog.h"

#ifdef ENABLE_RDKLOGGER
#include "rdk_debug.h"

#define RBUSLOG_TRACE(format, ...)       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_DEBUG(format, ...)       RDK_LOG(RDK_LOG_DEBUG,  "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_INFO(format, ...)        RDK_LOG(RDK_LOG_INFO,   "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_WARN(format, ...)        RDK_LOG(RDK_LOG_WARN,   "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_ERROR(format, ...)       RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_FATAL(format, ...)       RDK_LOG(RDK_LOG_FATAL,  "LOG.RDK.RBUS", format"\n", ##__VA_ARGS__)

#else

#define RBUSLOG_TRACE rtLog_Debug
#define RBUSLOG_DEBUG rtLog_Debug
#define RBUSLOG_INFO rtLog_Info
#define RBUSLOG_WARN rtLog_Warn
#define RBUSLOG_ERROR rtLog_Error
#define RBUSLOG_FATAL rtLog_Fatal

#endif /* ENABLE_RDKLOGGER */
#endif

/** @} */
