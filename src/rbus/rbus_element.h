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

#ifndef RBUS_ELEMENT_H
#define RBUS_ELEMENT_H

#include <stdlib.h>
#include <rbus.h>
#include <rtList.h>
#include <rtVector.h>
#include <rtTime.h>
#include "rbus_log.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ELM_PRIVATE_LOCK(ELM)      \
{                                                                         \
  int err;                                                                \
  elementNode *pTmp = (elementNode*) ELM;                                 \
  if((err = pthread_mutex_lock(&pTmp->elmMutex)) != 0)                    \
  {                                                                       \
    RBUSLOG_ERROR("Error @ mutex lock.. Err=%d:%s ", err, strerror(err)); \
  }                                                                       \
}

#define ELM_PRIVATE_UNLOCK(ELM)      \
{                                                                           \
  int err;                                                                  \
  elementNode *pTmp = (elementNode*) ELM;                                   \
  if((err = pthread_mutex_unlock(&pTmp->elmMutex)) != 0)                    \
  {                                                                         \
    RBUSLOG_ERROR("Error @ mutex unlock.. Err=%d:%s ", err, strerror(err)); \
  }                                                                         \
}


/******************************** STRUCTURES **********************************/
typedef struct elementNode elementNode;
typedef struct _rbusSubscription rbusSubscription_t;

typedef struct elementNode 
{
    char*                   name;           /* relative name of element */
    char*                   fullName;       /* full name/path of element */
    elementNode*            parent;         /* Up */
    elementNode*            child;          /* Downward */
    elementNode*            nextSibling;    /* Right */
    elementNode*            nextPrune;
    rbusElementType_t       type;           /* Type w/ Object=0 */
    rbusCallbackTable_t     cbTable;        /* Callback table for the element */
    rtList                  subscriptions;  /* The list of rbusSubscription_t to this element */
    char*                   alias;          /* For table rows */
    char*                   changeComp;     /* For properties, the last component to set the value */
    rtTime_t                changeTime;     /* For properties, the time the value was last set*/
    pthread_mutex_t         elmMutex;       /* To protect if there is a direct/private connection */
} elementNode;


/******************************** FUNCTIONS **********************************/
elementNode* getEmptyElementNode(void);
void freeElementNode(elementNode* node);
elementNode* insertElement(elementNode* root, rbusDataElement_t* elem);
void removeElement(elementNode* element);
elementNode* retrieveElement(elementNode* root, const char* name);
elementNode* retrieveInstanceElement(elementNode* root, const char* name);
void printRegisteredElements(elementNode* root, int level);
void fprintRegisteredElements(FILE* f, elementNode* root, int level);
void addElementSubscription(elementNode* node, rbusSubscription_t* sub, bool checkIfExists);
void removeElementSubscription(elementNode* node, rbusSubscription_t* sub);
bool elementHasAutoPubSubscriptions(elementNode* node, rbusSubscription_t* excluding);
//bool elementHasIntervalSubscriptions(elementNode* node, rbusSubscription_t* including);
void addInstanceToElement(elementNode* node, uint32_t instNum, char const* alias);
elementNode* instantiateTableRow(elementNode* tableNode, uint32_t instNum, char const* alias);
void deleteTableRow(elementNode* rowNode);
void getPropertyInstanceNames(elementNode* root, char const* query, rtVector propNameList);
void setPropertyChangeComponent(elementNode* node, char const* componentName);
void rbusElement_mutex_destroy(void);

#ifdef __cplusplus
}
#endif
#endif
