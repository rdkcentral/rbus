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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rbus.h>
#include <assert.h>
#include "rbus_element.h"
#include "rbus_subscriptions.h"
#include <rtMemory.h>

#define VERIFY_NULL(T) if(NULL == T){ return; }
#define DEBUG_ELEMENTS 0

#define ERROR_CHECK(CMD) \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    RBUSLOG_ERROR("Error %d:%s running command " #CMD, err, strerror(err)); \
  } \
}
#define LOCK() ERROR_CHECK(pthread_mutex_lock(&element_mutex))
#define UNLOCK() ERROR_CHECK(pthread_mutex_unlock(&element_mutex))

elementNode* pruneNode = NULL;
pthread_mutex_t element_mutex;
static int mutex_init = 0;

//****************************** UTILITY FUNCTIONS ***************************//
char const* getTypeString(rbusElementType_t type)
{
    switch(type)
    {
    case RBUS_ELEMENT_TYPE_PROPERTY: return "property"; break;
    case RBUS_ELEMENT_TYPE_TABLE: return "table"; break;
    case RBUS_ELEMENT_TYPE_EVENT: return "event"; break;
    case RBUS_ELEMENT_TYPE_METHOD: return "method"; break;
    default: return "object"; break;
    }
}
//****************************************************************************//


//********************************* FUNCTIONS ********************************//
elementNode* getEmptyElementNode(void)
{
    elementNode* node;
    pthread_mutexattr_t attrib;

    node = (elementNode *) rt_calloc(1, sizeof(elementNode));
    node->type = 0;//default of zero means OBJECT and if this gets used as a leaf, it will get update to be a either parameter, event, or method
    ERROR_CHECK(pthread_mutexattr_init(&attrib));
    ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK));
    ERROR_CHECK(pthread_mutex_init(&node->elmMutex, &attrib));

    return node;
}

static void freeElementRecurse(elementNode* node)
{
    VERIFY_NULL(node);
    elementNode* child = node->child;

    while(child)
    {
        elementNode* tmp = child;
        child = child->nextSibling;
        freeElementRecurse(tmp);
    }

    if (node->name)
    {
        free(node->name);
    }
    if (node->fullName)
    {
        free(node->fullName);
    }
    if (node->alias)
    {
        free(node->alias);
    }
    if (node->subscriptions)
    {
        rtList_Destroy(node->subscriptions, NULL);
    }
    if (node->changeComp)
    {
        free(node->changeComp);
    }

    free(node);

}

void freeElementNode(elementNode* node)
{
    VERIFY_NULL(node);
    elementNode* parent = node->parent;
    elementNode* child = node->child;

    while(child)
    {
        elementNode* tmp = child;
        child = child->nextSibling;
        freeElementRecurse(tmp);
    }

    if(parent)
    {
        if(parent->child == node)
        {
            parent->child = node->nextSibling;
        }
        else
        {
            child = parent->child;
            while(child)
            {
                if(child->nextSibling == node)
                {
                    child->nextSibling = node->nextSibling;
                    break;
                }
                child = child->nextSibling;
            }
        }
    }

    if (node->name)
    {
        free(node->name);
    }
    if (node->fullName)
    {
        free(node->fullName);
    }
    if (node->alias)
    {
        free(node->alias);
    }
    if (node->subscriptions)
    {
        rtList_Destroy(node->subscriptions, NULL);
    }
    if (node->changeComp)
    {
        free(node->changeComp);
    }
    free(node);

    /*remove objects with no children
     could be an intermitent object added during insertElem
     or it could be a table which no longer has the {i}
     however don't remove rows which are allowed to exist without child) */
    if(parent && parent->parent && parent->child == NULL && parent->parent->type != RBUS_ELEMENT_TYPE_TABLE)
    {
        if(pruneNode)
            parent->nextPrune = pruneNode;
        pruneNode = parent;
    }
}

