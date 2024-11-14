/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <rbuscore.h>
#include <rbus.h>
#include "rbusProviderConsumer.h"
#include <errno.h>

#define getMonth(mon) \
{ \
  (strncasecmp(mon, "jan", 3) == 0) ?  0  : \
  (strncasecmp(mon, "feb", 3) == 0) ?  1  : \
  (strncasecmp(mon, "mar", 3) == 0) ?  2  : \
  (strncasecmp(mon, "apr", 3) == 0) ?  3  : \
  (strncasecmp(mon, "may", 3) == 0) ?  4  : \
  (strncasecmp(mon, "jun", 3) == 0) ?  5  : \
  (strncasecmp(mon, "jul", 3) == 0) ?  6  : \
  (strncasecmp(mon, "aug", 3) == 0) ?  7  : \
  (strncasecmp(mon, "sep", 3) == 0) ?  8  : \
  (strncasecmp(mon, "oct", 3) == 0) ?  9  : \
  (strncasecmp(mon, "nov", 3) == 0) ?  10 : \
  (strncasecmp(mon, "dec", 3) == 0) ?  11 : \
  0 \
}

void getCompileTime(struct tm *t)
{
  char date_time_macro[32]={0};
  char *token = NULL;
  char* saveptr = NULL;

  snprintf(date_time_macro,sizeof(date_time_macro),"%s %s",__DATE__,__TIME__);

  memset(t, 0, sizeof(struct tm));
  t->tm_mon = -1;
  t->tm_sec = -1;
  t->tm_min = -1;
  t->tm_hour = -1;

  token = strtok_r(date_time_macro, " ", &saveptr);

  while(NULL != token)
  {
    if(-1 == t->tm_mon)
      t->tm_mon = getMonth(token);
    else if(0 == t->tm_mday)
      t->tm_mday = atoi(token);
    else if(0 == t->tm_year)
      t->tm_year = atoi(token);
    else if(-1 == t->tm_hour)
      t->tm_hour = atoi(token);
    else if(-1 == t->tm_min)
      t->tm_min = atoi(token);
    else if(-1 == t->tm_sec)
      t->tm_sec = atoi(token);

    if(0 == t->tm_year)
      token = strtok_r(NULL, " ", &saveptr);
    else
      token = strtok_r(NULL, ":", &saveptr);
  }

  return;
}

void isElementPresent(rbusHandle_t handle, const char *elementName)
{
  rbusError_t rc = RBUS_ERROR_SUCCESS;
  char const* pElementName[1] = {0};
  int componentCnt = 0, elementFound = 0;
  char **pComponentNames;

  while(!elementFound)
  {
    pElementName[0] = elementName;
    rc = rbus_discoverComponentName (handle, 1, pElementName, &componentCnt, &pComponentNames);
    if(RBUS_ERROR_SUCCESS == rc)
    {
      if(componentCnt)
      {
        EXPECT_EQ(componentCnt, 1);
        /*componentCnt should never be greater than 1*/
        if(componentCnt > 1) pause();

        elementFound = 1;
        free(pComponentNames[0]);
      }
      free(pComponentNames);
    }
    else
    {
      printf ("Failed to discover components. Error Code = %d\r\n", rc);
    }
    if(!elementFound) usleep(5000);
  }
}
