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
#include "dmError.h"
#include <stdarg.h>

dmError::dmError(int code, std::string const& s)
  : m_what(s)
  , m_code(code)
{
}

dmError::~dmError()
{
}

void
dmError::throwError(int code, char const* fmt, ...)
{
  int const kBufferSize = 256;

  char buff[kBufferSize];

  va_list argp;
  va_start(argp, fmt);
  int n = vsnprintf(buff, kBufferSize, fmt, argp);
  if (n >= kBufferSize)
    buff[kBufferSize - 1] = '\0';
  else
    buff[n] = '\0';
  va_end(argp);

  throw dmError(code, buff);
}
