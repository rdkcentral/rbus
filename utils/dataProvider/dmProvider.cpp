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
#include <cstring>
#include <iostream>

#include "dmProvider.h"
#include <rtError.h>
#include <rtLog.h>

dmProvider::dmProvider() : m_isTransaction(false)
{
}

dmProvider::~dmProvider()
{
}

void
dmProvider::doGet(std::vector<dmPropertyInfo> const& params, dmQueryResult& result)
{
  for (auto const& propInfo : params)
  {
    auto itr = m_provider_functions.find(propInfo.name());
    if ((itr != m_provider_functions.end()) && (itr->second.getter != nullptr))
    {
      dmValue val = itr->second.getter();
      result.addValue(propInfo, val);
    }
    else
    {
      dmQueryResult temp;
      doGet(propInfo, temp);
      if (temp.status() == RT_PROP_NOT_FOUND || temp.values().size() == 0)
      {
        rtLog_Debug("property:%s not found", propInfo.name().c_str());
        result.addValue(propInfo, "", RT_PROP_NOT_FOUND, "Property not found");
      }
      else 
      {
        result.addValue(propInfo, temp.values()[0].Value);
      }
    }
  }
}

void
dmProvider::doGet(dmPropertyInfo const& /*param*/, dmQueryResult& /*result*/)
{
  // EMPTY: overridden by the user 
}

void
dmProvider::doSet(std::vector<dmNamedValue> const& params, dmQueryResult& result)
{
  bool doTransaction(params.size() > 1);
  if(doTransaction)
  {
    m_isTransaction = true;
    beginTransaction();
  }
  for (auto const& value : params)
  {
    if(!value.info().isWritable())
    {
      rtLog_Debug("property:%s not writable", value.name().c_str());
      result.addValue(value.info(), "", RT_ERROR, "Property not writable");
      continue;
    }
    auto itr = m_provider_functions.find(value.name());
    if ((itr != m_provider_functions.end()) && (itr->second.setter != nullptr))
    {
      itr->second.setter(value.value());
      result.addValue(value.info(), value.value());
    }
    else
    {
      dmQueryResult temp;
      doSet(value.info(), value.value(), temp);
      if (temp.status() == RT_PROP_NOT_FOUND || temp.values().size() == 0)
      {
        rtLog_Debug("property:%s not found", value.name().c_str());
        result.addValue(value.info(), "", RT_PROP_NOT_FOUND, "Property not found");
      }
      else 
      {
        result.addValue(value.info(), temp.values()[0].Value);
      }
    }
  }
  if(doTransaction)
  {
    endTransaction();
    m_isTransaction = false;
  }
}

void
dmProvider::doSet(dmPropertyInfo const& /*info*/, dmValue const& /*value*/, dmQueryResult& /*result*/)
{
  // EMPTY: overridden by the user 
}


// inserts function callback
void
dmProvider::onGet(std::string const& propertyName, getter_function func)
{
  auto itr = m_provider_functions.find(propertyName);
  if (itr != m_provider_functions.end())
  {
    itr->second.getter = func;
  }
  else
  {
    provider_functions funcs;
    funcs.setter = nullptr;
    funcs.getter = func;
    m_provider_functions.insert(std::make_pair(propertyName, funcs));
  }
}

// inserts function callback
void
dmProvider::onSet(std::string const& propertyName, setter_function func)
{
  auto itr = m_provider_functions.find(propertyName);
  if (itr != m_provider_functions.end())
  {
    itr->second.setter = func;
  }
  else
  {
    provider_functions funcs;
    funcs.setter = func;
    funcs.getter = nullptr;
    m_provider_functions.insert(std::make_pair(propertyName, funcs));
  }
}
