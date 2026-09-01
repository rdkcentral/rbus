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
#include "dmProviderDatabase.h"
#include "dmQuery.h"

#include <iostream>
#include <dirent.h>
#include <cstddef>
#include <string>
#include <cstring>
#include <map>
#include <iterator>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <list>

#include "rtConnection.h"
#include "rtMessage.h"
#include "rtError.h"
#include "rtLog.h"

#include <cjson/cJSON.h>

#include "dmUtility.h"
#include "dmProviderInfo.h"

namespace
{
  using dmDatabase = std::map< std::string, std::shared_ptr<dmProviderInfo> >;

  dmDatabase model_db;
  dmDatabase model_roots;
  /*
  bool matches_object(char const* query, char const* obj)
  {
    char const* p = strrchr(query, '.');
    int n = static_cast<int>((intptr_t) p - (intptr_t) query);
    return strncmp(query, obj, n) == 0;
  }
  */
}

class dmQueryImpl : public dmQuery
{
public:
  dmQueryImpl(dmProviderDatabase* DB)
  {
    rtConnection_Create(&m_con, "DMCLI", "tcp://127.0.0.1:10001");

    db = DB;
  }

  ~dmQueryImpl()
  {
    rtConnection_Destroy(m_con);
  }

  void makeRequest(std::string& topic, std::string& queryString)
  {
    rtMessage req;
    rtMessage_Create(&req);
    rtMessage_SetString(req, "method", m_operation.c_str());
    rtMessage_SetString(req, "provider", m_instanceName.c_str());
    rtMessage item;
    rtMessage_Create(&item);
    rtMessage_SetString(item, "name", queryString.c_str());
    if (strcmp(m_operation.c_str(), "set") == 0)
      rtMessage_SetString(item, "value", m_value.c_str());
    rtMessage_SetMessage(req, "params", item);
    rtMessage res;

    rtError err = rtConnection_SendRequest(m_con, req, topic.c_str(), &res, 5000);
#if 0
    if ((err == RT_OK) && rtLog_IsLevelEnabled(RT_LOG_DEBUG))
    {
      char* p = nullptr;
      uint32_t n = 0;
      rtMessage_ToString(res, &p, &n);
      if (p)
      {
        rtLog_Debug("res:%s", p);
        free(p);
      }
    }
#endif
    if (err == RT_OK)
    {
      int32_t len;
      rtMessage_GetArrayLength(res, "result", &len);

      for (int32_t i = 0; i < len; i++)
      {
        rtMessage item;
        rtMessage_GetMessageItem(res, "result", i, &item);

#if 0 //to log the whole message
        char* pBuff = nullptr;
        uint32_t nBuff;
        rtMessage_ToString(item, &pBuff, &nBuff);
        if(pBuff)
        {
          rtLog_Warn("MSG=%s", pBuff);
          free(pBuff);
        }
#endif
        int status = 0;
        char const* status_msg = nullptr;
        if (rtMessage_GetInt32(item, "status", &status) != RT_OK)
          rtLog_Error("failed to get 'status' from response");
        if (rtMessage_GetString(item, "status_msg", &status_msg) != RT_OK)
          rtLog_Debug("no status message in response");
        if(status != 0)
          m_results.setStatus(status);
        if(status_msg != nullptr)
          m_results.setStatusMsg(status_msg);

        int32_t paramslen;
        rtMessage_GetArrayLength(item, "params", &paramslen);
        for (int32_t j = 0; j < paramslen; ++j)
        {
          int status = 0;
          char const* param = nullptr;
          char const* value = nullptr;
          char const* status_msg = nullptr;

          rtMessage p;
          rtMessage_GetMessageItem(item, "params", j, &p);

          if (rtMessage_GetString(p, "name", &param) != RT_OK)
            rtLog_Debug("failed to get 'name' from paramter");
          if (rtMessage_GetString(p, "value", &value) != RT_OK)
            rtLog_Debug("failed to get 'value' from parameter");
          if (rtMessage_GetInt32(p, "status", &status) != RT_OK)
            rtLog_Error("failed to get 'status' from response");
          if (rtMessage_GetString(p, "status_msg", &status_msg) != RT_OK)
            rtLog_Debug("no status message in response");

          if (param != nullptr && value != nullptr)
          {
            dmPropertyInfo info = m_providerInfo->getPropertyInfo(param);
            if(info.fullName().empty())
            {
              info.setName(dmUtility::trimPropertyName(param));
              info.setFullName(param);
            }
            m_results.addValue(info, dmValue(value), status, status_msg != nullptr ? status_msg : "");
          }
        }
      }

      m_results.setObjectName(m_instanceName);

      rtMessage_Release(res);
      rtMessage_Release(req);
    }
  }

