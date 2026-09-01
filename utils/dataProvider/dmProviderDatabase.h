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
#ifndef __DM_PROVIDER_DATABASE_H__
#define __DM_PROVIDER_DATABASE_H__

#include "dmProviderInfo.h"
#include "dmQuery.h"
#include "dmProviderOperation.h"
#include "dmPropertyInfo.h"

#include <map>
#include <string>
#include <vector>

class dmProviderDatabase
{
public:
  dmProviderDatabase(std::string const& dir);

  dmQuery* createQuery();
  dmQuery* createQuery(dmProviderOperation op, char const* s);
  std::shared_ptr<dmProviderInfo> getProviderByProviderName(std::string const& s) const;
  std::shared_ptr<dmProviderInfo> getProviderByModelName(std::string const& s) const;
  std::shared_ptr<dmProviderInfo> getProviderByPropertyName(std::string const& s, bool* isListItem=nullptr) const;
  std::shared_ptr<dmProviderInfo> getProviderByObjectName(std::string const& s, bool* isListItem=nullptr) const;
private:
  void loadFromDir(std::string const& dir);
  void loadFile(std::string const& dir, char const* fname);
  std::shared_ptr<dmProviderInfo> makeProviderInfo(char const* json);
  void buildProviderTree();
private:
  std::string m_modelDirectory;
};

#endif