static void pruneTree()
{
    while(pruneNode)
    {
        elementNode* tmp = pruneNode;
        pruneNode = pruneNode->nextPrune;
        freeElementNode(tmp);
    }
}

static void createElementChain(elementNode* node, elementNode*** chainOut, int* numChain)
{
    elementNode** chain;
    elementNode* parent;
    int num = 0;
    int i = 0;

    VERIFY_NULL(node);
    parent = node;
    while(parent)
    {
        num++;
        parent = parent->parent;
    }
    chain = rt_malloc(num * sizeof(elementNode*));
    parent = node;
    while(parent)
    {
        chain[num - (i++) - 1] = parent;
        parent = parent->parent;
    }
#if DEBUG_ELEMENTS
    printf("createElementChain %s\n", node->fullName);
    for(i = 0; i < num; ++i)
        printf( "Chain %d: %s\n", i, chain[i]->name );
#endif
    *chainOut = chain;
    *numChain = num;
}

static void replicateAcrossTableRowInstances(elementNode* newNode);

elementNode* insertElement(elementNode* root, rbusDataElement_t* elem)
{
    char* token = NULL;
    char* name = NULL;
    char* saveptr = NULL;
    elementNode* currentNode = root;
    elementNode* tempNode = NULL;
    elementNode* nextNode = NULL;
    int ret = 0, createChild = 0;
    char buff[RBUS_MAX_NAME_LENGTH];
    pthread_mutexattr_t attrib;

    if(!mutex_init)
    {
       ERROR_CHECK(pthread_mutexattr_init(&attrib));
       ERROR_CHECK(pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE));
       ERROR_CHECK(pthread_mutex_init(&element_mutex, &attrib));
       mutex_init = 1;
    }

    if(currentNode == NULL || elem == NULL)
    {
        return NULL;
    }
    LOCK();
    nextNode = currentNode->child;
    createChild = 1;

    RBUSLOG_DEBUG("Request to insert element [%s]!!", elem->name);

    name = strdup(elem->name);

    /* If this is a table being registered using .{i}. syntax, such as
       "Device.WiFi.AccessPoint.{i}.", then we strip off the .{i}.
       because its the token before that which is the actual table name.
        e.g. "Device.WiFi.AccessPoint." is the table and {i} is a row placeholder.
       After the table is added below, we will add the {i} as child of table, 
        but this {i} will be called a row template.  And from this template
        we can instantiate all the objects and properties under it when we 
        add a row instance.
     */
    if(elem->type == RBUS_ELEMENT_TYPE_TABLE)
    {
        size_t len = strlen(name);
        if(len > 4)
        {
            if(strcmp(name + len - 5, ".{i}.") == 0)
            {
                name[len-5] = 0;
            }
            else if(strcmp(name + len - 4, ".{i}") == 0)
            {
                name[len-4] = 0;
            }
        }
    }

    token = strtok_r(name, ".", &saveptr);

    while( token != NULL )
    {
        if(nextNode == NULL)
        {
            if(createChild)
            {
                RBUSLOG_DEBUG("Create child [%s]", token);
                tempNode = getEmptyElementNode();
                tempNode->parent = currentNode;
                if(currentNode == root)    
                {
                    tempNode->fullName = strdup(token);
                }
                else
                {
                    snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->fullName, token);
                    tempNode->fullName = strdup(buff);
                }
                tempNode->name = strdup(token);
                currentNode->child = tempNode;
                currentNode = tempNode;
                nextNode = currentNode->child;
                createChild = 1;
            }
        }
        while(nextNode != NULL)
        {
            RBUSLOG_DEBUG("child name=[%s], Token = [%s]", nextNode->name, token);
            if(strcmp(nextNode->name, token) == 0)
            {
                currentNode = nextNode;
                nextNode = currentNode->child;
                createChild = 1;
                break;
            }
            else
            {
                currentNode = nextNode;
                nextNode = currentNode->nextSibling;
                createChild = 0;
                if(nextNode == NULL)
                {
                    RBUSLOG_DEBUG("Create Sibling [%s]", token);
                    tempNode = getEmptyElementNode();
                    tempNode->parent = currentNode->parent;
                    if(currentNode->parent->fullName)
                        snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->parent->fullName, token);
                    else
                        snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s", token);
                    RBUSLOG_DEBUG("Full name [%s]", buff);
                    tempNode->fullName = strdup(buff);
                    tempNode->name = strdup(token);
                    currentNode->nextSibling = tempNode;
                    currentNode = tempNode;
                    createChild = 1;
                }
            }
        }
        token = strtok_r(NULL, ".", &saveptr);
    }
    if(ret == 0)
    {
        currentNode->type = elem->type;
        currentNode->cbTable.getHandler = elem->cbTable.getHandler;
        currentNode->cbTable.setHandler = elem->cbTable.setHandler;
        currentNode->cbTable.tableAddRowHandler = elem->cbTable.tableAddRowHandler;
        currentNode->cbTable.tableRemoveRowHandler = elem->cbTable.tableRemoveRowHandler;
        currentNode->cbTable.eventSubHandler = elem->cbTable.eventSubHandler;

        /* See the big comment near the top of this function.
           We add {i} as a child object of the table.
           This will be the row template used to instantiate rows from.
           Its presumed a provider will register more elements under this, such as
            Device.WiFi.AccessPoint.{i}.Foo etc,...
         */
        if(elem->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            elementNode* rowTemplate = getEmptyElementNode();
            rowTemplate->parent = currentNode;
            rowTemplate->name = strdup("{i}");
            snprintf(buff, RBUS_MAX_NAME_LENGTH, "%s.%s", currentNode->fullName, rowTemplate->name);
            rowTemplate->fullName = strdup(buff);
            currentNode->child = rowTemplate;
            currentNode->cbTable.tableSyncHandler = elem->cbTable.methodHandler;
        }
        else
        {
            currentNode->cbTable.methodHandler = elem->cbTable.methodHandler;
        }
    }
    free(name);
    UNLOCK();

    if(ret == 0)
    {
        replicateAcrossTableRowInstances(currentNode);

        return currentNode;
    }
    else
    {
        return NULL;
    }
}

