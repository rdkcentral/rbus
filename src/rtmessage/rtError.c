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

// rtError.cpp

#include "rtError.h"
#include "rtMemory.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define RT_ERROR_CASE(ERR) case ERR: s = # ERR; break;
#define RT_ERROR_CLASS(ERR) (((ERR) >> 16) & 0x0000ffff)
#define RT_ERROR_CODE(ERR) ((ERR) & 0x0000ffff)

static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_key_t key;

static const char* rtStrError_BuiltIn(rtError e);
static const char* rtStrError_SystemError(int e);

typedef struct
{
  char error_message[256];
  rtError last_error;
} rtErrorThreadSpecific;

static void rtErrorInitThreadSpecificKey()
{
  (void) pthread_key_create(&key, free);
}

rtErrorThreadSpecific* getThreadSpecific()
{
  pthread_once(&once, rtErrorInitThreadSpecificKey);

  rtErrorThreadSpecific* specific = (rtErrorThreadSpecific *)(pthread_getspecific(key));
  if (specific == NULL)
  {
    specific = (rtErrorThreadSpecific *) rt_try_malloc(sizeof(rtErrorThreadSpecific));
    if(!specific)
      return NULL;
    specific->last_error = 0;
    specific->error_message[0] = '\0';
    pthread_setspecific(key, specific);
  }
  return specific;
}

const char* rtStrError(rtError e)
{
  const char* s = NULL;

  int klass = RT_ERROR_CLASS(e);
  switch (klass)
  {
    case RT_ERROR_CLASS_BUILTIN:
    s = rtStrError_BuiltIn(RT_ERROR_CODE(e));
    break;

    case RT_ERROR_CLASS_SYSERROR:
    s = rtStrError_SystemError(RT_ERROR_CODE(e));
    break;

    default:
      return "UNKNOWN_ERROR";
      break;
  }
  return s;

}

rtError rtErrorGetLastError()
{
  rtError current = 0;
  rtErrorThreadSpecific* specific = getThreadSpecific();
  if (specific)
  {
    current = specific->last_error;
    free(specific);
  }
  return current;
}

void rtErrorSetLastError(rtError e)
{
  rtErrorThreadSpecific* specific = getThreadSpecific();
  if (specific)
    specific->last_error = e;
}

const char* rtStrError_SystemError(int e)
{
  rtErrorThreadSpecific* specific = getThreadSpecific();

  char* buff = NULL;

  // TODO: The below #ifdef is not the best approach. @see man strerror_r and
  // /usr/include/string.h for proper check of which version of strerror_r is 
  // available. Need to check for osx portability also
  if (specific)
  {
    #ifdef __linux__
    int n = strerror_r(e, specific->error_message, 256);
    if (!n)
      buff = specific->error_message;
    #else
    buff = strerror(e);
    #endif
  }

  return buff;
}

const char* rtStrError_BuiltIn(rtError e)
{
  const char* s = "UNKNOWN";
  switch (e)
  {
    RT_ERROR_CASE(RT_OK);
    RT_ERROR_CASE(RT_FAIL);
    RT_ERROR_CASE(RT_ERROR_NOT_ENOUGH_ARGS);
    RT_ERROR_CASE(RT_ERROR_INVALID_ARG);
    RT_ERROR_CASE(RT_PROP_NOT_FOUND);
    RT_ERROR_CASE(RT_OBJECT_NOT_INITIALIZED);
    RT_ERROR_CASE(RT_PROPERTY_NOT_FOUND);
    RT_ERROR_CASE(RT_OBJECT_NO_LONGER_AVAILABLE);
    RT_ERROR_CASE(RT_RESOURCE_NOT_FOUND);
    RT_ERROR_CASE(RT_NO_CONNECTION);
    RT_ERROR_CASE(RT_ERROR_NOT_IMPLEMENTED);
    RT_ERROR_CASE(RT_ERROR_TYPE_MISMATCH);

    RT_ERROR_CASE(RT_ERROR_TIMEOUT);
    RT_ERROR_CASE(RT_ERROR_DUPLICATE_ENTRY);
    RT_ERROR_CASE(RT_ERROR_OBJECT_NOT_FOUND);
    RT_ERROR_CASE(RT_ERROR_PROTOCOL_ERROR);
    RT_ERROR_CASE(RT_ERROR_INVALID_OPERATION);
    RT_ERROR_CASE(RT_ERROR_IN_PROGRESS);
    RT_ERROR_CASE(RT_ERROR_QUEUE_EMPTY);
    RT_ERROR_CASE(RT_ERROR_STREAM_CLOSED);
    default:
      break;
  }
  return s;
}

rtError
rtErrorFromErrno(int err)
{
  // RT_ASSERT(err <= 65535); // uint16_max
  return err == 0 ? RT_OK : (err | (RT_ERROR_CLASS_SYSERROR << 16));
}
