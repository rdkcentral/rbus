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
#include "dmPropertyInfo.h"

dmPropertyInfo::dmPropertyInfo()
  : m_name()
  , m_type(dmValueType_Unknown)
  , m_optional(false)
  , m_writable(false)
{
}

void dmPropertyInfo::setName(std::string const& name)
{
  m_name = name;
}

void dmPropertyInfo::setType(dmValueType t)
{
  m_type = t;
}

void dmPropertyInfo::setIsOptional(bool b)
{
  m_optional = b;
}

void dmPropertyInfo::setIsWritable(bool b)
{
  m_writable = b;
}

void
dmPropertyInfo::setFullName(std::string const& name)
{
  m_full_name = name;
}
