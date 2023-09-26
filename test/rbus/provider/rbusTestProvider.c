/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <rbus.h>
#include <rtList.h>
#include <rtLog.h>
#include <rtMemory.h>
#include "../common/runningParamHelper.h"
#include "../common/testValueHelper.h"


char componentName[RBUS_MAX_NAME_LENGTH] = "TestProvider";

/*value tests*/
TestValueProperty* gTestValues;
rbusValue_t gBigString = NULL;
rbusValue_t gBigBytes = NULL;
int32_t gByValue = 0;
bool gEventTableSub[10] = {false};
int gEnableNoAutoPubEvent = 0;
/*
 * Generic Data Model Tree Structure
 */
#define MAX_ALIAS_LEN 64
#define RBUS_ELEMENT_TYPE_OBJECT 1000
#define RBUS_ELEMENT_TYPE_TABLE_ROW 1001

char* getName(const char* format)
{
    static char buffer[RBUS_MAX_NAME_LENGTH];
    snprintf(buffer, RBUS_MAX_NAME_LENGTH, format, componentName);
    return buffer;
}

struct Node;
struct TableNode;
struct TableRowNode;
struct PropertyNode;

typedef rbusError_t (*addTableRowHandler_t)(struct TableNode* tableNode, char const* alias, uint32_t* instNum);
typedef rbusError_t (*removeTableRowHandler_t)(struct TableRowNode* rowNode);

typedef struct Node
{
    rbusElementType_t type;
    char name[MAX_ALIAS_LEN];
    struct Node* parent;
    rtList children;
} Node;

typedef struct TableNode
{
    struct Node node;
    addTableRowHandler_t addHandler;
    removeTableRowHandler_t removeHandler;
    uint32_t instNum;
} TableNode;

typedef struct TableRowNode
{
    struct Node node;
    uint32_t instNum;
    /*row alias stored in Node.name*/
} TableRowNode;

typedef struct PropertyNode
{
    struct Node node;
    rbusProperty_t prop;
} PropertyNode;

Node* getNode(Node* startNode, char const* fullName)
{
    char* tokName;
    char* token;
    char* saveptr = NULL;
    Node* node = NULL;

    tokName = strdup(fullName);
    token = strtok_r(tokName, ".", &saveptr);

    while(token)
    {
        if(!node)
        {
            if(strcmp(startNode->name, token)==0)
                node = startNode;
        }
        else
        {
            Node* nextNode = NULL;
            Node* child;
            rtListItem item;
            void* data;

            rtList_GetFront(node->children, &item);

            while(item)
            {
                rtListItem_GetData(item, &data);

                child = (Node*)data;

                if(child->type == RBUS_ELEMENT_TYPE_TABLE_ROW)
                {
                    /*check if row alias matches
                      alias is stored in the name*/
                    if(strlen(child->name) > 0)
                    {
                        char alias[MAX_ALIAS_LEN+2];

                        /*tr-069: the alias must be wrapped in square brackets*/
                        snprintf(alias, MAX_ALIAS_LEN+2, "[%s]", child->name);

                        if(strcmp(alias, token)==0)
                        {
                            nextNode = child;
                        }
                    }

                    /*check if row instance number matches*/
                    if(!nextNode)
                    {
                        char number[MAX_ALIAS_LEN];

                        snprintf(number, MAX_ALIAS_LEN, "%d", ((TableRowNode*)child)->instNum);

                        if(strcmp(number, token)==0)
                        {
                            nextNode = child;
                        }
                    }
                }
                else if(strcmp(child->name, token)==0)
                {
                    nextNode = child;
                }

                if(nextNode)
                    break;

                rtListItem_GetNext(item, &item);
            }

            if(nextNode)
            {
                node = nextNode;
            }
            else
            {
                free(tokName);
                return NULL;
            }
        }
        token = strtok_r(NULL, ".", &saveptr);
    }
    free(tokName);
    return node;    
}

void destroyNode(Node* node)
{
    rtListItem item;
    void* data;

    /*destroy all children*/    
    rtList_GetFront(node->children, &item);

    while(item)
    {
        rtListItem_GetData(item, &data);
        rtListItem_GetNext(item, &item);

        destroyNode(data);
    }
    rtList_Destroy(node->children, NULL);

    /*release property*/
    if(node->type == RBUS_ELEMENT_TYPE_PROPERTY)
    {
        rbusProperty_Release(((PropertyNode*)node)->prop);
    }

    /*remove node from parent children list*/
    if(node->parent)
    {
        rtList_GetFront(node->parent->children, &item);

        /*remove row from table*/
        while(item)
        {
            rtListItem_GetData(item, &data);

            if(data == node)
            {
                rtList_RemoveItem(node->parent->children, item, NULL);
                break;
            }

            rtListItem_GetNext(item, &item);
        }
    }

    /*and of course*/
    free(node);
}

rbusError_t defaultRemoveTableRowHandler(TableRowNode* rowNode)
{
    destroyNode((Node*)rowNode);
    return RBUS_ERROR_SUCCESS;
}

Node* createNode(Node* parent, rbusElementType_t type, char const* name)
{
    Node* node;
    size_t nodeSize;

    switch((int)type)
    {
        case RBUS_ELEMENT_TYPE_TABLE: 
        nodeSize = sizeof(struct TableNode); 
        break;

        case RBUS_ELEMENT_TYPE_TABLE_ROW:
        nodeSize = sizeof(struct TableRowNode); 
        break;

        case RBUS_ELEMENT_TYPE_PROPERTY:
        nodeSize = sizeof(struct PropertyNode); 
        break;

        default:
        nodeSize = sizeof(struct Node);
        break;
    }

    node = rt_calloc(1, nodeSize);

    node->type = type;

    if(name)
    {
        strncpy(node->name, name, MAX_ALIAS_LEN-1);
    }

    if(parent)
    {
        node->parent = parent;
        rtList_PushBack(parent->children, node, NULL);
    }

    rtList_Create(&node->children);

    return node;
}

