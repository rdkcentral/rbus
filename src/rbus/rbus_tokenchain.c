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

#include "rbus_tokenchain.h"
#include <rtMemory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DEBUG_TOKEN 0

/*
    A consumer can subsrcibe with event names that included three type of expressions
    for row identification: instance number, alias, and wildcard.

    Examples consumer subscription event names:
        Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength
        Device.WiFi.AccessPoint.[ap1].AssociatedDevice.*.SignalStrength
        Device.WiFi.AccessPoint.1.AssociatedDevice.*.SignalStrength

    The purpose of a TokenChain is to provide a facility to quickly check if an instantiated
    element matches a subscription event name.  

    For example, if a new row is added to the AssociatedDevice table with instance number 1
    and alias 'ap1', the following element for SignalStrength is instantiated:
        Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength

    This element can also be reference by alias 'api1' like this:
        Device.WiFi.AccessPoint.1.AssociatedDevice.[ap1].SignalStrength

    Here are some examples of event names that would match this new row:
        Yes for eventName=Device.WiFi.AccessPoint.1.AssociatedDevice.1.SignalStrength
        Yes for eventName=Device.WiFi.AccessPoint.1.AssociatedDevice.*.SignalStrength
        Yes for eventName=Device.WiFi.AccessPoint.*.AssociatedDevice.1.SignalStrength
        Yes for eventName=Device.WiFi.AccessPoint.*.AssociatedDevice.*.SignalStrength
        Yes for eventName=Device.WiFi.AccessPoint.1.AssociatedDevice.[ap1].SignalStrength
        Yes for eventName=Device.WiFi.AccessPoint.*.AssociatedDevice.[ap1].SignalStrength

    Here are some example of event names that would not match this new row:
        No for eventName=Device.WiFi.AccessPoint.1.AssociatedDevice.2.SignalStrength  
        No for eventName=Device.WiFi.AccessPoint.2.AssociatedDevice.*.SignalStrength
        No for eventName=Device.WiFi.AccessPoint.*.AssociatedDevice.2.SignalStrength  
        No for eventName=Device.WiFi.AccessPoint.1.AssociatedDevice.[ap2].SignalStrength

    The token chain will be used to tokenize a subsrciption event name.  Then
    given an instantiated element node, the node and the token chain are iterated through
    in parallel.   The node is iterated from node (e.g. leaf) to parent to grandparent ect,
    and the token chain in reverse order.  With each iteration, the element name and token
    text is compared for a match.  When rows are encounterer, extra testing is done for 
    instance number, alias, and wildcard to determine if the instance row matches the
    event name row expression.
*/

TokenChain* TokenChain_create(char const* sourceName, elementNode* regNode)
{
    TokenChain* chain = NULL;
    Token* next = NULL;
    int nameLen;
    char* name = NULL;
    char* ptr = NULL;
    elementNode* node = NULL;

    if(regNode == NULL)
    {
        RBUSLOG_ERROR("ERROR: regNode NULL");
        return NULL;
    }

    if(sourceName == NULL)
    {
        RBUSLOG_ERROR("ERROR: sourceName NULL");
        return NULL;
    }

    nameLen = (int)strlen(sourceName);

    if(nameLen == 0)    
    {
        RBUSLOG_ERROR("ERROR: sourceName EMPTY");
        return NULL;
    }

    chain = rt_malloc(sizeof(TokenChain));

    chain->first = chain->last = NULL;

    name = strdup(sourceName);

    node = regNode;

    ptr = name + nameLen -1;

#   if DEBUG_TOKEN
    RBUSLOG_INFO("%s DEBUG: %s, %s", __FUNCTION__, sourceName, regNode->fullName);
#   endif

    while(ptr != name && node->parent)
    {
        Token* tok;

        if(*ptr == '.')
        {
            *ptr = 0;
            ptr--;
        }

        while(*ptr != '.' && ptr != name)
        {
            ptr--;
        }

        tok = rt_malloc(sizeof(Token));
        tok->node = node;
        tok->prev = NULL;
        tok->type = TokenNonRow;

        if(*ptr == '.')
        {
            tok->text = ptr+1;
        }
        else
        {
            tok->text = ptr;
        }

        if(chain->last == NULL)
        {
            chain->last = tok;
            tok->next = NULL;
        }
        else
        {
            tok->next = next;
            next->prev = tok;
        }

        next = tok;

        /* handle table rows */
        if(node->parent && node->parent->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            if(*tok->text == '[') /* alias */
            {
                char* p;

                tok->text++; /* move past [ to record just the alias */

                p = tok->text + 1;

                while(*p != ']' && (p - name) < nameLen)
                {
                    p++;
                }

                if(*p == ']')
                {
                    *p = 0;

                    tok->type = TokenAlias;
                }
                else
                {
                    RBUSLOG_ERROR("ERROR: alias missing closing bracket: %s", tok->text);
                    goto tokenChainError;
                }
            }
            else
            {
                if(strcmp(tok->text, "*") == 0) /* wildcard */
                {
                    tok->type = TokenWildcard;
                }
                else /* instance number */
                {
                    if(atoi(tok->text) > 0)
                    {
                        tok->type = TokenInstNum;
                    }
                    else
                    {   
                        RBUSLOG_ERROR("ERROR: invalid instance number %s", tok->text);
                        goto tokenChainError;
                    }
                }   
            }
        }

#       if DEBUG_TOKEN
        RBUSLOG_INFO("%s DEBUG: token->name=%s token->type=%d source->name=%s source->type=%d", __FUNCTION__, tok->text, tok->type, node->name, node->type);
#       endif

        node = node->parent;
    }

    if(ptr != name || node->parent != NULL)
    {
        RBUSLOG_ERROR("ERROR: token count missmatch source=%s node=%s", sourceName, regNode->fullName);
        goto tokenChainError;
    }

    chain->first = next;
    return chain;

tokenChainError:

    next = chain->last;
    while(next)
    {
        Token* last = next;
        next = next->next;
        free(last);
    }
    free(chain);
    free(name);
    return NULL;
};

