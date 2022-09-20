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
#include "dmProviderHost.h"
#include "dmProvider.h"
#include "dmProviderOperation.h"

#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <unistd.h>

#include <rtConnection.h>
#include <rtError.h>
#include <rtLog.h>

#include "dmUtility.h"

namespace
{

  dmNamedValue
  makeNamedValue(dmPropertyInfo const& propInfo, char const* valueAsString)
  {
    // TODO conver to propert type from string
    return dmNamedValue(propInfo, dmValue(valueAsString));
  }
}

class dmProviderHostImpl : public dmProviderHost
{
public:
  dmProviderHostImpl()
  {

  }

  virtual ~dmProviderHostImpl()
  {
    if (m_con)
    {
      rtLog_Info("closing rtMessage connection");
      rtConnection_Destroy(m_con);
      m_con = nullptr;
    }
  }

private:
  virtual bool providerRegistered(std::string const& name)
  {
    std::stringstream topic;
    topic << "RDK.MODEL.";
    topic << name;
    std::string s = topic.str();

    rtLog_Debug("register provider with topic:%s", s.c_str());
    rtError e = rtConnection_AddListener(m_con, s.c_str(), &dmProviderHostImpl::requestHandler, this);
    if (e != RT_OK)
    {
      rtLog_Warn("failed to register provider listener. %s", rtStrError(e));
      return false;
    }

    return true;
  }

  void start()
  {
    rtConnection_Create(&m_con, "USE_UNIQUE_NAME_HERE", "tcp://127.0.0.1:10001");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_thread.reset(new std::thread(&dmProviderHostImpl::run, this));
  }

  void stop()
  {
    rtConnection_Destroy(m_con);
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_thread)
    {
      // TODO
    }
  }

  void run()
  {
    pause();
  }

  static void requestHandler(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n,
    void* closure)
  {
    if (!rtMessageHeader_IsRequest(hdr))
    {
      rtLog_Error("got message that wasn't request in datamodel callback");
      return;
    }

    rtMessage req;
    rtError e = rtMessage_FromBytes(&req, buff, n);
    rtLog_Debug("req: %s", buff);

    if (e != RT_OK)
    {
      rtLog_Warn("failed to decode datamodel request");
      // TODO: return error
    }

    dmProviderHostImpl* host = reinterpret_cast<dmProviderHostImpl *>(closure);
    if (!host)
    {
      rtLog_Error("dmProviderHost is null");
      // TODO: return error
    }

    dmQueryResult result;
    dmProviderOperation op = host->decodeOperation(req);

    if (op == dmProviderOperation_Get)
    {
      std::string providerName;
      std::vector<dmPropertyInfo> params;
      host->decodeGetRequest(req, providerName, params);
      host->doGet(providerName, params, result);
    }
    else if (op == dmProviderOperation_Set)
    {
      std::string providerName;
      std::vector<dmNamedValue> params;
      host->decodeSetRequest(req, providerName, params);
      host->doSet(providerName, params, result);
    }

    rtMessage_Release(req);

    rtMessage res;
    rtMessage_Create(&res);
    host->encodeResult(res, result);
    rtConnection_SendResponse(m_con, hdr, res, 1000);
    rtMessage_Release(res);
  }

  dmProviderOperation decodeOperation(rtMessage req)
  {
    char const* operation = nullptr;
    rtMessage_GetString(req, "method", &operation);
    if ((strcmp(operation, "set") == 0))
      return dmProviderOperation_Set;
    else
      return dmProviderOperation_Get;
  }

  void decodeGetRequest(rtMessage req, std::string& name, std::vector<dmPropertyInfo>& params)
  {
    char const* providerName = nullptr;
    dmPropertyInfo propertyInfo;

    rtMessage_GetString(req, "provider", &providerName);
    if (providerName)
      name = providerName;

    rtMessage item;
    rtMessage_GetMessage(req, "params", &item);

    char const* propertyName = nullptr;
    rtMessage_GetString(item, "name", &propertyName);

    bool isWildcard = dmUtility::isWildcard(propertyName);
    std::string objectName = isWildcard ? dmUtility::trimWildcard(propertyName) : dmUtility::trimProperty(propertyName);

    rtLog_Debug("decodeGetRequest property=%s\n", (propertyName != nullptr ? propertyName : ""));

    if(name != objectName)
      rtLog_Warn("provider/property name missmatch %s, %s", name.c_str(), objectName.c_str());

    std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(objectName);
    if (objectInfo)
    {
      rtLog_Debug("decodeGetRequest object found %s", providerName);

      if (isWildcard)
        params = objectInfo->properties();
      else
      {
        dmPropertyInfo info = objectInfo->getPropertyInfo(propertyName);
        //if property not found then set its fullName so that it gets passed up in the error
        if(info.fullName().empty())
          info.setFullName(propertyName);
        params.push_back(info);
      }
    }
    else
    {
      rtLog_Debug("decodeGetRequest object not found %s", propertyName);
    }

    rtMessage_Release(item);
  }

  void decodeSetRequest(rtMessage req, std::string& name, std::vector<dmNamedValue>& params)
  {
    char const* providerName = nullptr;

    rtMessage_GetString(req, "provider", &providerName);
    if (providerName)
      name = providerName;

    rtMessage item;
    rtMessage_GetMessage(req, "params", &item);

    char const* targetName = nullptr;
    rtMessage_GetString(item, "name", &targetName);

    char const* value = nullptr;
    rtMessage_GetString(item, "value", &value);

    if(!value)
    {
      rtLog_Debug("decodeSetRequest value is null");
      return;
    }

    size_t len = strlen(value);

    if(len == 0)
    {
      rtLog_Debug("decodeSetRequest value is empty");
      return;
    }

    rtLog_Debug("decoderSetRequest name=%s value=%s\n", (targetName != nullptr ? targetName : ""), (value != nullptr ? value : ""));

    std::vector< std::pair<std::string,std::string> > nameVals;
    std::shared_ptr<dmProviderInfo> objectInfo;

    //determine if value is a single value or multi-set value, like object={prop1=val1,prop2=val2...}
    if(dmUtility::parseMultisetValue(value, nameVals))
    {
      objectInfo = db->getProviderByObjectName(targetName);//for multi-set the targetName is the object name
    }
    else
    {
      objectInfo = db->getProviderByPropertyName(targetName);
      nameVals.push_back(std::make_pair(dmUtility::trimPropertyName(targetName), value));
    }

    if(objectInfo)
    {
      rtLog_Debug("decodeSetRequest object found %s", targetName);
      std::vector<dmPropertyInfo> props = objectInfo->properties();

      for (auto const& nameVal : nameVals)
      {
        std::string propertyName = nameVal.first;
        auto itr = std::find_if(
          props.begin(),
          props.end(),
          [propertyName](dmPropertyInfo const& info) { 
            rtLog_Debug("decodeSetRequest find_if %s compare to %s = %d\n", info.name().c_str(), propertyName.c_str(), (int)(info.name() == propertyName));
            return info.name() == propertyName; 
          });

        if (itr != props.end())
        {
          params.push_back(makeNamedValue(*itr, nameVal.second.c_str()));
        }
        else
        {
          //if property not found then create one with a fullName so that it gets passed up in the error
          dmPropertyInfo info;
          info.setFullName(targetName);
          info.setIsWritable(true);
          params.push_back(makeNamedValue(info, nameVal.second.c_str()));
        }
      }
    }
    else
    {
      rtLog_Debug("decodeSetRequest object not found %s", targetName);
    }
    
    rtMessage_Release(item);
  }

  void encodeResult(rtMessage& res, dmQueryResult const& result)
  {
    rtMessage msg;
    rtMessage_Create(&msg);
    int statusCode = result.status();
    std::string statusMessage = result.statusMsg();

    for (dmQueryResult::Param const& param : result.values())
    {
      rtMessage p;
      rtMessage_Create(&p);
      rtMessage_SetString(p, "name", param.Info.fullName().c_str());
      rtMessage_SetString(p, "value", param.Value.toString().c_str());
      rtMessage_SetInt32(p, "status", param.StatusCode);
      rtMessage_SetString(p, "status_msg", param.StatusMessage.c_str());
      rtMessage_AddMessage(msg, "params", p);
      rtMessage_Release(p);
      // I don't like how we do this. This is trying to set the overall status of the
      // request to the first error it sees. For example, if we query two params, and
      // one is ok and the other is an error, the statuscode is set to the second error
      // code. If there are two errors it'll set it to the first. We should introduce an
      // top-level error code in the response message. possibly something that indicates
      // that there's partial failure.
      //if (param.StatusCode != 0 && statusCode == 0)
        //statusCode = param.StatusCode;

      //if (statusMessage.empty() && !param.StatusMessage.empty())
        //statusMessage = param.StatusMessage;

     // rtMessage_SetString(msg, "name", param.Info.fullName().c_str());
     // rtMessage_SetString(msg, "value", param.Value.toString().c_str());
    }

    rtMessage_SetInt32(msg, "status", statusCode);
    rtMessage_SetString(msg, "status_msg", statusMessage.c_str());
    rtMessage_AddMessage(res, "result", msg);
    rtMessage_Release(msg);
  }