TableNode* createTableNode(Node* parent, char const* name, addTableRowHandler_t addHandler)
{
    TableNode* node = (TableNode*)createNode(parent, RBUS_ELEMENT_TYPE_TABLE, name);
    node->addHandler = addHandler;
    node->removeHandler = defaultRemoveTableRowHandler;
    node->instNum = 1;
    return node;
}

rbusError_t createTableRowNode(Node* parent, char const* alias, uint32_t* instNum, TableRowNode** pnode)
{
    *pnode = NULL;

    assert(parent->type == RBUS_ELEMENT_TYPE_TABLE);

    /*verify alias not in use*/
    if(alias && strlen(alias))
    {
        rtListItem item;

        rtList_GetFront(parent->children, &item);

        while(item)
        {
            Node* child;

            rtListItem_GetData(item, (void**)&child);

            assert(child);

            if(child->type == RBUS_ELEMENT_TYPE_TABLE_ROW)
            {
                if(strlen(child->name) && strcmp(child->name, alias)==0)
                {
                    return RBUS_ERROR_ELEMENT_NAME_DUPLICATE;
                }
            }
            rtListItem_GetNext(item, &item);
        }
    }
    
    *pnode = (TableRowNode*)createNode(parent, RBUS_ELEMENT_TYPE_TABLE_ROW, alias);
    
    *instNum = (*pnode)->instNum = ((TableNode*)parent)->instNum++;

    return RBUS_ERROR_SUCCESS;
}

PropertyNode* createPropertyNode(Node* parent, char const* name)
{
    rbusValue_t val;
    PropertyNode* node = (PropertyNode*)createNode(parent, RBUS_ELEMENT_TYPE_PROPERTY, name);
    rbusValue_Init(&val);
    rbusValue_SetString(val, "");
    rbusProperty_Init(&node->prop, NULL, val);
    rbusValue_Release(val);
    return node;
}

/*
 * End Generic Data Model Tree Structure
 */

/*
 * Application Data Model Instantiation
 */
//TODO handle filter matching

int subscribed1 = 0;
int subscribed2 = 0;
int providerNotFoundTest = 0;
int subscribedProviderNotFound = 0;

Node* gRootNode;

rbusError_t addTable3RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

rbusError_t addTable2RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createTableNode((Node*)rowNode, "Table3", addTable3RowHandler);
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

rbusError_t addTable1RowHandler(TableNode* tableNode, char const* alias, uint32_t* instNum)
{
    TableRowNode* rowNode;
    rbusError_t err;

    err = createTableRowNode((Node*)tableNode, alias, instNum, &rowNode);

    if(err != RBUS_ERROR_SUCCESS)
        return err;

    /*add properties and tables to the new row object*/
    createTableNode((Node*)rowNode, "Table2", addTable2RowHandler);
    createPropertyNode((Node*)rowNode, "data");

    return RBUS_ERROR_SUCCESS;
}

void initNodeTree()
{
    gRootNode = createNode(NULL, RBUS_ELEMENT_TYPE_OBJECT, "Device");
    Node* pTestProvider = createNode(gRootNode, RBUS_ELEMENT_TYPE_OBJECT, componentName);
    createTableNode(pTestProvider, "Table1", addTable1RowHandler);
}

Node* getNodeWithTypeVerification(Node* startNode, rbusElementType_t type, char const* name)
{
    Node* node;

    node = getNode(startNode, name);

    if(!node)
    {
        printf("provider: node not found %s\n", name);
        return NULL;
    }

    if(node->type != type)
    {
        printf("provider: node type invalid %s\n", name);
        return NULL;
    }

    return node;
}

rbusError_t tableAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    (void)handle;

    printf("%s %s %s\n", __FUNCTION__, tableName, aliasName);

    TableNode* node = (TableNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_TABLE, tableName);

    if(node)
    {
        return node->addHandler(node, aliasName, instNum);
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t tableRemoveRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
    (void)handle;

    printf("%s %s\n", __FUNCTION__, rowName);

    TableRowNode* node = (TableRowNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_TABLE_ROW, rowName);

    if(node)
    {
        TableNode* table = (TableNode*)node->node.parent;

        return table->removeHandler(node);
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t tableRegAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    (void)handle;
    (void)instNum;

    printf("%s %s %s\n", __FUNCTION__, tableName, aliasName);
    return RBUS_ERROR_INVALID_INPUT;
}

rbusError_t tableRegRemoveRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
    (void)handle;

    printf("%s %s\n", __FUNCTION__, rowName);
    return RBUS_ERROR_INVALID_INPUT;
}

int tableRegSubscribe[2] = {0};
int tableRegComplete[2] = {0};

