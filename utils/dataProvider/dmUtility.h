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
#ifndef __DM_UTILITY_H__
#define __DM_UTILITY_H__

#include <string>
#include <string.h>
#include <sstream>
#include "rtLog.h"

class dmUtility
{
public:
  static void splitQuery(char const* query, char* parameter)
  {
    std::string str(query);
    std::size_t position = str.find_last_of(".\\");

    parameter[0]= '\0';
    strcat(parameter, str.substr(position + 1).c_str());
    str.clear();
  }

  static bool has_suffix(const std::string &str, const std::string &suffix)
  {
    return str.size() >= suffix.size() && str.find(suffix, str.size() - suffix.size()) != str.npos;
  }

  static bool isWildcard(char const* s)
  {
    if (!s)
      return false;
    return (s[strlen(s) -1 ] == '.');
  }

  static std::string trimWildcard(std::string const& s)
  {
    if (s.empty())
      return s;

    return (s[s.size() -1] == '.')
      ? s.substr(0, s.size() -1)
      : s;
  }

  static std::string trimProperty(std::string const& s)
  {
    std::string t;
    std::string::size_type idx = s.rfind('.');
    if (idx != std::string::npos)
      t = s.substr(0, idx);
    else
      t = s;
    return t;
  }

  static std::string trimPropertyName(std::string const &s)
  {
    std::string t;
    std::string::size_type idx = s.rfind('.');
    if (idx != std::string::npos)
      t = s.substr(idx + 1);
    else
      t = s;
    return t;
  }

  static std::string trimSetProperty(std::string const &s)
  {
    std::string::size_type idx = s.find('=');
    //check for multi setter in this form Parent.ObjectName={prop1=blah..} 
    if(idx != std::string::npos && s.length() > idx+1 && s.at(idx+1) == '{' && s.back() == '}')
      return s.substr(0, idx);
    else//so must be in form: Parent.ObjectName.PropName=blah
      return trimProperty(s);
  }

  static bool parseMultisetValue(std::string const& s, std::vector< std::pair<std::string,std::string> >& nameVals)
  {
    if(s.length() < 2 || s.front() != '{' || s.back() != '}')
      return false;
    std::string s2 = s;
    s2.erase(0,1);
    s2.pop_back();
    std::vector<std::string> pairs;
    dmUtility::splitString(s2, ',', pairs);
    for (std::string const& pair : pairs)
    {
      std::vector<std::string> args;
      dmUtility::splitString(pair, '=', args);
      if(args.size() == 2)
        nameVals.push_back(std::make_pair(args[0], args[1]));
      else
        rtLog_Debug("Invalide multiset value");
    }
    return true;
  }

  static void splitString(std::string const& s, char delim, std::vector<std::string>& out)
  {
    size_t n1 = 0;
    size_t n2 = s.find(delim);
    while(true)
    {
      std::string token = s.substr(n1, n2-n1);
      out.push_back(token);
      if(n2 == std::string::npos || n2 > s.length()-1)
        break;
      n1 = n2+1;
      n2 = s.find(delim, n1);
    }
  }
};

#endif