elementNode* retrieveElement(elementNode* root, const char* elmentName)
{
    char* token = NULL;
    char* name = NULL;
    char* saveptr = NULL;
    elementNode* currentNode = root;
    elementNode* nextNode = NULL;
    int tokenFound = 0;  
    RBUSLOG_DEBUG("Request to retrieve element [%s]", elmentName);
    if(currentNode == NULL)
    {
        return NULL;
    }
    LOCK();
    name = strdup(elmentName);
    nextNode = currentNode->child;

    /*TODO if name is a table row with an alias containing a dot, this will break (e.g. "Foo.[alias.1]")*/
    token = strtok_r(name, ".", &saveptr);
    while( token != NULL)
    {
        RBUSLOG_DEBUG("Token = [%s]", token);
        tokenFound = 0;
        if(nextNode == NULL)
        {
            break;
        }

        RBUSLOG_DEBUG("child name=[%s], Token = [%s]", nextNode->name, token);
        /*
        if(nextNode->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            assert(strcmp(nextNode->name, "{i}") == 0);
            tokenFound = 1;
            currentNode = nextNode;
            nextNode = currentNode->child;
        }
        else*/
        if(strcmp(nextNode->name, token) == 0)
        {
            RBUSLOG_DEBUG("tokenFound!");
            tokenFound = 1;
            currentNode = nextNode;
            nextNode = currentNode->child;
        }
        else
        {
            currentNode = nextNode;
            nextNode = currentNode->nextSibling;

            while(nextNode != NULL)
            {
                RBUSLOG_DEBUG("child name=[%s], Token = [%s]", nextNode->name, token);
                if(strcmp(nextNode->name, token) == 0)
                {
                    RBUSLOG_DEBUG("tokenFound!");
                    tokenFound = 1;
                    currentNode = nextNode;
                    nextNode = currentNode->child;
                    break;
                }
                else
                {
                    currentNode = nextNode;
                    nextNode = currentNode->nextSibling;
                }
            }
        }

        token = strtok_r(NULL, ".", &saveptr);

        if(token && nextNode && nextNode->parent && nextNode->parent->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            /* retrieveElement should only return regististration elements, not table row instantiated elements */
            token = "{i}";
        }
    }

    free(name);
    UNLOCK();

    if(tokenFound)
    {
        RBUSLOG_DEBUG("Found Element with param name [%s]", currentNode->name);
        return currentNode;
    }
    else
    {
        return NULL;
    }
}

