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
#ifndef __RBUS_CORE_LOG_H__
#define __RBUS_CORE_LOG_H__

#include <stdarg.h>

#ifdef ENABLE_RDKLOGGER
#include "rdk_debug.h"


#define RBUSCORELOG_ERROR(format, ...)       RDK_LOG(RDK_LOG_ERROR,  "LOG.RDK.RBUSCORE", format"\n", ##__VA_ARGS__)
#define RBUSCORELOG_WARN(format, ...)        RDK_LOG(RDK_LOG_WARN,   "LOG.RDK.RBUSCORE", format"\n", ##__VA_ARGS__)
#define RBUSCORELOG_INFO(format, ...)        RDK_LOG(RDK_LOG_INFO,   "LOG.RDK.RBUSCORE", format"\n", ##__VA_ARGS__)
#define RBUSCORELOG_DEBUG(format, ...)       RDK_LOG(RDK_LOG_DEBUG,  "LOG.RDK.RBUSCORE", format"\n", ##__VA_ARGS__)
#define RBUSCORELOG_TRACE(format, ...)       RDK_LOG(RDK_LOG_TRACE1, "LOG.RDK.RBUSCORE", format"\n", ##__VA_ARGS__)

#else
#include "rtLog.h"

#define RBUSCORELOG_ERROR(format...)        rtLog_Error(format)
#define RBUSCORELOG_WARN(format...)         rtLog_Warn(format)
#define RBUSCORELOG_INFO(format...)         rtLog_Info(format)
#define RBUSCORELOG_DEBUG(format...)        rtLog_Debug(format)
#define RBUSCORELOG_TRACE(format...)        rtLog_Trace(format)

#endif /* ENABLE_RDKLOGGER */

#endif  // __RBUS_CORE_LOG_H__
