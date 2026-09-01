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
#ifndef __DM_ERROR_H__
#define __DM_ERROR_H__

#include <exception>
#include <string>

class dmError : public std::exception
{
public:
  virtual ~dmError() throw();

  virtual char const* what() const throw()
    { return m_what.c_str(); }

  static void throwError(int code, char const* fmt, ...) __attribute__ ((format (printf, 2, 3)));

private:
  dmError(int code, std::string const& s);

private:
  std::string m_what;
  int m_code;
};

#endif