  virtual bool exec()
  {
    if (!m_providerInfo)
    {
      rtLog_Warn("Trying to execute a query without a provider");
      return false;
    }

    std::string topic("RDK.MODEL.");
    topic += m_instanceName;

    rtLog_Debug("sending dm query : %s on topic :%s", m_query.c_str(), topic.c_str());

    makeRequest(topic, m_query);

    reset();
    return true;
  }

  virtual void reset()
  {
    m_operation.clear();
    m_query.clear();
    m_value.clear();
    m_instanceName.clear();
    m_providerInfo.reset();
  }

  virtual bool setQueryString(dmProviderOperation op, char const* s)
  {
    switch (op)
    {
      case dmProviderOperation_Get:
      {
        m_operation = "get";
        m_query = s;
        if (dmUtility::isWildcard(m_query.c_str()))
          m_instanceName = dmUtility::trimWildcard(m_query);
        else
          m_instanceName = dmUtility::trimProperty(m_query);
      }
      break;
      case dmProviderOperation_Set:
      {
        m_operation = "set";
        std::string data(s);
        if (data.find("=") != std::string::npos)
        {
          std::size_t position = data.find("=");
          m_query = data.substr(0, position);
          m_value = data.substr(position+1);
          m_instanceName = dmUtility::trimSetProperty(data);

          /*
          For data like "Parent.ObjectName.PropName=xx.yy.zz", when trimSetProperty is called once for data in the above line, the m_instanceName value will be "Parent.ObjectName.PropName=xx.yy"
          The while block below, trims m_instanceName upto "Parent.ObjectName"
          */
          std::string tmpData(m_instanceName);
          int hasEq = (tmpData.find("=") != std::string::npos);

          while (hasEq)
          {
            m_instanceName = dmUtility::trimSetProperty(tmpData);

            if (m_instanceName.empty())
              return false;

            hasEq = (m_instanceName.find("=") != std::string::npos);
            tmpData = m_instanceName;
          }
        }
        else
        {
          rtLog_Error("SET operation without value");
          return false;
        }
        break;
      }
    }
    return true;
  }

  virtual dmQueryResult const& results()
  {
    return m_results;
  }

  void setProviderInfo(std::shared_ptr<dmProviderInfo> const& providerInfo)
  {
    m_providerInfo = providerInfo;
  }

private:
  rtConnection m_con;
  dmQueryResult m_results;
  std::string m_query;
  std::string m_value;
  std::string m_operation;
  std::string m_instanceName;
  dmProviderDatabase *db;
  std::shared_ptr<dmProviderInfo> m_providerInfo;

  // TODO: int m_count;
  // should use static counter or other way to inject json-rpc request/id
};

dmProviderDatabase::dmProviderDatabase(std::string const& dir)
  : m_modelDirectory(dir)
{
  const char* modelDirOverride = getenv("DM_DATA_MODEL_DIRECTORY");
  if(modelDirOverride)
  {
    m_modelDirectory = modelDirOverride;
  }
  
  loadFromDir(m_modelDirectory);

  buildProviderTree();
}

