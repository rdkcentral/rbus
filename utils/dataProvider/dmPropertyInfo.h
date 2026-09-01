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
#ifndef __DM_PROPERTY_INFO_H__
#define __DM_PROPERTY_INFO_H__

#include <string>

#include "dmValueType.h"

class dmProviderDatabase;

class dmPropertyInfo
{
  friend class dmProviderDatabase;
public:
  inline std::string const& name() const
    { return m_name; }

  inline dmValueType type() const
    { return m_type; }

  inline bool isOptional() const
    { return m_optional; }

  inline bool isWritable() const
    { return m_writable; }

  inline std::string const& fullName() const
    { return m_full_name; }

public:
  dmPropertyInfo();
  void setName(std::string const& name);
  void setType(dmValueType t);
  void setIsOptional(bool b);
  void setIsWritable(bool b);
  void setFullName(std::string const& name);

private:
  std::string m_name;
  dmValueType m_type;
  bool        m_optional;
  bool        m_writable;
  std::string m_full_name;
};


#endif
