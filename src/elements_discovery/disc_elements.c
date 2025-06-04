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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "rbus.h"
#include "rbuscore.h"
#include "rtLog.h"
#include "rtMemory.h"
#include "rtMessage.h"
#include <cjson/cJSON.h>
rbusHandle_t   g_busHandle = 0;

typedef struct MethodData
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
}MethodData;

static void* asyncMethodFunc(void *p)
{
    printf("%s enter\n", __FUNCTION__);
    bool firstLevelOnly, returnTables = false, returnProperties, returnMethods, returnEvents;
    MethodData* data;
    data = p;
    rtMessage input_args_msg  = NULL;
    rtMessage discoverSupportedDMResponse_msg  = NULL;
    rtMessage supportedDM_msg = NULL;

    rtMessage_Create(&discoverSupportedDMResponse_msg);
    rtMessage_Create(&input_args_msg);
    rtMessage_Create(&supportedDM_msg);

    rbusObject_fwrite(data->inParams, 1, stdout);
    const char* topLevelPath = rbusValue_GetString(rbusObject_GetValue(data->inParams, "topLevelPath"), NULL);
    firstLevelOnly = rbusValue_GetBoolean(rbusObject_GetValue(data->inParams, "firstLevelOnly"));
    //returnTables = rbusValue_GetBoolean(rbusObject_GetValue(data->inParams, "returnTables"));
    returnProperties = rbusValue_GetBoolean(rbusObject_GetValue(data->inParams, "returnProperties"));
    returnMethods =  rbusValue_GetBoolean(rbusObject_GetValue(data->inParams, "returnMethods"));
    returnEvents = rbusValue_GetBoolean(rbusObject_GetValue(data->inParams, "returnEvents"));

    printf("topLevelPath: %s firstLevelOnly: %d returnTables: %d returnProperties:%d  returnMethods:%d returnEvents: %d\n", topLevelPath, firstLevelOnly, returnTables, returnProperties, returnMethods, returnEvents);

    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int componentCnt = 0;
    char **pComponentNames;
    int err = rbus_discoverRegisteredComponents(&componentCnt, &pComponentNames);
    if(RBUSCORE_SUCCESS == err)
    {
        int i;
        int allElementCount = 0;
        for (i = 0; i < componentCnt; i++)
        {
           int numElements = 0;
           char** pElementNames;
           if (strstr(pComponentNames[i], "INBOX"))
               continue;
            rc = rbus_discoverComponentDataElements (g_busHandle, pComponentNames[i], false, &numElements, &pElementNames);
            if(numElements)
            {
                for (int j = 0; j < numElements; j++)
                {
                    if (strcmp(pComponentNames[i], pElementNames[j]) == 0)
                        continue;
                    bool nextLevel = false;
                    rbusElementInfo_t* elems = NULL;

                    rc = rbusElementInfo_get(g_busHandle, pElementNames[j], nextLevel ? -1 : RBUS_MAX_NAME_DEPTH, &elems);

                    if(RBUS_ERROR_SUCCESS == rc)
                    {
                        if(elems)
                        {
                            rbusElementInfo_t* elem;
                            char const* component;
                            elem = elems;
                            component = NULL;
                            while(elem)
                            {
                                rtMessage access = NULL;
                                rtMessage_Create(&access);
                                allElementCount++;;
                                if(component == NULL || strcmp(component, elem->component) != 0)
                                {
                                    //printf("\n\rComponent %s:\n\r", elem->component);
                                    component = elem->component;
                                }

                                rtMessage element_msg = NULL;
                                rtMessage_Create(&element_msg);
                                rtMessage_SetString(element_msg, "name", elem->name);
                                if ((elem->type == RBUS_ELEMENT_TYPE_PROPERTY) && returnProperties)
                                {
                                    if (elem->access & RBUS_ACCESS_SET)
                                        rtMessage_AddString(element_msg, "access", "set");
                                    if (elem->access & RBUS_ACCESS_GET)
                                        rtMessage_AddString(element_msg, "access", "get");
                                    if (elem->access & RBUS_ACCESS_SUBSCRIBE)
                                        rtMessage_AddString(element_msg, "access", "subscribe");
                                    rtMessage_AddMessage(supportedDM_msg, "properties", element_msg);

                                }
                                else if ((elem->type == RBUS_ELEMENT_TYPE_TABLE) && returnTables)
                                {
                                    if(elem->access & RBUS_ACCESS_ADDROW)
                                        rtMessage_AddString(element_msg, "access", "Writable");
                                    rtMessage_AddMessage(supportedDM_msg, "table", element_msg);
                                }
                                else if ((elem->type == RBUS_ELEMENT_TYPE_EVENT) && returnEvents)
                                {
                                    rtMessage_AddMessage(supportedDM_msg, "events", element_msg);
                                }
                                else if ((elem->type == RBUS_ELEMENT_TYPE_METHOD) && returnMethods)
                                {
                                    rtMessage_AddMessage(supportedDM_msg, "methods", element_msg);
                                }
                                else
                                {
                                    rtMessage_AddMessage(supportedDM_msg, "object", element_msg);
                                }

                                rtMessage_Release(element_msg);
                                rtMessage_Release(access);
                                elem = elem->next;
                            }
                            rbusElementInfo_free(g_busHandle, elems);
                        }
                        else
                        {
                            printf ("No results returned \n\r");
                        }
                    }
                    else
                    {
                        printf ("Failed to get the data. Error : %d\n\r",rc);
                    }

                    free(pElementNames[j]);
                }
                free(pElementNames);
            }
            else
            {
               printf("No elements discovered!\n\r");
            }
        }
        free(pComponentNames);
    }
    else
    {
        printf ("Failed to discover components. Error Code = %d\n\r", rc);
    }

    rtMessage_SetMessage(discoverSupportedDMResponse_msg,  "Inputargs", input_args_msg);
    rtMessage_SetMessage(discoverSupportedDMResponse_msg,  "supportedDM", supportedDM_msg);
    char *response = NULL;
    uint32_t len = 0;
    rtMessage_ToString(discoverSupportedDMResponse_msg, &response, &len);
    rbusObject_t outParams;
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, response);
    rbusObject_Init(&outParams, NULL);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);
    rtMessage_Release(supportedDM_msg);
    rtMessage_Release(input_args_msg);
    rtMessage_Release(discoverSupportedDMResponse_msg);

    printf("%s sending response\n", __FUNCTION__);
    err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_SUCCESS, outParams);
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