elementNode* retrieveInstanceElement(elementNode* root, const char* elmentName)
{
    return retrieveInstanceElementEx(NULL, root, elmentName, false);
}

elementNode* retrieveInstanceElementEx(rbusHandle_t handle, elementNode* root, const char* elmentName, bool syncTables)
{
    char* token = NULL;
    char* name = NULL;
    char* saveptr = NULL;
    elementNode* currentNode = root;
    elementNode* nextNode = NULL;
    int tokenFound = 0;
    bool isWildcard = false;

    RBUSLOG_DEBUG("Request to retrieve element [%s]", elmentName);
   
    if(currentNode == NULL)
    {
        return NULL;
    }
    LOCK();	
    name = strdup(elmentName);
    nextNode = currentNode->child;

    /*TODO if name is a table row with an alias containing a dot, this will break (e.g. "Foo.[alias.1]")*/
    token = strtok_r(name, ".", &saveptr);
    while( token != NULL)
    {
        RBUSLOG_DEBUG("Token = [%s]", token);
        tokenFound = 0;

        if(nextNode == NULL)
        {
            break;
        }

        RBUSLOG_DEBUG("child name=[%s], Token = [%s]", nextNode->name, token);

        if(strcmp(nextNode->name, token) == 0)
        {
            RBUSLOG_DEBUG("tokenFound!");
            tokenFound = 1;
        }
        else
        {
            currentNode = nextNode;
            nextNode = currentNode->nextSibling;

            while(nextNode != NULL)
            {
                RBUSLOG_DEBUG("child name=[%s], Token = [%s]", nextNode->name, token);
                if(strcmp(nextNode->name, token) == 0)
                {
                    RBUSLOG_DEBUG("tokenFound!");
                    tokenFound = 1;
                    break;
                }
                else
                {
                    /*check the alias if its a table row*/
                    if(nextNode->parent->type == RBUS_ELEMENT_TYPE_TABLE)
                    {
                        if(nextNode->alias)
                        {
                            size_t tlen = strlen(token);
                            if(tlen > 2 && token[0] == '[' && token[tlen-1] == ']')
                            {
                                if(strlen(nextNode->alias) == tlen-2 && strncmp(nextNode->alias, token+1, tlen-2) == 0)
                                {
                                    RBUSLOG_DEBUG("tokenFound by alias %s!", nextNode->alias);
                                    tokenFound = 1;
                                    break;
                                }
                            }
                        }
                    }

                    currentNode = nextNode;
                    nextNode = currentNode->nextSibling;
                }
            }
        }

        token = strtok_r(NULL, ".", &saveptr);

        if (tokenFound)
        {
            if (token && nextNode->type == RBUS_ELEMENT_TYPE_TABLE && (syncTables && nextNode->cbTable.tableSyncHandler))
            {
                ELM_PRIVATE_LOCK(nextNode);
                nextNode->cbTable.tableSyncHandler(handle, nextNode->fullName);
                ELM_PRIVATE_UNLOCK(nextNode);
            }
            currentNode = nextNode;
            nextNode = currentNode->child;
        }

        if(token && nextNode && nextNode->parent && nextNode->parent->type == RBUS_ELEMENT_TYPE_TABLE) 
        {
            if(!isWildcard && !strcmp(token,"*"))
                isWildcard = true;
            
            /* retrieveInstanceElement should return only the registration element if the table has a getHandler installed (used by MtaAgent/TR104)
                of if wildcard query */
            if(isWildcard || nextNode->parent->cbTable.getHandler)
            {
                token = "{i}";
            }
        }        
    }

    free(name);
    UNLOCK();

    if(tokenFound)
    {
        RBUSLOG_DEBUG("Found Element with param name [%s]", currentNode->name);
        return currentNode;
    }
    else
    {
        return NULL;
    }
}

