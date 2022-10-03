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
#include <rbus.h>
#include <stdio.h>
#include <string.h>

TEST(rbusSessionTest, test1)
{
  int rc = RBUS_ERROR_BUS_ERROR;
  rbusHandle_t handle = NULL;
  unsigned int sessionId = 0 , newSessionId = 0;
  char *componentName = NULL;

  componentName = strdup("sessiontest");

  rc = rbus_open(&handle, componentName);
  EXPECT_EQ(rc,RBUS_ERROR_SUCCESS);

  if(RBUS_ERROR_SUCCESS == rc)
  {
    rc = rbus_createSession(handle , &sessionId);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);

    rc = rbus_getCurrentSession(handle , &newSessionId);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
    EXPECT_EQ(sessionId, newSessionId);

    rc = rbus_closeSession(handle, sessionId);
    EXPECT_EQ(rc, RBUS_ERROR_SUCCESS);
    rbus_close(handle);
  }
  free(componentName);
}
