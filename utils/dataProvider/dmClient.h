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
#ifndef __DM_CLIENT_H__
#define __DM_CLIENT_H__

#include "dmQueryResult.h"
#include "dmProviderOperation.h"
#include "rtLog.h"

class dmProviderProxy
{
public:
private:
};

class dmClientNotifier
{
public:
  virtual void onResult(const dmQueryResult& result) = 0;//1 onResult for non list query.  1 for each list item in a list query.
  virtual void onError(int status, std::string const& message) = 0;//1 onError if error for non list, 1 for each list item query that fails.
};

class dmClient
{
public:
  virtual ~dmClient() { }
  virtual bool runQuery(dmProviderOperation operation, std::string const& parameter, bool recursive, dmClientNotifier* notifier) = 0;

  static dmClient* create(std::string const& datamodelDir, rtLogLevel logLevel = RT_LOG_WARN);
  static void destroy(dmClient* client);
private:
};


#endif
