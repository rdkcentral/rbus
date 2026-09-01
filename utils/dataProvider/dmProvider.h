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
#ifndef __DM_PROVIDER_H__
#define __DM_PROVIDER_H__

#include "dmPropertyInfo.h"
#include "dmQueryResult.h"

#include <functional>
#include <map>
#include <vector>

class dmProvider
{
public:
  dmProvider();
  dmProvider(const char* alias);
  virtual ~dmProvider();

  virtual void doGet(std::vector<dmPropertyInfo> const& params, dmQueryResult& result);
  virtual void doSet(std::vector<dmNamedValue> const& params, dmQueryResult& result);

protected:
  virtual void doGet(dmPropertyInfo const& param, dmQueryResult& result);
  virtual void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result);

  using getter_function = std::function<dmValue (void)>;
  using setter_function = std::function<void (dmValue const& value)>;

  void onGet(std::string const& propertyName, getter_function func);
  void onSet(std::string const& propertyName, setter_function func);

  virtual void beginTransaction(){}
  virtual void endTransaction(){}
  bool isTransaction() const 
    { return m_isTransaction; }
private:
  struct provider_functions
  {
    getter_function getter;
    setter_function setter;
  };
  std::map< std::string, provider_functions > m_provider_functions;
  bool m_isTransaction;
};

#endif