/* Asyn MethodHandler */
static rbusError_t elements_discovery_methodHandler(rbusHandle_t handle,
        char const* methodName, rbusObject_t inParams,
        rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;
    (void)methodName;
    (void)inParams;
    (void)outParams;
    pthread_t pid;
    MethodData* data = rt_malloc(sizeof(MethodData));
    data->asyncHandle = asyncHandle;


    data->inParams = inParams;

    rbusObject_Retain(inParams);

    if(pthread_create(&pid, NULL, asyncMethodFunc, data) || pthread_detach(pid))
    {
        printf("%s failed to spawn thread\n", __FUNCTION__);
        return RBUS_ERROR_BUS_ERROR;
    }

    return RBUS_ERROR_ASYNC_RESPONSE;

    return RBUS_ERROR_SUCCESS;
}

/*Signal handler for closing broker connection*/
static void handle_signal(int sig)
{
    (void) sig;
    if (g_busHandle)
    {
        rbus_close(g_busHandle);
        g_busHandle = 0;
    }
    printf("rbus elements discovery exiting.\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    printf("rbus elements discovery launching.\n");
    rtLog_SetLevel(RT_LOG_INFO);
    int rc = RBUS_ERROR_SUCCESS;
    char componentName[] = "rbus_elements_discovery";
    if (argc == 2)
    {
        if (-1 == daemon(0 /*chdir to "/"*/, 1 /*redirect stdout/stderr to /dev/null*/ ))
        {
            rtLog_Fatal("failed to fork off daemon. %s", rtStrError(errno));
            exit(1);
        }
        signal(SIGTERM, handle_signal);
    }

    rbusDataElement_t dataElements[1] = {
        {"Device.X_RDK-SupportedDM.DiscoverSupportedDM", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, elements_discovery_methodHandler}}
    };

    while(1)
    {
        if ((g_busHandle == NULL) && ((rc = rbus_open(&g_busHandle, componentName) == RBUS_ERROR_SUCCESS)))
        {
            printf("Successfully connected to bus.\n");
            break;
        }
        else
        {
            printf("Error in connecting to the bus.\n");
            sleep(10);
        }
    }

    if ((rc = rbus_regDataElements(g_busHandle, 1, dataElements)) == RBUS_ERROR_SUCCESS)
        printf("Successfully registered Event.\n");

    pause();

    rbus_unregDataElements(g_busHandle, 1, dataElements);
    rbus_close(g_busHandle);
    g_busHandle = NULL;

    printf("rbus elements discovery exiting.\n");
    return 0;
}