rbusError_t tableRegSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    *autoPublish = true;

    printf(
        "tableRegSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp(getName("Device.%s.TableReg."), eventName))
    {
        tableRegSubscribe[0] += action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : -1;
    }
    else if(!strcmp(getName("Device.%s.TableReg.1.TableReg."), eventName))
    {
        tableRegSubscribe[1] += action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : -1;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t dataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* propName = rbusProperty_GetName(property);

    printf("%s %s\n", __FUNCTION__, propName);

    PropertyNode* node = (PropertyNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_PROPERTY, propName);

    if(node)
    {
        char* sVal = rbusValue_ToString(rbusProperty_GetValue(node->prop), NULL, 0);
        printf("%s %s: node found with value=%s\n", __FUNCTION__, propName, sVal);
        free(sVal);

        rbusProperty_SetValue(property, rbusProperty_GetValue(node->prop));

        return RBUS_ERROR_SUCCESS;
    }     
    else
    {
        printf("%s %s: node not found\n" , __FUNCTION__, propName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t dataSetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;

    char const* propName = rbusProperty_GetName(property);

    printf("%s %s\n", __FUNCTION__, propName);

    PropertyNode* node = (PropertyNode*)getNodeWithTypeVerification(gRootNode, RBUS_ELEMENT_TYPE_PROPERTY, propName);

    if(node)
    {
        rbusProperty_SetValue(node->prop, rbusProperty_GetValue(property));
        return RBUS_ERROR_SUCCESS;
    }     
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
}

rbusError_t setProviderNotFound(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;
    (void)property;

    printf("starting provider not found test\n");
    providerNotFoundTest = 1;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t resetTablesSetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    printf("%s called for property %s\n", __FUNCTION__, name);

    if(rbusValue_GetBoolean(rbusProperty_GetValue(property)) == true)
    {
        printf("%s resetting table data\n", __FUNCTION__);
        destroyNode(gRootNode);
        initNodeTree();
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;
    *autoPublish = true;

    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp(getName("Device.%s.Event1!"), eventName))
    {
        subscribed1 += action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : -1;
    }
    else if(!strcmp(getName("Device.%s.Event2!"), eventName))
    {
        subscribed2 += action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : -1;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t provideNotFoundSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    printf(
        "provideNotFoundSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp(getName("Device.%s.ProviderNotFoundEvent1!"), eventName))
    {
        subscribedProviderNotFound += action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : -1;
    }
    else
    {
        printf("provider: provideNotFoundSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t provideErrorSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    printf(
        "provideErrorSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);
    
    return RBUS_ERROR_ACCESS_NOT_ALLOWED;
}

rbusError_t getValueHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)opts;

    printf("%s called for property %s\n", __FUNCTION__, name);

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        if(strcmp(name,data->name)==0)
        {
            rbusProperty_SetValue(property, data->values[0]);
            break;
        }
        data++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t setValueHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    printf("%s called for property %s\n", __FUNCTION__, name);

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        if(strcmp(name,data->name)==0)
        {
            rbusValue_SetPointer(&data->values[0], rbusProperty_GetValue(property));
            break;
        }
        data++;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getBigHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)opts;

    printf("%s called for property %s\n", __FUNCTION__, name);

    if(strstr(name, "String") == 0)
    {
        rbusProperty_SetValue(property, gBigString);
    }
    else if(strstr(name, "Bytes") == 0)
    {
        rbusProperty_SetValue(property, gBigBytes);
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t setBigHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)options;

    printf("%s called for property %s\n", __FUNCTION__, name);

    if(strstr(name, "String") == 0)
    {
        rbusValue_Copy(gBigString, rbusProperty_GetValue(property));
    }
    else if(strstr(name, "Bytes") == 0)
    {
        rbusValue_Copy(gBigBytes, rbusProperty_GetValue(property));
    }
    else
    {
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    static uint32_t count = 0;
    (void)handle;
    (void)opts;

    char buff[100];
    snprintf(buff, 100, "value %d", count++/2);//fake a value change every 3rd time the getHandler is called
    printf("Called get handler for [%s=%s]\n", rbusProperty_GetName(property), buff);

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buff);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCIntHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char const* name = rbusProperty_GetName(property);
    int index = atoi(&name[strlen(name)-1]);

    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata[6] = {0,0,0,0,0,0};  /*the actual value to send back*/
    static int32_t mydelta[6] = {1,1,1,1,1,1}; /*how much to change the value by*/
    static int32_t mycount[6] = {0,0,0,0,0,0}; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

    rbusValue_t value;

    mycount[index]++;
    if((mycount[index] % myfreq) == 0) 
    {
        mydata[index] += mydelta[index];
        if(mydata[index] == mymax)
            mydelta[index] = -1;
        else if(mydata[index] == mymin)
            mydelta[index] = 1;
    }

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, mydata[index]);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    printf("getVCIntHandler [%s]=[%d]\n", name, mydata[index]);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCStrHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char const* name = rbusProperty_GetName(property);
    int index = atoi(&name[strlen(name)-1]);

    /*fake a value change every 'myfreq' times this function is called*/
    static int32_t mydata[6] = {0,0,0,0,0,0};  /*the actual value to send back*/
    static int32_t mydelta[6] = {1,1,1,1,1,1}; /*how much to change the value by*/
    static int32_t mycount[6] = {0,0,0,0,0,0}; /*number of times this function called*/
    static int32_t myfreq = 2;  /*number of times this function called before changing value*/
    static int32_t mymin = 0, mymax=5; /*keep value between mymin and mymax*/

    static char* values[6] = {
        "aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff"
    };

    rbusValue_t value;

    mycount[index]++;
    if((mycount[index] % myfreq) == 0) 
    {
        mydata[index] += mydelta[index];
        if(mydata[index] == mymax)
            mydelta[index] = -1;
        else if(mydata[index] == mymin)
            mydelta[index] = 1;
    }

    rbusValue_Init(&value);
    rbusValue_SetString(value, values[mydata[index]]);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);

    printf("getVCStrHandler [%s]=[%s]\n", name, values[mydata[index]]);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getVCByHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetInt32(value, gByValue);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    printf("getVCByHandler [%s]=[%d]\n", rbusProperty_GetName(property), gByValue);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setVCByHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    gByValue = rbusValue_GetInt32(rbusProperty_GetValue(property));
    printf("setVCByHandler [%s]=[%d]\n", rbusProperty_GetName(property), gByValue);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t subAutoPubIntHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)eventName;
    (void)filter;
    (void)interval;
    (void)autoPublish;
    (void)action;
    printf("subAutoPubIntHandler\n");

    *autoPublish = true;
    return RBUS_ERROR_SUCCESS;
}


rbusError_t subNoAutoPubIntHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)eventName;
    (void)filter;
    (void)interval;
    (void)autoPublish;

    printf("subNoAutoPubIntHandler\n");

    *autoPublish = false;
    if(action == RBUS_EVENT_ACTION_SUBSCRIBE)
        gEnableNoAutoPubEvent++;
    else 
        gEnableNoAutoPubEvent--;

    return RBUS_ERROR_SUCCESS;
}



typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
    int err_value;
}MethodData;

static void* asyncMethodFunc(void *p)
{
    MethodData* data;
    rbusObject_t outParams;
    rbusValue_t value;
    rbusError_t err;
    char buff[256];
    char* str;

    printf("%s enter\n", __FUNCTION__);

    sleep(3);

    data = p;

    rbusObject_Init(&outParams, NULL);

    rbusValue_Init(&value);
    if(data->err_value == 0)
    {
        rbusValue_SetString(value, "MethodAsync2()");
    }
    else
    {
        rbusValue_SetString(value, "MethodAsync3()");
    }
    rbusObject_SetValue(outParams, "name", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    str = rbusValue_ToString(rbusObject_GetValue(data->inParams, "param1"), NULL, 0);
    snprintf(buff, 255, "Async Method Response inParams=%s\n", str);
    free(str);
    rbusValue_SetString(value, buff);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);
    
    printf("%s sending response\n", __FUNCTION__);
    if(data->err_value == 0)
    {
	err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_SUCCESS, outParams);
    }
    else
    {
	err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_INVALID_INPUT, outParams);
    }
    if(err != RBUS_ERROR_SUCCESS)
    {
        printf("%s rbusMethod_SendAsyncResponse failed err:%d\n", __FUNCTION__, err);
    }

    rbusObject_Release(data->inParams);
    rbusObject_Release(outParams);

    free(data);

    printf("%s exit\n", __FUNCTION__);

    return NULL;
}

static rbusError_t methodHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;
    rbusValue_t value;

    printf("methodHandler called: %s\n", methodName);
    rbusObject_fwrite(inParams, 1, stdout);
    printf("\n");


    if(strstr(methodName, "Method1()"))
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Method1()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "Method11()"))
    {
        printf("methodHandler failed\n");
        return RBUS_ERROR_INVALID_OPERATION;
    }
    else
    if(strstr(methodName, "Method2()"))
    {
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Method2()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "Method3()"))
    {
        rbusValue_t value1, value2;

        rbusValue_Init(&value1);
        rbusValue_Init(&value2);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "Method3()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        rbusValue_SetInt32(value1, RBUS_ERROR_INVALID_OPERATION);
        rbusValue_SetString(value2, "RBUS_ERROR_INVALID_OPERATION");
        rbusObject_SetValue(outParams, "error_code", value1);
        rbusObject_SetValue(outParams, "error_string", value2);
        printf("methodHandler failed\n");
        rbusValue_Release(value1);
        rbusValue_Release(value2);
        return RBUS_ERROR_INVALID_OPERATION;
    }
    else
    if(strstr(methodName, "MethodAsync1()"))
    {
        sleep(4);
        rbusValue_Init(&value);
        rbusValue_SetString(value, "MethodAsync1()");
        rbusObject_SetValue(outParams, "name", value);
        rbusValue_Release(value);
        printf("methodHandler success\n");
        return RBUS_ERROR_SUCCESS;
    }
    else
    if(strstr(methodName, "MethodAsync2()") || strstr(methodName, "MethodAsync3()"))
    {
        pthread_t pid;
        MethodData* data = rt_malloc(sizeof(MethodData));
        data->asyncHandle = asyncHandle;
        data->inParams = inParams;
	if(strstr(methodName, "MethodAsync2()"))
	{
	    data->err_value = 0;
	}
	else
	{
	    data->err_value = 1;
	}
        rbusObject_Retain(inParams);
        if(pthread_create(&pid, NULL, asyncMethodFunc, data) || pthread_detach(pid))
        {
            printf("%s failed to spawn thread\n", __FUNCTION__);
            return RBUS_ERROR_BUS_ERROR;
        }
        return RBUS_ERROR_ASYNC_RESPONSE;
    }
    printf("methodHandler fail\n");
    return RBUS_ERROR_BUS_ERROR;
}

int ppTableInstNums[3] = {1,1,1};

rbusError_t ppTableAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    (void)handle;
    (void)aliasName;

    if(!strcmp(tableName, getName("Device.%s.PartialPath1")))
    {
        *instNum = ppTableInstNums[0]++;
    }
    else if(!strcmp(tableName, getName("Device.%s.PartialPath1.1.SubTable")))
    {
        *instNum = ppTableInstNums[1]++;
    }
    else if(!strcmp(tableName, getName("Device.%s.PartialPath1.2.SubTable")))
    {
        *instNum = ppTableInstNums[2]++;
    }

    printf("partialPathTableAddRowHandler table=%s instNum=%d\n", tableName, *instNum); 
    return RBUS_ERROR_SUCCESS;
}

rbusError_t ppTableRemRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
    (void)handle;
    (void)rowName;
    return RBUS_ERROR_SUCCESS;
}

