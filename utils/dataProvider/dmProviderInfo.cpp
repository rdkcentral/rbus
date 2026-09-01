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
#include "dmProviderInfo.h"
#include "rtLog.h"

dmProviderInfo::dmProviderInfo()
  : m_objectName()
  , m_providerName()
  , m_props()
  , m_isList(false)
{
}

void dmProviderInfo::setProviderName(std::string const& name)
{
  m_providerName = name;
}

void dmProviderInfo::setObjectName(std::string const& name)
{
  m_objectName = name;
}

void dmProviderInfo::addProperty(dmPropertyInfo const& propInfo)
{
  m_props.push_back(propInfo);
}


dmPropertyInfo
dmProviderInfo::getPropertyInfo(char const* propertyName) const
{
  if (!propertyName)
    return dmPropertyInfo();

  std::string s(propertyName);
  std::string::size_type idx = s.rfind('.');
  if (idx != std::string::npos)
    s = s.substr(idx + 1);

  for (const dmPropertyInfo& info : m_props)
  {
    if (info.name() == s)
      return info;
  }
  return dmPropertyInfo();
}
