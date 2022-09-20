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
#include "dmClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <rtError.h>
#include "dmProviderDatabase.h"
#include "dmQuery.h"
#include "dmUtility.h"

void getResultValue(std::vector<dmQueryResult::Param> const& results, std::string const& name, dmValue& value)
{
    rtLog_Debug("getResultValue %s", name.c_str());
    for (auto const& param: results)
    {
      if(dmUtility::trimPropertyName(param.Info.fullName()) == name)
      {
        rtLog_Debug("getResultValue found type=%d string=%s", param.Value.type(), param.Value.toString().c_str());
        value = param.Value;
      }
    }
}

class dmClientImpl : public dmClient
{
public:
  dmClientImpl(std::string const& datamodelDir, rtLogLevel logLevel)
  {
    rtLog_SetLevel(logLevel);
    rtLog_Debug("create data base dmClient");
    m_db = new dmProviderDatabase(datamodelDir);
  }

  ~dmClientImpl()
  {
    delete m_db;
  }

  bool runGet(std::string const& parameter, bool recursive, dmClientNotifier* notifier)
  {
    bool success = true;
    bool isWildcard = false;
    bool isList = false;
    bool isListItem;
    std::string objectName(parameter);
    std::shared_ptr<dmProviderInfo> providerInfo;

    if (dmUtility::isWildcard(objectName.c_str()))
    {
      rtLog_Debug("runQuery isWildcard %s", objectName.c_str());
      isWildcard = true;
      //objectName = dmUtility::trimWildcard(objectName);

      providerInfo = m_db->getProviderByPropertyName(parameter,&isListItem);
      if(!providerInfo)
      {
        rtLog_Warn("runQuery no providerinfo");
        return false;
      }
      rtLog_Info("runQuery providerInfo info=%s objectName=%s list=%d isListItem=%d", providerInfo->objectName().c_str(), objectName.c_str(), (int)providerInfo->isList(), (int)isListItem);
      if(!isListItem && providerInfo->isList())
        isList = true;
    }

    if(isWildcard && isList)
    {
      std::string list_name = dmUtility::trimWildcard(parameter);
      std::string parent_name = dmUtility::trimProperty(list_name);
      dmValue value(0);
      rtLog_Debug("dmcli_get list=%s parent=%s", list_name.c_str(), parent_name.c_str());

      std::string num_entries_param = list_name + "NumberOfEntries";
      if(!runOneQuery(dmProviderOperation_Get, num_entries_param, nullptr, &value))
      {
        notifier->onError(RT_FAIL, "dmcli_get list number query failed");
        return false;
      }
      int num_entries = atoi(value.toString().c_str());
      rtLog_Debug("dmcli_get list num_entries=%d", num_entries);

      std::string alias_entries_param = list_name + "IdsOfEntries";
      if(!runOneQuery(dmProviderOperation_Get, alias_entries_param , nullptr, &value))
      {

        notifier->onError(RT_FAIL, "dmcli_get list entry query failed");
        return false;
      }

      std::string entries = value.toString().c_str();
      rtLog_Debug("dmcli_get list entries=%s", entries.c_str());

      std::vector<std::string> out;
      dmUtility::splitString(entries, ',', out);
      rtLog_Debug("dmcli_get split=%d", (int)out.size());
      if((int)out.size() != num_entries)
      {
        notifier->onError(RT_FAIL, "dmcli_get list failed: size mismatch");
        return false;
      }

      success = true;
      for(int i = 0; i < num_entries; ++i)
      {
        std::stringstream list_item_param;
        list_item_param << list_name << "." << out[i] << ".";
        std::vector<dmQueryResult::Param> res;
        if(!runOneQuery(dmProviderOperation_Get, list_item_param.str(), notifier))
          success = false;//if only a list item query fails, we keep going but still at end, return false

        if(recursive)
        {
          std::shared_ptr<dmProviderInfo> objectInfo = m_db->getProviderByPropertyName(list_item_param.str()); 
          rtLog_Debug("dmcli_get recurse parameter=%s objectName=%s", list_item_param.str().c_str(), objectInfo->objectName().c_str());
          for(size_t i = 0; i < objectInfo->getChildren().size(); ++i)
          {
            std::string childParameter = list_item_param.str().c_str() + dmUtility::trimPropertyName(objectInfo->getChildren()[i].lock()->objectName()) + ".";
            rtLog_Debug("dmcli_get childObjectName %s", childParameter.c_str());

            success = runQuery(dmProviderOperation_Get, childParameter, recursive, notifier);
          }
        }
      }
    }
    else
    {
      success = runOneQuery(dmProviderOperation_Get, parameter, notifier);

      if(recursive)
      {
        std::shared_ptr<dmProviderInfo> objectInfo = m_db->getProviderByPropertyName(parameter); 
        rtLog_Info("dmcli_get recurse %s", objectInfo->objectName().c_str());
        for(size_t i = 0; i < objectInfo->getChildren().size(); ++i)
        {
          std::string childParameter = dmUtility::trimProperty(parameter) + "." + dmUtility::trimPropertyName(objectInfo->getChildren()[i].lock()->objectName()) + ".";
          rtLog_Warn("dmcli_get childObjectName %s", childParameter.c_str());

          success = runQuery(dmProviderOperation_Get, childParameter, recursive, notifier);
        }
      }
    }
    return success;
  }

  bool runSet(std::string const& parameter, dmClientNotifier* notifier)
  {
    return runOneQuery(dmProviderOperation_Set, parameter, notifier);
  }

  bool runQuery(dmProviderOperation operation, std::string const& parameter, bool recursive, dmClientNotifier* notifier)
  {
    rtLog_Debug("runQuery %s", parameter.c_str());
    if(operation == dmProviderOperation_Get)
      return runGet(parameter,recursive,notifier);
    else
      return runSet(parameter,notifier);
  }

private:

  bool runOneQuery(dmProviderOperation op, std::string const& parameter, dmClientNotifier* notifier, dmValue* value = nullptr)
  {
    rtLog_Debug("runOneQuery %s", parameter.c_str());
    std::unique_ptr<dmQuery> query(m_db->createQuery(op, parameter.c_str()));

    if (!query)
    {
      std::string msg = "failed to create query";
      rtLog_Warn("%s", msg.c_str());
      if(notifier)
        notifier->onError(RT_FAIL, msg);
      return false;
    }

    if (!query->exec())
    {
      std::string msg = "failed to exec query";
      rtLog_Warn("%s", msg.c_str());
      if(notifier)
        notifier->onError(RT_FAIL, msg);
      return false;
    }
    dmQueryResult const& results = query->results();
    if (results.status() == RT_OK)
    {
      if(notifier)
        notifier->onResult(results);
      if(value)
      {
        *value = 0;
        getResultValue(results.values(), dmUtility::trimPropertyName(parameter), *value);
      }
      return true;
    }
    else
    {
      rtLog_Warn("Error: status=%d msg=%s", results.status(), results.statusMsg().c_str());
      if(notifier)
        notifier->onError(results.status(), results.statusMsg());
      return false;
    }
  }

  dmProviderDatabase* m_db;
};

dmClient* dmClient::create(std::string const& datamodelDir, rtLogLevel logLevel)
{
  return new dmClientImpl(datamodelDir, logLevel);
}

void dmClient::destroy(dmClient* client)
{
  if(client)
    delete client;
}