void
dmProviderDatabase::loadFromDir(std::string const& dirname)
{
  DIR* dir;
  struct dirent *ent;

  rtLog_Debug("loading database from directory:%s", dirname.c_str());
  if ((dir = opendir(dirname.c_str())) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      if (strcmp(ent->d_name,".") != 0 && strcmp(ent->d_name,"..") != 0)
        loadFile(dirname.c_str(), ent->d_name);
    }
    closedir(dir);
  } 
  else
  {
    rtLog_Warn("failed to open directory:%s. %s", dirname.c_str(),
      strerror(errno));
  }
}

void
dmProviderDatabase::loadFile(std::string const& dir, char const* fname)
{
  std::string path = dir;
  path += "/";
  path += fname;

  rtLog_Debug("load file:%s", path.c_str());

  std::ifstream file;
  file.open(path.c_str(), std::ifstream::in);
  if (!file.is_open())
  {
    rtLog_Warn("failed to open file. %s. %s", fname, strerror(errno));
    return;
  }

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);

  std::vector<char> buff(length + 1, '\0');
  file.read(buff.data(), length);

  // cJSON* name = cJSON_GetObjectItem(json, "name");
  // modelInfoDB.insert(std::make_pair(name->valuestring, json));
  std::shared_ptr<dmProviderInfo> providerInfo = makeProviderInfo(&buff[0]);
  if (providerInfo)
  {
    rtLog_Debug("model_db insert:%s", providerInfo->objectName().c_str());
    model_db.insert(std::make_pair(providerInfo->objectName(), providerInfo));
  }
  else
  {
    rtLog_Error("Failed to parse json from:%s. %s", path.c_str(), cJSON_GetErrorPtr());
  }
}

void printTree(std::shared_ptr<dmProviderInfo> p, int curdep)
{
  if(!p)
    return;
  char buffer[200];
  sprintf(buffer, "%*.s%s", curdep, "  ", p->objectName().c_str());
  rtLog_Debug("tree(%d): %s children=%d", curdep, buffer, (int)p->getChildren().size());
  curdep++;
  for(size_t i = 0; i < p->getChildren().size(); ++i)
    printTree(p->getChildren()[i].lock(),curdep);  
}

void
dmProviderDatabase::buildProviderTree()
{
  for (auto itr : model_db)
  {
    rtLog_Debug("buildProviderTree %s isList=%d", itr.second->objectName().c_str(), (int)itr.second->isList());
    std::string parentName = dmUtility::trimProperty(itr.second->objectName());

    bool found = false;
    for (auto itr2 : model_db)
    {
      if(itr2.second->objectName() == parentName)
      {
        itr2.second->m_children.push_back(itr.second);
        itr.second->m_parent = itr2.second;
        found = true;
        break;
      }
    }
  
    if(!found)
      model_roots.insert(std::make_pair(itr.second->objectName(), itr.second));
  }

  for (auto itr : model_roots)
  {
    printTree(itr.second, 0);
  }
}

#if 0
int
dmProviderDatabase::isWritable(char const* param, char const* provider)
{
  for (auto iter : modelInfoDB)
  {
    cJSON* objProv = cJSON_GetObjectItem(iter.second, "provider");
    std::string objectProvider = objProv->valuestring;
    if (strcmp(objectProvider.c_str(), provider) == 0)
    {
      cJSON *properties = cJSON_GetObjectItem(iter.second, "properties");
      for (int i = 0 ; i < cJSON_GetArraySize(properties); i++)
      {
        cJSON *subitem = cJSON_GetArrayItem(properties, i);
        cJSON* obj = cJSON_GetObjectItem(subitem, "name");
        std::string objName = obj->valuestring;
        if (strcmp(objName.c_str(), param) == 0)
        {
          cJSON* objWrite = cJSON_GetObjectItem(subitem, "writable");
          return objWrite->valueint;
        }
      }
    }
  }
  return 1;
}
#endif

