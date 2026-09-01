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
#include "rtDebug.h"
#include "rtLog.h"
#include <stdio.h>

void
rtDebug_PrintBuffer(char const* label, uint8_t* p, int n)
{
  int i;
  printf("------ BEGIN %s ------", label);
  for (i = 0; i < n; ++i)
  {
    if (i % 16 == 0)
      printf("\n");
    printf("0x%02x ", p[i]);
  }
  printf("------ END   %s ------\n", label);
  printf("\n");
}