static void removeElementInternal(elementNode* rowNode, elementNode** chain, int numChain)
{
    VERIFY_NULL(rowNode);
    elementNode* currentNode = rowNode;
    int i = 0;
#if DEBUG_ELEMENTS
    printf("\n\nremoveElementInternal %s\n", rowNode->fullName);
    for(i = 0; i < numChain; ++i){
        VERIFY_NULL(chain[i]);
        printf( "Chain %d: %s\n", i, chain[i]->name );
    }
    i = 0; 
#endif
    while(currentNode && i < numChain)
    {
        elementNode* chainNode = chain[i];
        elementNode* childNode = NULL;

        /*if node is table then recurse into all row instances*/
        if(currentNode && currentNode->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            /* if removing template, then remove all rows first */
            if(strcmp(chainNode->name, "{i}") == 0)
            {
                childNode = currentNode->child;
                while(childNode)
                {
                    elementNode* nextNode = childNode->nextSibling;

                    if(strcmp(childNode->name, "{i}") != 0)
                    {
                        if(numChain-i-1 > 0)
                        {
                            removeElementInternal(childNode, &chain[i+1], numChain-i-1);
                        }
                        else
                        {
                            freeElementNode(childNode);
                        }                            
                    }
                    childNode = nextNode;
                }
            }

            /* remove the matching node (either the template or a specific row (if not template)*/
            childNode = currentNode->child;
            
            while(childNode)
            {
                if(strcmp(childNode->name, chainNode->name) == 0)
                {
                    if(numChain-i-1 > 0)
                    {
                        removeElementInternal(childNode, &chain[i+1], numChain-i-1);
                    }
                    else
                    {
                        freeElementNode(childNode);
                    }
                    break;
                }
                childNode = childNode->nextSibling;
            }
            break;
        }
        else
        {
            /*search for node in children*/
            elementNode* childNode = currentNode->child;

            while(childNode)
            {
                if(strcmp(childNode->name, chainNode->name) == 0)
                {
                    if(i == numChain-1)
                    {
                        freeElementNode(childNode);
                        return;
                    }
                    else
                    {
                        /*go deeper*/
                        currentNode = childNode;
                        i++;
                    }
                    break;
                }
                childNode = childNode->nextSibling;
            }

            if(!childNode)
            {
                RBUSLOG_INFO("Couldn't find node %s\n", chainNode->fullName);
                return;
            }
        }
    }
}

void removeElement(elementNode* element)
{
    elementNode** chain = NULL;
    int numChain = 0;
    VERIFY_NULL(element);
    RBUSLOG_DEBUG("removeElement %s\n", element->fullName);
    LOCK();
    createElementChain(element, &chain, &numChain);
    if(numChain > 1)
        removeElementInternal(chain[0], &chain[1], numChain-1);
    else
        freeElementNode(chain[0]);
    free(chain);
    pruneTree();    
    UNLOCK();
}

