/*
 * Minimal NotifyDML sample provider.
 *
 * Provides:
 * - Table: Device.Test.Table.{i}.
 * - Property: Device.Test.Table.{i}.SignalStrength (int32)
 *
 * It registers the table, then periodically adds/removes rows and changes SignalStrength quickly
 * to exercise batching/coalescing on the consumer side.
 */

#include <rbus.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static int32_t gSignalStrength[16];
static int gNextInst = 1;

static rbusError_t getSignalStrength(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;
    char const* name = rbusProperty_GetName(property);
    /* name: Device.Test.Table.<inst>.SignalStrength */
    char const* p = strstr(name, "Device.Test.Table.");
    if(!p) return RBUS_ERROR_INVALID_INPUT;
    p += strlen("Device.Test.Table.");
    int inst = atoi(p);
    if(inst <= 0 || inst >= (int)(sizeof(gSignalStrength)/sizeof(gSignalStrength[0])))
        return RBUS_ERROR_INVALID_INPUT;
    rbusValue_t v;
    rbusValue_Init(&v);
    rbusValue_SetInt32(v, gSignalStrength[inst]);
    rbusProperty_SetValue(property, v);
    rbusValue_Release(v);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t setSignalStrength(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;
    char const* name = rbusProperty_GetName(property);
    rbusValue_t v = rbusProperty_GetValue(property);
    if(!v) return RBUS_ERROR_INVALID_INPUT;
    char const* p = strstr(name, "Device.Test.Table.");
    if(!p) return RBUS_ERROR_INVALID_INPUT;
    p += strlen("Device.Test.Table.");
    int inst = atoi(p);
    if(inst <= 0 || inst >= (int)(sizeof(gSignalStrength)/sizeof(gSignalStrength[0])))
        return RBUS_ERROR_INVALID_INPUT;
    gSignalStrength[inst] = rbusValue_GetInt32(v);
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t addRow(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    (void)handle;
    (void)tableName;
    (void)aliasName;
    *instNum = (uint32_t)gNextInst++;
    if(*instNum >= 16)
        *instNum = 1;
    gSignalStrength[*instNum] = -50;
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t removeRow(rbusHandle_t handle, char const* rowName)
{
    (void)handle;
    (void)rowName;
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    rbusHandle_t handle;
    if(rbus_open(&handle, "dmlNotifyProvider") != RBUS_ERROR_SUCCESS)
    {
        printf("rbus_open failed\n");
        return 1;
    }

    rbusDataElement_t elems[2];
    memset(elems, 0, sizeof(elems));

    elems[0].name = "Device.Test.Table.{i}.";
    elems[0].type = RBUS_ELEMENT_TYPE_TABLE;
    elems[0].cbTable.tableAddRowHandler = addRow;
    elems[0].cbTable.tableRemoveRowHandler = removeRow;

    elems[1].name = "Device.Test.Table.{i}.SignalStrength";
    elems[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
    elems[1].cbTable.getHandler = getSignalStrength;
    elems[1].cbTable.setHandler = setSignalStrength;

    if(rbus_regDataElements(handle, 2, elems) != RBUS_ERROR_SUCCESS)
    {
        printf("rbus_regDataElements failed\n");
        rbus_close(handle);
        return 2;
    }

    /* Create first row via provider-side registerRow */
    uint32_t inst = 1;
    rbusTable_registerRow(handle, "Device.Test.Table.", inst, NULL);

    while(1)
    {
        /* Burst update SignalStrength for inst=1 */
        for(int i=0;i<20;i++)
        {
            char path[RBUS_MAX_NAME_LENGTH];
            snprintf(path, sizeof(path), "Device.Test.Table.%u.SignalStrength", inst);
            rbus_setInt(handle, path, -30 - i);
            usleep(10 * 1000);
        }

        /* Add and remove another row to exercise create/delete notifications */
        uint32_t inst2 = 0;
        rbusTable_addRow(handle, "Device.Test.Table.", NULL, &inst2);
        sleep(1);
        if(inst2)
        {
            char rowName[RBUS_MAX_NAME_LENGTH];
            snprintf(rowName, sizeof(rowName), "Device.Test.Table.%u", inst2);
            rbusTable_removeRow(handle, rowName);
        }

        sleep(2);
    }
    return 0;
}

