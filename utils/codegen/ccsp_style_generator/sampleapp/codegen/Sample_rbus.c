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

/*
 * 
 * code generated with: python ../../scripts/rbus_code_generator_ccsp_style.py ../Sample_dm.xml Sample
 */

#include <rbus.h>
#include <rbus_context_helpers.h>
#include <rtMemory.h>
#include <rtLog.h>
#include <stdlib.h>
#include <string.h>

rbusHandle_t gBusHandle = NULL;
char const gComponentID[] = "Sample";

rbusError_t registerGeneratedDataElements(rbusHandle_t handle);

rbusError_t Sample_RegisterRow(char const* tableName, uint32_t instNum, char const* alias, void* context)
{
    rbusError_t rc;
    rc = rbusTable_registerRow(gBusHandle, tableName, instNum, alias);
    if(rc == RBUS_ERROR_SUCCESS)
        SetRowContext(tableName, instNum, alias, context);    
    return rc;    
}

rbusError_t Sample_UnregisterRow(char const* tableName, uint32_t instNum)
{
    rbusError_t rc;
    char rowName[RBUS_MAX_NAME_LENGTH];
    snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d", tableName, instNum);
    rc = rbusTable_unregisterRow(gBusHandle, rowName);
    if(rc == RBUS_ERROR_SUCCESS)
        RemoveRowContextByName(rowName);    
    return rc;    
}

rbusError_t Sample_Init()
{
    rbusError_t rc;
    rbusHandle_t rbusHandle = NULL;
    
    rc = rbus_open(&rbusHandle, gComponentID);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        return rc;
    }
    rc = registerGeneratedDataElements(rbusHandle);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rbus_close(rbusHandle);
        return rc;
    }
    Context_Init();
    gBusHandle = rbusHandle;
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_Unload()
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    if(gBusHandle)
    {
        rc = rbus_close(gBusHandle);
        gBusHandle = NULL;
    }
    Context_Release();
    return rc;    
}