static void printElement(elementNode* node, int level)
{
    RBUSLOG_INFO("%*s[name:%s type:%s fullName:%s addr:%p, parent:%p %s%s]", 
        level*2, level ? " " : "",
        node->name, 
        getTypeString(node->type),
        node->fullName,
        node,
        node->parent,
        node->alias ? "alias:" : "",
        node->alias ? node->alias : "");
}

void printRegisteredElements(elementNode* root, int level)
{
    elementNode* child = root;
    elementNode* sibling = NULL;

    if(child)
    {
        printElement(child, level);
        if(child->child)
        {
            printRegisteredElements(child->child, level+1);
        }
        sibling = child->nextSibling;
        while(sibling)
        {
            printElement(sibling, level);
            if(sibling->child)
            {
                printRegisteredElements(sibling->child, level+1);
            }
            sibling = sibling->nextSibling;
        }
    }
}

static void fprintElement(FILE* f, elementNode* node, int level)
{
    /*this format is used by unit test so don't change w/o updating rbusTestElementTree.c*/
    fprintf(f, "%*s%s:%s %s %s%s\n", 
        level*2, level ? " " : "",
        node->name, 
        getTypeString(node->type),
        node->fullName,
        node->alias ? "alias:" : "",
        node->alias ? node->alias : "");
}

void fprintRegisteredElements(FILE* f, elementNode* root, int level)
{
    VERIFY_NULL(f);
    elementNode* child = root;
    elementNode* sibling = NULL;
    VERIFY_NULL(child);

    if(child)
    {
        fprintElement(f, child, level);
        if(child->child)
        {
            fprintRegisteredElements(f, child->child, level+1);
        }
        sibling = child->nextSibling;
        while(sibling)
        {
            fprintElement(f, sibling, level);
            if(sibling->child)
            {
                fprintRegisteredElements(f, sibling->child, level+1);
            }
            sibling = sibling->nextSibling;
        }
    }
}

void addElementSubscription(elementNode* node, rbusSubscription_t* sub, bool checkIfExists)
{
    VERIFY_NULL(node);
    if(checkIfExists && node->subscriptions)
    {
        rtListItem item;
        rbusSubscription_t* data;

        if(node->subscriptions)
        {
            rtList_GetFront(node->subscriptions, &item);

            while(item)
            {
                rtListItem_GetData(item, (void**)&data);
                if(data == sub)
                {
                    /*already exists so return*/
                    return;
                }
                rtListItem_GetNext(item, &item);
            }
        }
    }

    if(!node->subscriptions)
    {
        rtList_Create(&node->subscriptions);
    }

    rtList_PushBack(node->subscriptions, sub, NULL);
}

void removeElementSubscription(elementNode* node, rbusSubscription_t* sub)
{
    rtListItem item;
    rbusSubscription_t* data;
    size_t size = 0;

    VERIFY_NULL(node);
    if(node->subscriptions)
    {
        rtList_GetFront(node->subscriptions, &item);

        while(item)
        {
            rtListItem_GetData(item, (void**)&data);
            if(data == sub)
            {
                rtList_RemoveItem(node->subscriptions, item, NULL);
                rtList_GetSize(node->subscriptions, &size);
                if(size == 0)
                {
                    rtList_Destroy(node->subscriptions, NULL);
                    node->subscriptions = NULL;
                }
                return;
            }
            rtListItem_GetNext(item, &item);
        }
    }
}

bool elementHasAutoPubSubscriptions(elementNode* node, rbusSubscription_t* excluding)
{
    if(!node)
        return false;
    if(node->subscriptions)
    {
        rtListItem item;
        rbusSubscription_t* sub;

        rtList_GetFront(node->subscriptions, &item);

        while(item)
        {
            rtListItem_GetData(item, (void**)&sub);
            if(!sub)
                return false;
            if(sub->autoPublish)
            {
                if((excluding != sub) && (!sub->interval))
                {
                    return true;
                }
            }

            rtListItem_GetNext(item, &item);
        }
    }
    return false;
}

