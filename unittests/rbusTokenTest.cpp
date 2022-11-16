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
#include "rbus_tokenchain.h"

TEST(rbusTokenTest, negtestToken1)
{
  TokenChain* tokens;
  char const* eventName="Device.rbusProvider.Param1";
  elementNode* registryElem=NULL;

  tokens = TokenChain_create(eventName, registryElem);
  EXPECT_EQ(tokens,nullptr);
}

TEST(rbusTokenTest, negtestToken2)
{
  TokenChain* tokens;
  char const* eventName=NULL;
  elementNode* registryElem = (elementNode *)malloc(sizeof(elementNode));

  tokens = TokenChain_create(eventName, registryElem);
  EXPECT_EQ(tokens,nullptr);

  free(registryElem);
  free(tokens);
}

TEST(rbusTokenTest, negtestToken3)
{
  TokenChain* tokens;
  char const eventName[] = {0};
  elementNode* registryElem = (elementNode *)malloc(sizeof(elementNode));

  tokens = TokenChain_create(eventName, registryElem);
  EXPECT_EQ(tokens,nullptr);

  free(registryElem);
  free(tokens);
}

TEST(rbusTokenTest, negtestToken4)
{
  TokenChain* tokens;
  char const* eventName="Device.rbusProvider.Param1";
  elementNode* registryElem = (elementNode *)malloc(sizeof(elementNode));

  registryElem->parent = (elementNode *)malloc(sizeof(elementNode)); 
  registryElem->parent->type = RBUS_ELEMENT_TYPE_TABLE;

  tokens = TokenChain_create(eventName, registryElem);
  EXPECT_EQ(tokens,nullptr);

  free(registryElem->parent);
  free(registryElem);
  free(tokens);
}

TEST(rbusTokenTest, negtestToken5)
{
  TokenChain* tokens;
  char const* eventName="Device.rbusProvider.Param1";
  elementNode* registryElem = (elementNode *)calloc(1,sizeof(elementNode));

  registryElem->parent = (elementNode *)calloc(1,sizeof(elementNode));

  tokens = TokenChain_create(eventName, registryElem);
  EXPECT_EQ(tokens,nullptr);

  free(registryElem->parent);
  free(registryElem);
  free(tokens);
}
