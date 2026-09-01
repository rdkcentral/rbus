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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include "dmClient.h"

#ifdef INCLUDE_BREAKPAD
#include "breakpadwrap.h"
#endif

void print_help()
{
  printf("dmcli [OPTIONS] [COMMAND] [PARAMS]\n");
  printf("\t-d  --datamodel   Set datamodel directory\n");
  printf("\t-h  --help        Print this help and exit\n");
  printf("\t-v  --verbose     Print verbose output. same as -l 0\n");
  printf("\t-l  --log-level   Set log level 0(verbose)-4\n");
  printf("\t-g  --get         Gets a list of parameters\n");
  printf("\t-s  --set         Sets a list of parameters\n");
  printf("\t-r  --recursive   get all sub-ojects, recursively. only applies when -g(--get) is used.  \n");
  printf("\n");
  printf("Examples:\n");
  printf("\t1.) dmcli -g Device.DeviceInfo.ModelName,Device.DeviceInfo.SerialNumber\n");
  printf("\t2.) dmcli -s Services.Service.Foo.Bar=1,Services.Sevice.Foo.Baz=2\n");
  printf("\n");
}

//for non-list query, expect a single onResult or onError
//for lists, 1 query per list item is made so expect onResult or onError for each item in the list
class Notifier : public dmClientNotifier
{
public:
  virtual void onResult(const dmQueryResult& result)
  {
    size_t maxParamLength = 0;
    for (auto const& param : result.values())
    {
      size_t len = param.Info.name().length();
        if (len > maxParamLength)
          maxParamLength = len;
    }
    maxParamLength += result.objectName().length() + 1;

    std::cout << std::endl;
    for (auto const& param: result.values())
    {
      if(param.StatusCode == 0)
      {
        std::cout << "    ";
        std::cout.width(maxParamLength);
        std::cout << std::left;
        std::cout << result.objectName() + "." + param.Info.name();
        std::cout << " = ";
        std::cout << param.Value.toString();
        std::cout << std::endl;
      }
    }
    for (auto const& param: result.values())
    {
      if(param.StatusCode != 0)
      {
        std::cout << "    Error " << param.StatusCode << ": " << param.StatusMessage << ". " << result.objectName() + "." + param.Info.name() << std::endl;
      }
    }
    std::cout << std::endl;
  }
  virtual void onError(int status, std::string const& message)
  {
    std::cout << std::endl << "    Error " << status << ": " << message << std::endl;
  }
};

void splitParams(std::string const& params, std::vector<std::string>& tokens)
{
  size_t first = 0;
  size_t last = 0;
  bool inbracket;
  for(last = 0; last < params.size(); ++last)
  {
    if(params[last] == '{')
      inbracket = true;
    else if(params[last] == '}')
      inbracket = false;
    else if(params[last] == ',' && !inbracket)
    {
      tokens.push_back(params.substr(first, last-first));
      first = last+1;
    }
  }
  if(last == params.size() && last - first > 1)
      tokens.push_back(params.substr(first, last-first));
}

int main(int argc, char *argv[])
{
  int exit_code = 0;
  std::string param_list;
  bool recursive = false;
  rtLogLevel log_level = RT_LOG_FATAL;

#ifdef INCLUDE_BREAKPAD
  sleep(1);
  BreakPadWrapExceptionHandler eh;
  eh = newBreakPadWrapExceptionHandler();
#endif

  #ifdef DEFAULT_DATAMODELDIR
  std::string datamodel_dir = DEFAULT_DATAMODELDIR;
  #else
  std::string datamode_dir;
  #endif

  dmProviderOperation op = dmProviderOperation_Get;

  if (argc == 1)
  {
    print_help();
    return 0;
  }

  while (1)
  {
    int option_index;
    static struct option long_options[] = 
    {
      { "get", required_argument, 0, 'g' },
      { "set", required_argument, 0, 's' },
      { "recursive", no_argument, 0, 'r' },
      { "help", no_argument, 0, 'h' },
      { "datamode-dir", required_argument, 0, 'd' },
      { "verbose", no_argument, 0, 'v' },
      { "log-level", no_argument, 0, 'l' },
      { 0, 0, 0, 0 }
    };

    int c = getopt_long(argc, argv, "d:g:hs:rvl:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
      case 'd':
        datamodel_dir = optarg;
        break;

      case 'g':
        param_list = optarg;
        op = dmProviderOperation_Get;
        break;

      case 's':
        param_list = optarg;
        op = dmProviderOperation_Set;
        break;

      case 'r':
        recursive = true;
        break;

      case 'h':
        print_help();
        exit(0);
        break;

      case 'v':
        log_level = RT_LOG_DEBUG;
        break;

      case 'l':
        log_level = (rtLogLevel)atoi(optarg);
        break;

      case '?':
        break;
    }
  }

  if (param_list.empty())
  {
    printf("\nNo parameter list found\n");
    print_help();
    exit(1);
  }

  dmClient* client = dmClient::create(datamodel_dir.c_str(), log_level);

  std::vector<std::string> tokens;
  splitParams(param_list, tokens);

  for(auto token: tokens)
  {
    Notifier notifier;
    client->runQuery(op, token, recursive, &notifier);
  }

  dmClient::destroy(client);

  return exit_code;
}