/*
    Example tree:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength

    duplicateNode(
        sourceNode=Device.WiFi.AccessPoint.{i}, 
        parentNode=Device.WiFi.AccessPoint, 
        name="1")

    Result:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength
    Device.WiFi.AccessPoint.1.Prop1
    Device.WiFi.AccessPoint.1.OtherObject.Property2
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.SignalStrength

 */
static elementNode* duplicateNode(elementNode* sourceNode, elementNode* parentNode, char const* name )
{
    elementNode* node;
    elementNode* child;
    char fullName[RBUS_MAX_NAME_LENGTH];
    node = getEmptyElementNode();

    snprintf(fullName, RBUS_MAX_NAME_LENGTH, "%s.%s", parentNode->fullName, name);
    node->fullName = strdup(fullName);
    node->name = strdup(name);
    node->type = sourceNode->type;
    node->cbTable = sourceNode->cbTable;
    node->parent = parentNode;

    /*add new node to the parent's child list*/
    if(parentNode->child)
    {
        child = parentNode->child;
        while(child->nextSibling)
            child = child->nextSibling;
        child->nextSibling = node;
    }
    else
    {
        parentNode->child = node;
    }

    /*duplicate children of sourceNode*/
    child = sourceNode->child;
    while(child)
    {
        duplicateNode(child, node, child->name);
        child = child->nextSibling;
    }

    return node;
}

/*
    Lets say we have this example registered in tree:

    Device.WiFi.AccessPoint.{i}.
    Device.WiFi.AccessPoint.{i}.Prop1
    Device.WiFi.AccessPoint.{i}.OtherObject.Property2
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.{i}.AssociatedDevice.{i}.SignalStrength

    Now a table row with instNum 1 is created under this table:
        Device.WiFi.AccessPoint.{i}.

    We need to create the following in tree:

    Device.WiFi.AccessPoint.1.
    Device.WiFi.AccessPoint.1.Prop1
    Device.WiFi.AccessPoint.1.OtherObject.Property2
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.
    Device.WiFi.AccessPoint.1.AssociatedDevice.{i}.SignalStrength

    Steps:
        Create node Device.WiFi.AccessPoint.1.
        Duplicate the entire node tree under Device.WiFi.AccessPoint.{i}.
        Set the parent of the duplicated node tree to Device.WiFi.AccessPoint.1.

    @param tableNode        The node with name {i} of type RBUS_ELEMENT_TYPE_TABLE
    @param instNum          The new row's instance number
    @param alias            The new row's instance alias (Optional)
*/

elementNode* instantiateTableRow(elementNode* tableNode, uint32_t instNum, char const* alias)
{
    elementNode* rowTemplate;
    char name[32];
    if(!tableNode)
        return NULL;

    LOCK();
#if DEBUG_ELEMENTS
    RBUSLOG_INFO("table=%s instNum=%u alias=%s", tableNode->fullName, instNum, alias);
    printElement(tableNode, 0);
#endif

#if DEBUG_ELEMENTS
    {
        elementNode* root = tableNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### BEFORE INSTANTION ###################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#########################################################");
    }
#endif

    /*find the row template which has name="{i}"*/

    rowTemplate = tableNode->child;
    while(rowTemplate)
    {
        if(strcmp(rowTemplate->name, "{i}") == 0)
            break;
        rowTemplate = rowTemplate->nextSibling;
    }

    if(!rowTemplate)
    {
	UNLOCK();
        assert(false);
        RBUSLOG_ERROR("ERROR: row template not found for table %s", tableNode->fullName);
        return NULL;
    }

    snprintf(name, 32, "%u", instNum);

    elementNode* row = duplicateNode(rowTemplate, tableNode, name);

    if(alias)
    {
        row->alias = strdup(alias);
    }

#if DEBUG_ELEMENTS
    {
        elementNode* root = tableNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### AFTER INSTANTION ####################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#########################################################");
    }
#endif
    UNLOCK();

    return row;
}