private:
  std::unique_ptr<std::thread> m_thread;
  std::mutex m_mutex;
  static rtConnection m_con;
};

rtConnection dmProviderHostImpl::m_con = nullptr;

dmProviderHost*
dmProviderHost::create()
{
  return new dmProviderHostImpl();
}

bool
dmProviderHost::registerProvider(char const* object, std::unique_ptr<dmProvider> provider)
{
  bool b = false;
  std::shared_ptr<dmProviderInfo> objectInfo = db->getProviderByObjectName(std::string(object));

  if (objectInfo)
  {
    rtLog_Debug("registerProvider fullName=%s instanceName=%s", objectInfo->objectName().c_str(), object);
    b = providerRegistered(object);
    if (b)
    {
      m_providers.insert(std::make_pair(object, std::move(provider)));
      if(objectInfo->isList())
      {
        std::string listName = dmUtility::trimProperty(object);
        rtLog_Debug("adding list item %s to list %s", object, listName.c_str());
        m_lists[listName].push_back(object);
      }
    }
  }
  else
  {
    rtLog_Error("failed to find provider info in database for object:%s", object);
  }
  return b;
}

void
dmProviderHost::doGet(std::string const& providerName, std::vector<dmPropertyInfo> const& params, dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    rtLog_Debug("dmProviderHost::doGet %s found", providerName.c_str());
    itr->second->doGet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doGet %s not found", providerName.c_str());
  }
}

void
dmProviderHost::doSet(std::string const& providerName, std::vector<dmNamedValue> const& params, dmQueryResult& result)
{
  auto itr = m_providers.find(providerName);
  if (itr != m_providers.end())
  {
    rtLog_Debug("dmProviderHost::doSet %s found", providerName.c_str());
    itr->second->doSet(params, result);
  }
  else
  {
    rtLog_Debug("dmProviderHost::doSet %s not found", providerName.c_str());
  }
}