std::shared_ptr<dmProviderInfo>
dmProviderDatabase::getProviderByObjectName(std::string const& p, bool* isListItem) const
{
  using namespace std;
  if(p.length() == 0)
    return nullptr;
  std::string p2 = p;
  size_t n1 = 0;
  size_t n2 = p.find('.');
  string fullName;
  string instanceName;
  bool isList = false;
  std::shared_ptr<dmProviderInfo> provider;
  while(true)
  {
    string token = p.substr(n1, n2-n1);
    if(!isList)
    {
      if(isListItem)
        *isListItem = false;
      if(!fullName.empty())
        fullName += ".";
      fullName += token;

      auto itr = model_db.find(fullName);    
      if(itr != model_db.end())
      {
        provider = itr->second;
       if(provider && provider->isList())
          isList = true;
      }
      else if(provider)
      {
        rtLog_Debug("Failed to find %s", fullName.c_str());
        return nullptr;
      }
    }
    else
    {
      isList = false;
      if(isListItem)
        *isListItem = true;
    }
    
    if(!instanceName.empty())
      instanceName += ".";
    instanceName += token;
    if(n2 == string::npos || n2 > p.length()-1)
      break;
    n1 = n2+1;
    n2 = p.find('.', n1);
  }
  rtLog_Debug("getProviderByPropertyName input=%s output=%s trimmed=%s fullName=%s instanceName=%s", 
    p.c_str(), provider != nullptr ? provider->objectName().c_str() : "null", p2.c_str(),  fullName.c_str(), instanceName.c_str());
  return provider;
}

std::shared_ptr<dmProviderInfo>
dmProviderDatabase::getProviderByPropertyName(std::string const& s, bool* isListItem) const
{
  if(s.length() == 0)
    return nullptr;
  if (dmUtility::isWildcard(s.c_str()))
    return getProviderByObjectName(dmUtility::trimWildcard(s), isListItem);
  else
    return getProviderByObjectName(dmUtility::trimProperty(s), isListItem);
}

std::shared_ptr<dmProviderInfo>
dmProviderDatabase::getProviderByModelName(std::string const& s) const
{
  //rtLog_Info("get provider by objectname:%s", s.c_str());

  std::string objectName;
  if (dmUtility::isWildcard(s.c_str()))
    objectName = dmUtility::trimWildcard(s);
  else
    objectName = s;

  auto itr = model_db.find(objectName);
  //if (itr == model_db.end())
  //  rtLog_Debug("failed to find %s in model database", objectName.c_str());

  return (itr != model_db.end())
    ? itr->second
    : std::shared_ptr<dmProviderInfo>();
}

std::shared_ptr<dmProviderInfo>
dmProviderDatabase::getProviderByProviderName(std::string const& s) const
{
  for (auto itr : model_db)
  {
    if (itr.second->providerName() == s)
      return itr.second;
  }
  return std::shared_ptr<dmProviderInfo>();
}

#if 0
char const*
dmProviderDatabase::getProviderFromObject(char const* object) const
{
  for (auto itr : modelInfoDB)
  {
    if (strcmp(itr.first.c_str(), object) == 0)
    {
      cJSON* obj = cJSON_GetObjectItem(itr.second, "provider");
      return obj->valuestring;
    }
  }
  return nullptr;
}
#endif

#if 0
std::vector<char const*>
dmProviderDatabase::getParameters(char const* provider) const
{
  std::vector<char const*> parameters;
  for (auto itr : modelInfoDB)
  {
    cJSON* objProv = cJSON_GetObjectItem(itr.second, "provider");
    std::string objectProvider = objProv->valuestring;
    if (strcmp(objectProvider.c_str(), provider) == 0)
    {
      cJSON *properties = cJSON_GetObjectItem(itr.second, "properties");
      for (int i = 0 ; i < cJSON_GetArraySize(properties); i++)
      {
        cJSON *subitem = cJSON_GetArrayItem(properties, i);
        cJSON* obj = cJSON_GetObjectItem(subitem, "name");
        parameters.push_back(obj->valuestring);
      }
    }
  }
  return parameters;
}
#endif

dmQuery*
dmProviderDatabase::createQuery()
{
  return new dmQueryImpl(this);
}