void TokenChain_destroy(TokenChain* chain)
{
    if(!chain)
        return;
    Token* token = chain->first;
    if(token)
        free(token->text);
    while(token)
    {
        Token* next = token->next;
        free(token);
        token = next;
    }
    free(chain);
}

bool TokenChain_match(TokenChain* chain, elementNode* instNode)
{
    if((!chain)||(!instNode))
        return false;
    Token* token = chain->last;
    elementNode* inst = instNode;
    int rc;

#   if DEBUG_TOKEN
    RBUSLOG_INFO("%s DEBUG: instNode=%s tokenChain=", __FUNCTION__, instNode->fullName);
    TokenChain_print(chain);
#   endif

    while(token && inst && inst->parent != NULL)
    {
#       if DEBUG_TOKEN
        RBUSLOG_INFO("%s DEBUG: comparing inst %s:%d to token %s:%d", __FUNCTION__, inst->name, inst->type, token->text, token->node->type);
#       endif

        if(token->node->type != inst->type)
        {
#           if DEBUG_TOKEN
            RBUSLOG_INFO("%s DEBUG: inst type %d doesn't match token type %d", __FUNCTION__, inst->type, token->node->type);
#           endif

            return false;
        }

        if(token->node->parent && token->node->parent->type == RBUS_ELEMENT_TYPE_TABLE)
        {
#       if DEBUG_TOKEN
            RBUSLOG_INFO("%s DEBUG: comparing parent inst %s:%d to token %s:%d", __FUNCTION__, inst->parent->name, inst->parent->type, token->next->text, token->node->parent->type);
#       endif

            if(inst->parent->type != RBUS_ELEMENT_TYPE_TABLE)
            {
#               if DEBUG_TOKEN
                RBUSLOG_INFO("%s DEBUG: inst node is not a row %s", __FUNCTION__, inst->name);
#               endif

                return false;
            }

            if(token->type == TokenInstNum)
            {
                rc = strcmp(inst->name, token->text);

#               if DEBUG_TOKEN
                RBUSLOG_INFO("%s DEBUG: instance numbers %s and %s %s", __FUNCTION__, inst->name, token->text, rc==0 ? "match" : "don't match");
#               endif

                if(rc != 0)
                {
                    return false;
                }
            }
            else if(token->type == TokenAlias)
            {
                if(inst->alias)
                {
                    rc = strcmp(inst->alias, token->text);

#                   if DEBUG_TOKEN
                    RBUSLOG_INFO("%s DEBUG: aliases %s and %s %s", __FUNCTION__, inst->alias, token->text, rc==0 ? "match" : "don't match");
#                   endif
                }

                if(rc != 0)
                {
                    return false;
                }
            }
            else
            {
                assert(token->type == TokenWildcard);
            }
        }
        else
        {
            rc = strcmp(inst->name, token->text);

#           if DEBUG_TOKEN
            RBUSLOG_INFO("%s DEBUG: instance numbers %s and %s %s", __FUNCTION__, inst->name, token->text, rc==0 ? "match" : "don't match");
#           endif

            if(rc != 0)
            {
                return false;
            }
        }

        token = token->prev;
        inst = inst->parent;
    }

    if( (token == NULL && inst->parent != NULL) || (token && inst->parent == NULL))
    {
#       if DEBUG_TOKEN
        RBUSLOG_INFO("%s DEBUG: token counts don't match", __FUNCTION__);
#       endif

        return false;
    }

#   if DEBUG_TOKEN
    RBUSLOG_INFO("%s DEBUG: match found", __FUNCTION__);
#   endif

    return true;
}

#if DEBUG_TOKEN
void TokenChain_print(TokenChain* chain)
{
    Token* token = chain->first;
    while(token)
    {
        RBUSLOG_INFO("%s->", token->text);
        token = token->next;
    }
    RBUSLOG_INFO("(null)");
}
#endif
