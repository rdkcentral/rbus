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
#include "dmValue.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace
{
  struct dmValueTypeName
  {
    std::string asstring;
    dmValueType asenum;
  };

  std::vector<dmValueTypeName> dmValueNames =
  {
    { "int8", dmValueType_Int8 },
    { "int16", dmValueType_Int16 },
    { "int32", dmValueType_Int32 },
    { "int64", dmValueType_Int64 },
    { "uint8", dmValueType_UInt8 },
    { "uint16", dmValueType_UInt16 },
    { "uint32", dmValueType_UInt32 },
    { "uint64", dmValueType_UInt64 },
    { "single", dmValueType_Single },
    { "string", dmValueType_String },
    { "double", dmValueType_Double },
    { "bool", dmValueType_Boolean }
  };
}

dmValue::dmValue(bool b)
  : m_type(dmValueType_Boolean)
  , m_value(b) { }

dmValue::dmValue(std::string const& s)
  : m_type(dmValueType_String)
  , m_string(s) { }

dmValue::dmValue(char const* s)
  : m_type(dmValueType_String)
  , m_string(s) { }

dmValue::dmValue(int8_t n)
  : m_type(dmValueType_Int8)
  , m_value(n) { }

dmValue::dmValue(int16_t n)
  : m_type(dmValueType_Int16)
  , m_value(n) { }

dmValue::dmValue(int32_t n)
  : m_type(dmValueType_Int32)
  , m_value(n) { }

dmValue::dmValue(int64_t n)
  : m_type(dmValueType_Int64)
  , m_value(n) { }

dmValue::dmValue(uint8_t n)
  : m_type(dmValueType_UInt8)
  , m_value(n) { }

dmValue::dmValue(uint16_t n)
  : m_type(dmValueType_UInt16)
  , m_value(n) { }

dmValue::dmValue(uint32_t n)
  : m_type(dmValueType_UInt32)
  , m_value(n) { }

dmValue::dmValue(uint64_t n)
  : m_type(dmValueType_UInt64)
  , m_value(n) { }

dmValue::dmValue(float f)
  : m_type(dmValueType_Single)
  , m_value(f) { }

dmValue::dmValue(double d)
  : m_type(dmValueType_Double)
  , m_value(d) { }

std::string
dmValue::toString() const
{
  std::stringstream buff;
  switch (m_type)
  {
    case dmValueType_Boolean:
      buff << (m_value.booleanValue ? "true" : "false");
      break;
    case dmValueType_Unknown:
      buff << "(null)";
      break;
    case dmValueType_String:
      buff.str(m_string);
      break;
    case dmValueType_Int8:
      buff << m_value.int8Value;
      break;
    case dmValueType_Int16:
      buff << m_value.int16Value;
      break;
    case dmValueType_Int32:
      buff << m_value.int32Value;
      break;
    case dmValueType_Int64:
      buff << m_value.int64Value;
      break;
    case dmValueType_UInt8:
      buff << m_value.uint8Value;
      break;
    case dmValueType_UInt16:
      buff << m_value.uint16Value;
      break;
    case dmValueType_UInt32:
      buff << m_value.uint32Value;
      break;
    case dmValueType_UInt64:
      buff << m_value.uint64Value;
      break;
    case dmValueType_Single:
      buff << m_value.singleValue;
      break;
    case dmValueType_Double:
      buff << m_value.doubleValue;
      break;
  }
  return buff.str();
}

dmValueType
dmValueType_fromString(char const* s)
{
  auto itr = std::find_if(dmValueNames.begin(), dmValueNames.end(),
    [s](dmValueTypeName const& entry) { return entry.asstring.compare(s) == 0; });

  if (itr == dmValueNames.end())
    return dmValueType_Unknown;

  return itr->asenum;
}

char const*
dmValueType_toString(dmValueType t)
{
  auto itr = std::find_if(dmValueNames.begin(), dmValueNames.end(),
    [t](dmValueTypeName const& entry) { return entry.asenum == t; });

  if (itr == dmValueNames.end())
    return "unknown";

  return itr->asstring.c_str();
}
