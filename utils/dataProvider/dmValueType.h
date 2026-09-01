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
#ifndef __DM_VALUE_TYPE_H__
#define __DM_VALUE_TYPE_H__

enum dmValueType
{
  dmValueType_Int8,
  dmValueType_Int16,
  dmValueType_Int32,
  dmValueType_Int64,
  dmValueType_UInt8,
  dmValueType_UInt16,
  dmValueType_UInt32,
  dmValueType_UInt64,
  dmValueType_String,
  dmValueType_Single,
  dmValueType_Double,
  dmValueType_Boolean,
  dmValueType_Unknown
};

dmValueType
dmValueType_fromString(char const* s);

char const*
dmValueType_toString(dmValueType t);

#endif