static rbusError_t Sample_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        //rbusProperty_SetBoolean(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Sample_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        //rbusProperty_SetInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Sample_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        //rbusProperty_SetUInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Sample_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        //rbusProperty_SetString(property, YOUR_VALUE);
    }
    else if(strcmp(context.name, "ReadonlyStringParam") == 0)
    {
        //rbusProperty_SetString(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_Validate(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_Commit(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_Rollback(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_Sample_Validate_Sample_Commit_Sample_Rollback(void* context)
{
    if(Sample_Validate(context) == 0)
    {
        if(Sample_Commit(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(Sample_Rollback(context) == 0)
            return RBUS_ERROR_INVALID_INPUT;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t Sample_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_Sample_Validate_Sample_Commit_Sample_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_SetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        int32_t val;

        rbusValueError_t verr = rbusProperty_GetInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_Sample_Validate_Sample_Commit_Sample_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_SetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_Sample_Validate_Sample_Commit_Sample_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Sample_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_Sample_Validate_Sample_Commit_Sample_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

void* WritableTable_AddEntry(void* ctx, uint32_t* instNum)
{
    return NULL;
}

static rbusError_t WritableTable_AddEntry_rbus(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    HandlerContext context = GetTableContext(tableName);
    void* rowContext = WritableTable_AddEntry(context.userData, instNum);
    if(!rowContext)
    {
        rtLog_Error("WritableTable_AddEntry returned null row context");
        return RBUS_ERROR_BUS_ERROR;
    }
    SetRowContext(context.fullName, *instNum, aliasName, rowContext);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_DelEntry(void* ctx, void* inst)
{
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t WritableTable_DelEntry_rbus(rbusHandle_t handle, char const* rowName)
{
    HandlerContext context = GetHandlerContext(rowName);  
    void* rowContext = GetRowContext(context.fullName);
    int rc = WritableTable_DelEntry(context.userData, rowContext);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rtLog_Error("WritableTable_DelEntry failed");
        return RBUS_ERROR_BUS_ERROR;
    }
    RemoveRowContextByName(context.fullName);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t WritableTable_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        //rbusProperty_SetBoolean(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t WritableTable_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        //rbusProperty_SetInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t WritableTable_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        //rbusProperty_SetUInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t WritableTable_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        //rbusProperty_SetString(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_Validate(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_Commit(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_Rollback(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_WritableTable_Validate_WritableTable_Commit_WritableTable_Rollback(void* context)
{
    if(WritableTable_Validate(context) == 0)
    {
        if(WritableTable_Commit(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(WritableTable_Rollback(context) == 0)
            return RBUS_ERROR_INVALID_INPUT;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t WritableTable_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_WritableTable_Validate_WritableTable_Commit_WritableTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_SetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        int32_t val;

        rbusValueError_t verr = rbusProperty_GetInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_WritableTable_Validate_WritableTable_Commit_WritableTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_SetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_WritableTable_Validate_WritableTable_Commit_WritableTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t WritableTable_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_WritableTable_Validate_WritableTable_Commit_WritableTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t StaticTable_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        //rbusProperty_SetBoolean(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t StaticTable_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        //rbusProperty_SetInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t StaticTable_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        //rbusProperty_SetUInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t StaticTable_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        //rbusProperty_SetString(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_Validate(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_Commit(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_Rollback(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_StaticTable_Validate_StaticTable_Commit_StaticTable_Rollback(void* context)
{
    if(StaticTable_Validate(context) == 0)
    {
        if(StaticTable_Commit(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(StaticTable_Rollback(context) == 0)
            return RBUS_ERROR_INVALID_INPUT;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t StaticTable_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "BoolParam") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_StaticTable_Validate_StaticTable_Commit_StaticTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_SetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "IntParam") == 0)
    {
        int32_t val;

        rbusValueError_t verr = rbusProperty_GetInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_StaticTable_Validate_StaticTable_Commit_StaticTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_SetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "UlongParam") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_StaticTable_Validate_StaticTable_Commit_StaticTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t StaticTable_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    if(strcmp(context.name, "StringParam") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_StaticTable_Validate_StaticTable_Commit_StaticTable_Rollback(context.userData);
    }

    return RBUS_ERROR_SUCCESS;
}

bool DynamicTable_IsUpdated(void* ctx)
{
    return true;
}

rbusError_t DynamicTable_Synchronize(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

int do_DynamicTable_IsUpdated_DynamicTable_Synchronize(HandlerContext context)
{
    if(IsTimeToSyncDynamicTable(context.name))
    {
        if(DynamicTable_IsUpdated(context.userData))
        {
            return DynamicTable_Synchronize(context.userData);
        }
    }
    return 0;
}

static rbusError_t DynamicTable_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);    
    rbusError_t ret;

    if((ret = do_DynamicTable_IsUpdated_DynamicTable_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    if(strcmp(context.name, "BoolParam") == 0)
    {
        //rbusProperty_SetBoolean(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t DynamicTable_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);    
    rbusError_t ret;

    if((ret = do_DynamicTable_IsUpdated_DynamicTable_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    if(strcmp(context.name, "IntParam") == 0)
    {
        //rbusProperty_SetInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t DynamicTable_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);    
    rbusError_t ret;

    if((ret = do_DynamicTable_IsUpdated_DynamicTable_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    if(strcmp(context.name, "UlongParam") == 0)
    {
        //rbusProperty_SetUInt32(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t DynamicTable_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);    
    rbusError_t ret;

    if((ret = do_DynamicTable_IsUpdated_DynamicTable_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    if(strcmp(context.name, "StringParam") == 0)
    {
        //rbusProperty_SetString(property, YOUR_VALUE);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t registerGeneratedDataElements(rbusHandle_t handle)
{
    rbusError_t rc;
    static rbusDataElement_t dataElements[20] = {
        {"Device.Sample.BoolParam", RBUS_ELEMENT_TYPE_PROPERTY, {Sample_GetParamBoolValue_rbus, Sample_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.IntParam", RBUS_ELEMENT_TYPE_PROPERTY, {Sample_GetParamIntValue_rbus, Sample_SetParamIntValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.UlongParam", RBUS_ELEMENT_TYPE_PROPERTY, {Sample_GetParamUlongValue_rbus, Sample_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StringParam", RBUS_ELEMENT_TYPE_PROPERTY, {Sample_GetParamStringValue_rbus, Sample_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.ReadonlyStringParam", RBUS_ELEMENT_TYPE_PROPERTY, {Sample_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.WritableTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, WritableTable_AddEntry_rbus, WritableTable_DelEntry_rbus, NULL, NULL}},
        {"Device.Sample.WritableTable.{i}.BoolParam", RBUS_ELEMENT_TYPE_PROPERTY, {WritableTable_GetParamBoolValue_rbus, WritableTable_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.WritableTable.{i}.IntParam", RBUS_ELEMENT_TYPE_PROPERTY, {WritableTable_GetParamIntValue_rbus, WritableTable_SetParamIntValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.WritableTable.{i}.UlongParam", RBUS_ELEMENT_TYPE_PROPERTY, {WritableTable_GetParamUlongValue_rbus, WritableTable_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.WritableTable.{i}.StringParam", RBUS_ELEMENT_TYPE_PROPERTY, {WritableTable_GetParamStringValue_rbus, WritableTable_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StaticTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StaticTable.{i}.BoolParam", RBUS_ELEMENT_TYPE_PROPERTY, {StaticTable_GetParamBoolValue_rbus, StaticTable_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StaticTable.{i}.IntParam", RBUS_ELEMENT_TYPE_PROPERTY, {StaticTable_GetParamIntValue_rbus, StaticTable_SetParamIntValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StaticTable.{i}.UlongParam", RBUS_ELEMENT_TYPE_PROPERTY, {StaticTable_GetParamUlongValue_rbus, StaticTable_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.StaticTable.{i}.StringParam", RBUS_ELEMENT_TYPE_PROPERTY, {StaticTable_GetParamStringValue_rbus, StaticTable_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Sample.DynamicTable.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.DynamicTable.{i}.BoolParam", RBUS_ELEMENT_TYPE_PROPERTY, {DynamicTable_GetParamBoolValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.DynamicTable.{i}.IntParam", RBUS_ELEMENT_TYPE_PROPERTY, {DynamicTable_GetParamIntValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.DynamicTable.{i}.UlongParam", RBUS_ELEMENT_TYPE_PROPERTY, {DynamicTable_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Sample.DynamicTable.{i}.StringParam", RBUS_ELEMENT_TYPE_PROPERTY, {DynamicTable_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}}
    };
    rc = rbus_regDataElements(handle, 20, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rtLog_Error("rbus_regDataElements failed");
    }
    return rc;
}
