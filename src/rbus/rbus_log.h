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

#define RBUSLOG_TRACE(format, ...)       rtLog_DebugPrint("RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_DEBUG(format, ...)       rtLog_DebugPrint("RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_INFO(format, ...)        rtLog_InfoPrint("RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_WARN(format, ...)        rtLog_WarnPrint("RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_ERROR(format, ...)       rtLog_ErrorPrint("RBUS", format"\n", ##__VA_ARGS__)
#define RBUSLOG_FATAL(format, ...)       rtLog_FatalPrint("RBUS", format"\n", ##__VA_ARGS__)

#endif

/** @} */
