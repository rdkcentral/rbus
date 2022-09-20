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
#include <rtLog.h>
#include <rtError.h>
#include <unistd.h>

class AdapterObject : public dmProvider
{
public:
  AdapterObject(const char* alias) : malias(alias)
  {
    onGet("Alias", [this]() -> dmValue { return malias.c_str(); });
  }
  std::string malias;
};

class InterfaceObject : public dmProvider
{
public:
  InterfaceObject(const char* alias) : malias(alias)
  {
    onGet("Alias", [this]() -> dmValue { return malias.c_str(); });
  }
  std::string malias;
};

class RouterObject : public dmProvider
{
public:
  RouterObject()
  {
    mname = "Router";
    onGet("Name", [this]() -> dmValue { return mname.c_str(); });
    onSet("Name", [this](dmValue const& value) -> void { mname = value.toString();});
  }
  std::string mname;
};

class EndPointObject : public dmProvider
{
public:
  EndPointObject(const char* alias, const char* ssid, const char* security, const char* channel, const char* mode, const char* status) 
    : malias(alias), mssid(ssid), msecurityType(security), mchannel(channel), mmode(mode), mstatus(status)
  {
    onGet("Status", [this]() -> dmValue { return mstatus.c_str(); });
    onSet("Status", [this](dmValue const& value) -> void { 
      if(isTransaction())
        mstatusT = value.toString(); 
      else
        mstatus = value.toString(); 
    });
    onGet("SSID", [this]() -> dmValue { return mssid.c_str(); });
    onSet("SSID", [this](dmValue const& value) -> void { 
      if(isTransaction())
        mssidT = value.toString(); 
      else
        mssid = value.toString();
    });
    onGet("AdapterNumberOfEntries", [this]() -> dmValue { return 1; });
    onGet("AdapterIdsOfEntries", [this]() -> dmValue { return "1"; });
    onGet("InterfaceNumberOfEntries", [this]() -> dmValue { return 1; });
    onGet("InterfaceIdsOfEntries", [this]() -> dmValue { return "1"; });
  }
protected:
  void doGet(dmPropertyInfo const& param, dmQueryResult& result)
  {
    if(param.name() == "Alias")
    {
      rtLog_Warn("doGet Alias");
      result.addValue(param, malias.c_str());
    }
  }
  void doSet(dmPropertyInfo const& info, dmValue const& value, dmQueryResult& result)
  {
    if(info.name() == "Alias")
    {
      rtLog_Warn("doSet Alias");
      malias = value.toString();
      result.addValue(info, malias.c_str());
    }
  }
  void beginTransaction()
  {
    mssidT.clear();
    mstatusT.clear();
  }
  void endTransaction()
  {
    if(!mssidT.empty() && !mstatusT.empty())
    {
      mssid = mssidT;
      mstatus = mstatusT;
      rtLog_Warn("EndPointObject committing transaction SSID=%s STATE=%s", mssid.c_str(), mstatus.c_str());
    }
    mssidT.clear();
    mstatusT.clear();
  }
private:
  std::string malias;
  std::string mssid;
  std::string msecurityType;
  std::string mchannel;
  std::string mmode;
  std::string mstatus;

  std::string mssidT;
  std::string mstatusT;
};

class WiFiObject : public dmProvider
{
public:
  WiFiObject() : m_noiseLevel("10dB"), m_userName("xcam_user")
  {
    onGet("EndPointNumberOfEntries", [this]() -> dmValue { return 3; });
    onGet("EndPointIdsOfEntries", [this]() -> dmValue { return "1,2,3"; });
    onGet("X_RDKCENTRAL-COM_NoiseLevel", [this]() -> dmValue { return "100db"; });
  }

private:
protected:
private:

  std::string m_noiseLevel;
  std::string m_userName;
};

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  //dmUtility::QueryParser::test(argv[1]);return 0;  
  
  dmProviderHost* host = dmProviderHost::create();
  host->start();

  host->registerProvider("Device.WiFi", std::unique_ptr<WiFiObject>(new WiFiObject()));
  host->registerProvider("Device.WiFi.EndPoint.1", std::unique_ptr<EndPointObject>(new EndPointObject("Netgear1", "NETGEAR1","WPA","0","12","Online")));
  host->registerProvider("Device.WiFi.EndPoint.2", std::unique_ptr<EndPointObject>(new EndPointObject("Netgear2", "RAWHIDE123","WPA+PKI","1","3","Online")));
  host->registerProvider("Device.WiFi.EndPoint.3", std::unique_ptr<EndPointObject>(new EndPointObject("XB3", "MyXin","Comcast","0","11","Offline")));
  host->registerProvider("Device.WiFi.EndPoint.1.Adapter.1", std::unique_ptr<AdapterObject>(new AdapterObject("a1")));
  host->registerProvider("Device.WiFi.EndPoint.2.Adapter.1", std::unique_ptr<AdapterObject>(new AdapterObject("a2")));
  host->registerProvider("Device.WiFi.EndPoint.3.Adapter.1", std::unique_ptr<AdapterObject>(new AdapterObject("a3")));
  host->registerProvider("Device.WiFi.EndPoint.1.Interface.1", std::unique_ptr<InterfaceObject>(new InterfaceObject("i1")));
  host->registerProvider("Device.WiFi.EndPoint.2.Interface.1", std::unique_ptr<InterfaceObject>(new InterfaceObject("i2")));
  host->registerProvider("Device.WiFi.EndPoint.3.Interface.1", std::unique_ptr<InterfaceObject>(new InterfaceObject("i3")));
  host->registerProvider("Device.WiFi.Router", std::unique_ptr<RouterObject>(new RouterObject()));
  while (true)
  {
    sleep(1);
  }

  host->stop();
  return 0;
}