void deleteTableRow(elementNode* rowNode)
{
    VERIFY_NULL(rowNode);
    LOCK();
    elementNode* parent = rowNode->parent;
    
    if(!parent)
    {
        RBUSLOG_INFO("row doesn't have a parent");
    }
#if DEBUG_ELEMENTS
    RBUSLOG_INFO("%s: row=%s", __FUNCTION__, rowNode->fullName);
    printElement(rowNode, 0);
    printElement(rowNode->parent, 0);
    {
        elementNode* root = rowNode;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### BEFORE DELETE ###################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#####################################################");
    }
#endif
    freeElementNode(rowNode);
#if DEBUG_ELEMENTS
    if(parent)
    {
        elementNode* root = parent;
        while(root->parent)
            root = root->parent;
        RBUSLOG_INFO("################### AFTER DELETE ####################");
        printRegisteredElements(root, 0);
        RBUSLOG_INFO("#####################################################");
    }
#endif
    UNLOCK();
}

void replicateAcrossTableRowInstancesInternal(elementNode* rowNode, elementNode* chain[], int numChain)
{
    elementNode* currentNode;
    int i;

#if DEBUG_ELEMENTS
    printf("replicateAcrossTableRowInstancesInternal %s\n", rowNode->fullName);
    for(i = 0; i < numChain; ++i)
	VERIFY_NULL(chain[i]);
        printf( "Chain %d: %s\n", i, chain[i]->name );
#endif
    i = 0;    
    currentNode = rowNode;

    while(currentNode && i < numChain)
    {
		VERIFY_NULL(chain[i]);
        /*if node is table then recurse into all row instances*/
        if(currentNode && currentNode->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            elementNode* childNode = currentNode->child;

            while(childNode)
            {
                //replicate for instance and template (this is internal table)
                replicateAcrossTableRowInstancesInternal(childNode, &chain[i+1], numChain-i-1);
                childNode = childNode->nextSibling;
            }

            break;
        }
        else
        {
            /*search for node in children*/
            elementNode* childNode = currentNode->child;

            while(childNode)
            {
                if(strcmp(childNode->name, chain[i]->name) == 0)
                {
                    break;
                }
                else
                {
                    childNode = childNode->nextSibling;
                }
            }

            if(childNode)
            {
                /*go deeper*/
                currentNode = childNode;
                i++;
            }
            else
            {
                duplicateNode(chain[i], currentNode, chain[i]->name);
                break;
            }
        }
    }
}

void replicateAcrossTableRowInstances(elementNode* newNode)
{
    elementNode** chain = NULL;
    int numChain = 0;
    int i = 0;

    LOCK();
    createElementChain(newNode, &chain, &numChain);

    for(i = 0; i < numChain; ++i)
    {
        elementNode* node = chain[i];

        if(node->parent && node->parent->type == RBUS_ELEMENT_TYPE_TABLE)
        {
            elementNode* childNode = node->parent->child;

            while(childNode)
            {
                /*if row*/
                if(childNode->type == 0 && strcmp(childNode->name, "{i}") != 0)
                {
                    replicateAcrossTableRowInstancesInternal(childNode, &chain[i+1], numChain-i-1);
                }

                childNode = childNode->nextSibling;
            }

            break;
        }
    }
    free(chain);
    UNLOCK();
}

void setPropertyChangeComponent(elementNode* node, char const* componentName)
{
    VERIFY_NULL(node);
    if(node->changeComp)
        free(node->changeComp);
    if(componentName)
        node->changeComp = strdup(componentName);
    rtTime_Now(&node->changeTime);

}

void rbusElement_mutex_destroy(void)
{
    if(mutex_init)
    {
      ERROR_CHECK(pthread_mutex_destroy(&element_mutex));
      mutex_init = 0;
    }
}
