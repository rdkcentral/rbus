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
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <rbus.h>
#include <libgen.h>

static char g_elementString[64];
static int param_index = 0;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int reg_param(rbusHandle_t handle);

rbusError_t tableRemoveRowHandler(rbusHandle_t handle, char const* rowName)
{
    (void)handle;

    printf(
        "%s called:\n" \
        "\trowName=%s\n", __func__,rowName);

    return RBUS_ERROR_SUCCESS;
}

rbusError_t tableAddRowHandler(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    (void)handle;

    static uint32_t instance = 1;
    *instNum = instance++;

    if(aliasName) {
        printf(
                "%s called:\n"
                "\ttableName=%s\n" \
                "\taliasName=%s\n", __func__, tableName, aliasName );
    } else {
        printf(
                "%s called:\n"
                "\ttableName=%s\n", __func__, tableName );
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* name = rbusProperty_GetName(property);
    rbusValue_t value;

    printf(
            "%s called:\n" \
            "\tproperty=%s\n", __func__, name);

    rbusValue_Init(&value);

    if(strcmp(name,"Device.TestElements") == 0)
        rbusValue_SetString(value, g_elementString);
    else
        rbusValue_SetString(value, name);

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    char const* name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);
    rbusValueType_t type = rbusValue_GetType(value);
    int rc = RBUS_ERROR_INVALID_INPUT;
    int len = 0;
    char const* pTmp = NULL;

    if (type != RBUS_STRING) {
        printf("%s Called Set handler with wrong data type\n", name);
        return rc;
    }

    pTmp = rbusValue_GetString(value, &len);
    if(strcmp(pTmp,"register") == 0) {
        rc = reg_param(handle);
    } else if (strcmp(pTmp,"quit") == 0) {
        rc = RBUS_ERROR_SUCCESS;
        pthread_cond_signal(&cond);
        memset(g_elementString,0,sizeof(g_elementString));
        strncpy(g_elementString,pTmp, len);
    } else {
        printf("%s Called Set handler with invalid value : %s\n", name, pTmp);
    }

    return rc;
}

int reg_param(rbusHandle_t handle)
{
    (void)(handle);
    int rc = RBUS_ERROR_SUCCESS;
    char buf[32] = {0};
    rbusDataElement_t dataElement;

    memset(&dataElement,0,sizeof(dataElement));

    param_index++;
    snprintf(buf,sizeof(buf),"Device.Tables.{i}.Test%hu", (unsigned short)param_index);
    dataElement.name = buf;
    dataElement.type = RBUS_ELEMENT_TYPE_PROPERTY;
    dataElement.cbTable.getHandler = getHandler;

    rc = rbus_regDataElements(handle, 1, &dataElement);
    printf("Register Data Element : %s\n",dataElement.name);
    memset(g_elementString,0,sizeof(g_elementString));
    snprintf(g_elementString,sizeof(g_elementString),"Registered Element - %s",buf);

    return rc;
}

#define dataElementsCount sizeof(dataElements)/sizeof(dataElements[0])
int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;
    char *component_name = basename(argv[0]);
    unsigned int quit_counter = 10;

    static rbusDataElement_t dataElements[] = {
        {"Device.Tables.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler, tableRemoveRowHandler, NULL, NULL}},
        {"Device.Tables.{i}.Data", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, NULL, NULL, NULL, NULL, NULL}},
        {"Device.TestElements", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler, setHandler, NULL, NULL, NULL, NULL}}
    };

    printf("provider: start\n");

    rc = rbus_open(&handle, component_name);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(handle, dataElementsCount, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_regDataElements failed: %d\n", rc);
        goto exit1;
    }

    pthread_cond_wait(&cond, &lock);

    while ((strcmp(g_elementString,"quit") != 0) && (quit_counter--)) {
        usleep(100000);
    }

    rbus_unregDataElements(handle, dataElementsCount, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