/*
    For testing partial path queries we create the following data model:

        "Device.TestProvider.PartialPath1.1.Param1"
        "Device.TestProvider.PartialPath1.1.Param2"
        "Device.TestProvider.PartialPath1.1.SubObject1.Param3"
        "Device.TestProvider.PartialPath1.1.SubObject1.Param4"
        "Device.TestProvider.PartialPath1.1.SubTable.1.Param5"
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param6"
        "Device.TestProvider.PartialPath1.1.SubTable.1.SubObject2.Param7"
        "Device.TestProvider.PartialPath1.2.Param1"
        "Device.TestProvider.PartialPath1.2.Param2"
        "Device.TestProvider.PartialPath1.2.SubObject1.Param3"
        "Device.TestProvider.PartialPath1.2.SubObject1.Param4"
        "Device.TestProvider.PartialPath1.2.SubTable.1.Param5"
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param6"
        "Device.TestProvider.PartialPath1.2.SubTable.1.SubObject2.Param7"
        "Device.TestProvider.PartialPath1.2.SubTable.2.Param5"
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param6"
        "Device.TestProvider.PartialPath1.2.SubTable.2.SubObject2.Param7"
        "Device.TestProvider.PartialPath1.2.SubTable.3.Param5"
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param6"
        "Device.TestProvider.PartialPath1.2.SubTable.3.SubObject2.Param7"

        "Device.TestProvider.PartialPath2.1.Param1"
        "Device.TestProvider.PartialPath2.1.Param2"
        "Device.TestProvider.PartialPath2.1.SubObject1.Param3"
        "Device.TestProvider.PartialPath2.1.SubObject1.Param4"
        "Device.TestProvider.PartialPath2.1.SubTable.1.Param5"
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param6"
        "Device.TestProvider.PartialPath2.1.SubTable.1.SubObject2.Param7"
        "Device.TestProvider.PartialPath2.2.Param1"
        "Device.TestProvider.PartialPath2.2.Param2"
        "Device.TestProvider.PartialPath2.2.SubObject1.Param3"
        "Device.TestProvider.PartialPath2.2.SubObject1.Param4"
        "Device.TestProvider.PartialPath2.2.SubTable.1.Param5"
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param6"
        "Device.TestProvider.PartialPath2.2.SubTable.1.SubObject2.Param7"
        "Device.TestProvider.PartialPath2.2.SubTable.2.Param5"
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param6"
        "Device.TestProvider.PartialPath2.2.SubTable.2.SubObject2.Param7"
        "Device.TestProvider.PartialPath2.2.SubTable.3.Param5"
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param6"
        "Device.TestProvider.PartialPath2.2.SubTable.3.SubObject2.Param7"

    Table rows inside "Device.TestProvider.PartialPath1" are created with rbusTable_registerRow.
    Table rows inside "Device.TestProvider.PartialPath2" are hidden and table level
    getHandlers are used to execute the partial path query.

    We'll set the values for each individual parameter to be the parameter name itself so
    that the consumer can simply verify the value equals the property name.
 */

rbusError_t ppParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    rbusValue_t value;
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    printf(
        "ppParamGetHandler called:\n" \
        "\tproperty=%s\n",
        name);

    if(!strcmp(name, getName("Device.%s.PartialPath1.1.Param1")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.Param2")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.SubObject1.Param3")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.SubObject1.Param4")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.SubTable.1.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.SubTable.1.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.1.SubTable.1.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.Param1")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.Param2")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubObject1.Param3")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubObject1.Param4")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.1.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.1.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.1.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.2.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.2.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.2.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.3.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.3.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath1.2.SubTable.3.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.Param1")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.Param2")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.SubObject1.Param3")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.SubObject1.Param4")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.SubTable.1.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.Param1")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.Param2")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubObject1.Param3")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubObject1.Param4")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.1.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.2.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param7")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.3.Param5")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param6")) ||
       !strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param7")))
    {
        /*set value to the name of the parameter so consumer can easily verify result*/
        rbusValue_Init(&value);
        rbusValue_SetString(value, name);
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
        return RBUS_ERROR_SUCCESS;
    }
    else
    {
        printf("ppParamGetHandler invalid name %s\n", name); 
        return RBUS_ERROR_BUS_ERROR;
    }
}

static void ppAddParam(rbusProperty_t list, const char* name)
{
    rbusProperty_t param;
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, name);
    rbusProperty_Init(&param, name, value);
    rbusProperty_Append(list, param);
    rbusProperty_Release(param);
    rbusValue_Release(value);
}

rbusError_t ppTableGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    printf(
        "ppTableGetHandler called:\n" \
        "\tproperty=%s\n",
        name);

    if(!strcmp(name, getName("Device.%s.PartialPath2.")))
    {
        ppAddParam(property, getName("Device.%s.PartialPath2.1.Param1"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.Param2"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubObject1.Param3"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubObject1.Param4"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param7"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.Param1"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.Param2"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubObject1.Param3"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubObject1.Param4"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param7"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param7"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param7"));
        return RBUS_ERROR_SUCCESS;
    }
    else if(!strcmp(name, getName("Device.%s.PartialPath2.1.SubTable.")))
    {
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.1.SubTable.1.SubObject2.Param7"));
        return RBUS_ERROR_SUCCESS;
    }
    else if(!strcmp(name, getName("Device.%s.PartialPath2.2.SubTable.")))
    {
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.1.SubObject2.Param7"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.2.SubObject2.Param7"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.Param5"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param6"));
        ppAddParam(property, getName("Device.%s.PartialPath2.2.SubTable.3.SubObject2.Param7"));
        return RBUS_ERROR_SUCCESS;
    }
    else
    {
        printf("ppTableGetHandler invalid name %s\n", name); 
        return RBUS_ERROR_BUS_ERROR;
    }
}