dmQuery* 
dmProviderDatabase::createQuery(dmProviderOperation op, char const* queryString)
{
  if (!queryString)
    return nullptr;

  std::shared_ptr<dmProviderInfo> providerInfo;

  if(op == dmProviderOperation_Get)
    providerInfo = getProviderByPropertyName(queryString);
  else{

    /*
    For queryString like "Parent.ObjectName.PropName=xx.yy.zz", getProviderByObjectName will have to find provider for "Parent.ObjectName"
    When trimSetProperty is called once for queryString like "Parent.ObjectName.PropName=xx.yy.zz", the output string is "Parent.ObjectName.PropName=xx.yy". 

    The while block below, trims trimQueryString upto "Parent.ObjectName" which is passed to getProviderByObjectName
    */

    std::string trimQueryString = dmUtility::trimSetProperty(queryString);

    if (trimQueryString.empty())
      return nullptr;

    std::string tmpData(trimQueryString);

    int hasEq = (tmpData.find("=") != std::string::npos);

    while(hasEq)
    {
      trimQueryString = dmUtility::trimSetProperty(tmpData);

      if (trimQueryString.empty())
        return nullptr;

      hasEq = (trimQueryString.find("=") != std::string::npos);
      tmpData = trimQueryString;
    }

    providerInfo = getProviderByObjectName(trimQueryString);
  }

  if (!providerInfo)
  {
    rtLog_Warn("failed to find provider for query string:%s", queryString);
    return nullptr;
  }

  dmQueryImpl* query = new dmQueryImpl(this);
  bool status = query->setQueryString(op, queryString);
  if (status)
    query->setProviderInfo(providerInfo);
  return query;
}

std::shared_ptr<dmProviderInfo>
dmProviderDatabase::makeProviderInfo(char const* s)
{
  std::shared_ptr<dmProviderInfo> providerInfo;

  cJSON* json = cJSON_Parse(s);
  if (!json)
    return providerInfo;

  providerInfo.reset(new dmProviderInfo());

  cJSON* p = nullptr;
  if ((p = cJSON_GetObjectItem(json, "name")) != nullptr)
    providerInfo->setObjectName(p->valuestring);
  if ((p = cJSON_GetObjectItem(json, "provider")) != nullptr)
    providerInfo->setProviderName(p->valuestring);
  if ((p = cJSON_GetObjectItem(json, "is_list")) != nullptr)
    providerInfo->setIsList(p->valueint == 1);

  // rtLog_Debug("adding object:%s", providerInfo->objectName().c_str());
  if ((p = cJSON_GetObjectItem(json, "properties")) != nullptr)
  {
    for (int i = 0, n = cJSON_GetArraySize(p); i < n; ++i)
    {
      // TODO: make this vector of pointers or shared pointers
      // std::shared_ptr<dmPropertyInfo> prop(new dmPropertyInfo());
      dmPropertyInfo prop;

      cJSON* props = cJSON_GetArrayItem(p, i);
      if (!props)
      {
        rtLog_Error("failed to get %d item from properties array", i);
        continue;
      }

      cJSON* q = nullptr;
      if ((q = cJSON_GetObjectItem(props, "name")) != nullptr)
      {
        prop.setName(q->valuestring);

        std::stringstream buff;
        buff << providerInfo->objectName();
        buff << ".";
        buff << q->valuestring;
        prop.setFullName(buff.str());
      }
      if ((q = cJSON_GetObjectItem(props, "type")) != nullptr)
        prop.setType(dmValueType_fromString(q->valuestring));
      if ((q = cJSON_GetObjectItem(props, "optional")) != nullptr)
      {
        prop.setIsOptional(q->type == cJSON_True);
      }
      if ((q = cJSON_GetObjectItem(props, "writable")) != nullptr)
      {
        prop.setIsWritable(q->type == cJSON_True);
      }
      // rtLog_Info("add prop:%s", prop.name().c_str());
      providerInfo->addProperty(prop);
    }
  }

  if (json)
    cJSON_Delete(json);

  return providerInfo;
}
