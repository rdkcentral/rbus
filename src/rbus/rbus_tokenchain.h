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

#ifndef RBUS_TOKENCHAIN_H
#define RBUS_TOKENCHAIN_H

#include "rbus_element.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TokenType
{
    TokenNonRow,
    TokenAlias,
    TokenInstNum,
    TokenWildcard
} TokenType;

typedef struct Token
{
    char* text;         /* text of token. e.g. the 'WiFi' in 'Device.WiFi.Radio.1' */
    elementNode* node;  /* the corresponding registration node in the element tree */
    TokenType type;     /* type of expression used to identify a row instance*/
    struct Token* prev; /* the previous token in list */
    struct Token* next; /* the next token in list */
} Token;

typedef struct TokenChain
{
    Token* first;       /* first token in chain */
    Token* last;        /* last token in chain -- if you want to go backwards through list */
} TokenChain;

TokenChain* TokenChain_create(char const* sourceName, elementNode* regNode);

void TokenChain_destroy(TokenChain* chain);

bool TokenChain_match(TokenChain* chain, elementNode* instNode);

void TokenChain_print(TokenChain* chain);

#ifdef __cplusplus
}
#endif

#endif