int getEventTableInstNum(char const* propName)
{
    int instNum = 0;
    char buff[2] = {0};
    char const* pinstNum = strstr(propName, "EventsTable.") + 12;
    buff[0] = *pinstNum;
    instNum = atoi(buff);
    printf("%s %s instNum=%d\n", __FUNCTION__, propName, instNum);
    return instNum;
}

rbusError_t eventsTablesAddRowHandler(
    rbusHandle_t handle,
    char const* tableName,
    char const* aliasName,
    uint32_t* instNum)
{
    (void)handle;
    (void)tableName;
    (void)aliasName;
    static int instanceNumber = 1;
    *instNum = instanceNumber++;
    printf("%s instNum=%d\n", __FUNCTION__, *instNum); 
    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventsTablesRemRowHandler(
    rbusHandle_t handle,
    char const* rowName)
{
    (void)handle;
    (void)rowName;
    printf("%s %s\n", __FUNCTION__, rowName); 
    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventsTablesPropGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    rbusValue_t value;
    static int instsData[10] = {0};
    int instNum = 0;

    instNum = getEventTableInstNum(rbusProperty_GetName(property));
    if(instNum == 0 || instNum >= 10)
        return RBUS_ERROR_INVALID_INPUT;
    instsData[instNum]++;

    rbusValue_Init(&value);
    rbusValue_SetInt32(value, instsData[instNum]);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventsTablesEventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;
    int instNum = 0;

    *autoPublish = true;
    printf(
        "%s called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        __FUNCTION__,
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);
    
    instNum = getEventTableInstNum(eventName);
    if(instNum == 0 || instNum >= 10)
        return RBUS_ERROR_INVALID_INPUT;

    if(action == RBUS_EVENT_ACTION_SUBSCRIBE)
        gEventTableSub[instNum] = true;
    else
        gEventTableSub[instNum] = false;

    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    int eventCounts[2]={0,0};
    int i, j;
    int loopFor = 0;
    rtLogLevel logLvl = RT_LOG_WARN;

    printf("provider: start\n");

    while (1)
    {
        int option_index = 0;
        int c;

        static struct option long_options[] = 
        {
            {"name",           required_argument,  0, 'n' },
            {"runtime",        required_argument,  0, 't' },
            {"log-level",      required_argument,  0, 'l' },
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "n:t:l:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'n':
            snprintf(componentName, RBUS_MAX_NAME_LENGTH, "%s", optarg);
            printf("componentName: %s\n", componentName);
            break;
        case 't':
            loopFor = atoi(optarg);
            printf("runtime %d seconds\n", loopFor);
            break;
        case 'l':
            logLvl = rtLogLevelFromString(optarg);
            break;
        default:
            break;
        }
    }

    #define numDataElems 58

    rbusDataElement_t dataElement[numDataElems] = {
        {"Device.%s.Event1!", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, eventSubHandler, NULL}},
        {"Device.%s.Event2!", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, eventSubHandler, NULL}},
        {"Device.%s.ErrorSubHandlerEvent!", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, provideErrorSubHandler, NULL}},
        /*testing value-change filter for Int32 and Strings only, for now*/
        {"Device.%s.VCParam", RBUS_ELEMENT_TYPE_PROPERTY, {getVCHandler,NULL,NULL,NULL, subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt0", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt2", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt3", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt4", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamInt5", RBUS_ELEMENT_TYPE_PROPERTY, {getVCIntHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr0", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr1", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr2", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr3", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr4", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamStr5", RBUS_ELEMENT_TYPE_PROPERTY, {getVCStrHandler,NULL,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.VCParamBy",   RBUS_ELEMENT_TYPE_PROPERTY, {getVCByHandler,setVCByHandler,NULL,NULL,subAutoPubIntHandler, NULL}},
        {"Device.%s.NoAutoPubInt", RBUS_ELEMENT_TYPE_PROPERTY, {NULL,NULL,NULL,NULL,subNoAutoPubIntHandler, NULL}},
        {"Device.%s.Table1.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.%s.Table1.{i}.Table2.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.%s.Table1.{i}.Table2.{i}.Table3.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, eventSubHandler, NULL}},
        {"Device.%s.Table1.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, eventSubHandler, NULL}},
        {"Device.%s.Table1.{i}.Table2.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, eventSubHandler, NULL}},
        {"Device.%s.Table1.{i}.Table2.{i}.Table3.{i}.data", RBUS_ELEMENT_TYPE_PROPERTY, {dataGetHandler, dataSetHandler, NULL, NULL, eventSubHandler, NULL}},
        {"Device.%s.TableReg.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableRegAddRowHandler, tableRegRemoveRowHandler, tableRegSubHandler, NULL}},
        {"Device.%s.TableReg.{i}.TableReg.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableRegAddRowHandler, tableRegRemoveRowHandler, tableRegSubHandler, NULL}},
        {"Device.%s.ResetTables", RBUS_ELEMENT_TYPE_PROPERTY, {NULL, resetTablesSetHandler, NULL, NULL, NULL, NULL}},
        {"Device.%s.Method11()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.Method3()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.Table1.{i}.Method1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.Table1.{i}.Table2.{i}.Method2()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.MethodAsync1()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.Table1.{i}.MethodAsync2()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        {"Device.%s.MethodAsync3()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, methodHandler}},
        /*Device.%s.PartialPath1 will have row instances added via rbusTable_registerRow*/
        {"Device.%s.PartialPath1.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubObject1.Param3", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubObject1.Param4", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubTable.{i}.Param5", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubTable.{i}.SubObject2.Param6", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath1.{i}.SubTable.{i}.SubObject2.Param7", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        /*Device.%s.PartialPath2 will not add rows with rbusTable_registerRow but will use table level getHandler execute partial path query*/
        {"Device.%s.PartialPath2.{i}.", RBUS_ELEMENT_TYPE_TABLE, {ppTableGetHandler, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.Param1", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.Param2", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubObject1.Param3", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubObject1.Param4", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {ppTableGetHandler, NULL, ppTableAddRowHandler, ppTableRemRowHandler, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubTable.{i}.Param5", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubTable.{i}.SubObject2.Param6", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.PartialPath2.{i}.SubTable.{i}.SubObject2.Param7", RBUS_ELEMENT_TYPE_PROPERTY, {ppParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.%s.TestProviderNotFound", RBUS_ELEMENT_TYPE_PROPERTY, {NULL,setProviderNotFound,NULL,NULL,NULL, NULL}},
        {"Device.%s.BigString", RBUS_ELEMENT_TYPE_PROPERTY, {getBigHandler,setBigHandler,NULL,NULL,NULL, NULL}},
        {"Device.%s.BigBytes", RBUS_ELEMENT_TYPE_PROPERTY, {getBigHandler,setBigHandler,NULL,NULL,NULL, NULL}},
        {"Device.%s.EventsTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, eventsTablesAddRowHandler, eventsTablesRemRowHandler, NULL, NULL}},
        {"Device.%s.EventsTable.{i}.Prop", RBUS_ELEMENT_TYPE_PROPERTY, {eventsTablesPropGetHandler, NULL, NULL, NULL, eventsTablesEventSubHandler, NULL}},
        {"Device.%s.EventsTable.{i}.Event", RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, eventsTablesEventSubHandler, NULL}}
    };

    for(i=0; i<numDataElems; ++i)
    {
        dataElement[i].name = strdup(getName(dataElement[i].name));
    }

    rtLog_SetLevel(logLvl);

    TestValueProperties_Init(&gTestValues);

    initNodeTree();

    rbusValue_Init(&gBigString);
    rbusValue_Init(&gBigBytes);
    {
        const int BIGSIZE = 1000;
        char* val = rt_malloc(BIGSIZE);
        int i = 0;
        for(i = 0; i < BIGSIZE-1; ++i)
            val[i] = (char)(32 + (i % 96));
        val[BIGSIZE-1]=0;
        rbusValue_SetString(gBigString, val);
        rbusValue_SetBytes(gBigBytes, (uint8_t*)val, BIGSIZE);
        free(val);
    }

    rc = rbus_open(&handle, componentName);
    rtLog_SetLevel(logLvl);
    printf("provider: rbus_open=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit2;

    //rtLog_SetLevel(RT_LOG_DEBUG);

    TestValueProperty* data = gTestValues;
    while(data->name)
    {
        rbusDataElement_t el = { data->name, RBUS_ELEMENT_TYPE_PROPERTY, {getValueHandler,setValueHandler,NULL,NULL,NULL,NULL}};
        rc = rbus_regDataElements(handle, 1, &el);
        printf("provider: rbus_regDataElements=%d\n", rc);
        if(rc != RBUS_ERROR_SUCCESS)
            goto exit1;
        data++;
    }
    
    rc = rbus_regDataElements(handle, numDataElems, dataElement);
    printf("provider: rbus_regDataElements=%d\n", rc);
    if(rc != RBUS_ERROR_SUCCESS)
        goto exit1;

    /*for partial path testing add exactly 2, 1, 3 rows as follows*/
    /*add 2 rows*/
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1"), ppTableInstNums[0]++, NULL);
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1"), ppTableInstNums[0]++, NULL);
    /*add 1 row*/
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1.1.SubTable"), ppTableInstNums[1]++, NULL);
    /*add 3 row2*/
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1.2.SubTable"), ppTableInstNums[2]++, NULL);
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1.2.SubTable"), ppTableInstNums[2]++, NULL);
    rbusTable_registerRow(handle, getName("Device.%s.PartialPath1.2.SubTable"), ppTableInstNums[2]++, NULL);

    if(loopFor == 0)
    {
        if(runningParamProvider_Init(handle, getName("Device.%s.TestRunning")) != RBUS_ERROR_SUCCESS)
        {
            printf("provider: failed to register running param\n");
            goto exit1;
        }

        printf("provider: waiting for consumer to start\n");
        while (!runningParamProvider_IsRunning())
        {
            usleep(10000);
        }
    }

    printf("provider: running\n");
    while (runningParamProvider_IsRunning() || loopFor)
    {

        #define BUFF_LEN 100
        char buffer[BUFF_LEN];

        if(loopFor > 0)
        {
            printf("exiting in %d seconds\n", loopFor);
            loopFor--;
        }

        sleep(1);

        for(j=0; j<2; ++j)
        {
            if((j==0 && subscribed1) || (j==1 && subscribed2))
            {
                printf("publishing Event%d\n", j);

                snprintf(buffer, BUFF_LEN, "event %d data %d", j, eventCounts[j]);

                rbusObject_t data;
                rbusValue_t bufferVal;
                rbusValue_t indexVal;

                rbusValue_Init(&bufferVal);
                rbusValue_Init(&indexVal);

                rbusValue_SetString(bufferVal, buffer);
                rbusValue_SetInt32(indexVal, eventCounts[j]);

                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "buffer", bufferVal);
                rbusObject_SetValue(data, "index", indexVal);

                rbusEvent_t event = {0};
                event.name = dataElement[j].name;
                event.data = data;
                event.type = RBUS_EVENT_GENERAL;

                rc = rbusEvent_Publish(handle, &event);

                rbusValue_Release(bufferVal);
                rbusValue_Release(indexVal);
                rbusObject_Release(data);

                if(rc != RBUS_ERROR_SUCCESS)
                    printf("provider: rbusEvent_Publish Event%d failed: %d\n", j, rc);

                eventCounts[j]++;
            }
        }

        if(providerNotFoundTest != 0)
        {
            if(providerNotFoundTest++ == 20)/*20 seconds after we are told to run this test -- register the data element to let them know provider ready*/
            {
                rbusDataElement_t el = {
                    getName("Device.%s.ProviderNotFoundEvent1!"), RBUS_ELEMENT_TYPE_EVENT, {NULL,NULL,NULL,NULL, provideNotFoundSubHandler, NULL}
                };
                rc = rbus_regDataElements(handle, 1, &el);
            }

            if(subscribedProviderNotFound)
            {
                printf("publishing ProviderNotFoundEvent!\n");

                rbusObject_t data;
                rbusValue_t bufferVal;
                rbusValue_Init(&bufferVal);
                rbusValue_SetString(bufferVal, "provider now up!");
                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "buffer", bufferVal);

                rbusEvent_t event = {0};
                event.name = getName("Device.%s.ProviderNotFoundEvent1!");
                event.data = data;
                event.type = RBUS_EVENT_GENERAL;

                rc = rbusEvent_Publish(handle, &event);

                rbusValue_Release(bufferVal);
                rbusObject_Release(data);

                if(rc != RBUS_ERROR_SUCCESS)
                    printf("provider: rbusEvent_Publish Event ProviderNotFoundEvent1! failed: %d\n", rc);
            }
        }

        if(tableRegSubscribe[0] > 0 && !tableRegComplete[0])
        {
            tableRegComplete[0] = 1;
            rbusTable_registerRow(handle, getName("Device.%s.TableReg."), 1, NULL);
            rbusTable_registerRow(handle, getName("Device.%s.TableReg."), 2, NULL);
        }
        if(tableRegSubscribe[1] > 0 && !tableRegComplete[1])
        {
            tableRegComplete[1] = 1;
            rbusTable_registerRow(handle, getName("Device.%s.TableReg.1.TableReg."), 1, NULL);
            rbusTable_registerRow(handle, getName("Device.%s.TableReg.1.TableReg."), 2, NULL);
        }

        printf("publishing tableEvent!\n");

        for(i = 0; i < 10; ++i)
        {
            if(gEventTableSub[i] == true)
            {
                char buff[100];

                sprintf(buff, "tableEvent data instNum=%d", i);
                rbusObject_t data;
                rbusValue_t val;
                rbusValue_Init(&val);
                rbusValue_SetString(val, buff);
                rbusObject_Init(&data, NULL);
                rbusObject_SetValue(data, "data", val);

                sprintf(buff, "%s.%d.Event", getName("Device.%s.EventsTable"), i); 
                rbusEvent_t event = {0};
                event.name = buff;
                event.data = data;
                event.type = RBUS_EVENT_GENERAL;

                rc = rbusEvent_Publish(handle, &event);

                rbusValue_Release(val);
                rbusObject_Release(data);

                if(rc != RBUS_ERROR_SUCCESS)
                    printf("provider: rbusEvent_Publish %s failed: %d\n", event.name, rc);
            }
        }

        if(gEnableNoAutoPubEvent > 0)
        {
            rbusEvent_t event = {0};
            rbusObject_t dataObj = NULL;
            rbusValue_t newVal = NULL;
            rbusValue_t oldVal = NULL;
            rbusValue_t byVal = NULL;
            
            /*will cycle between 3 and 7*/
            static int32_t val = 3;
            static int32_t dir = 1;

            rbusObject_Init(&dataObj, NULL);
            rbusValue_Init(&newVal);
            rbusValue_Init(&oldVal);
            rbusValue_Init(&byVal);
            
            rbusValue_SetInt32(oldVal, val);

            val += dir;
            if(val == 3)
                dir = 1;
            else if(val == 7)
                dir = -1;

            rbusValue_SetInt32(newVal, val);
            rbusValue_SetString(byVal, componentName);

            rbusObject_SetValue(dataObj, "value", newVal);
            rbusObject_SetValue(dataObj, "oldValue", oldVal);
            rbusObject_SetValue(dataObj, "by", byVal);

            event.name = strdup(getName("Device.%s.NoAutoPubInt"));
            event.data = dataObj;
            event.type = RBUS_EVENT_VALUE_CHANGED;
            rc = rbusEvent_Publish(handle, &event);

            rbusValue_Release(newVal);
            rbusValue_Release(oldVal);
            rbusValue_Release(byVal);
            rbusObject_Release(dataObj);
            free((char*)event.name);
        }
    }

    printf("provider: finishing\n");

    rc = rbus_unregDataElements(handle, numDataElems, dataElement);
    printf("provider: rbus_unregDataElements=%d\n", rc);

exit1:
    rc = rbus_close(handle);
    printf("provider: rbus_close=%d\n", rc);

exit2:

    TestValueProperties_Release(gTestValues);
    destroyNode(gRootNode);

    for(i=0; i<numDataElems; ++i)
    {
        free(dataElement[i].name);
    }

    if(gBigString)
        rbusValue_Release(gBigString);
    if(gBigBytes)
        rbusValue_Release(gBigBytes);

    printf("provider: exit\n");
    return rc;
}
