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

#include <rbus_context_helpers.h>
#include <rbus.h>
#include <rtHashMap.h>
#include <stdlib.h>
#include <string.h>

/*For dynamic tables, the minimum number of seconds that must
  elapse between Synchronization.  This is used to prevent
  these tables from resyncing themselves(rebuild themselves) many times
  during a partial path query, or multi set operation*/
#define DYNAMIC_TABLE_SYNC_CACHE_TIME 5 

typedef struct _ContextMaps
{
    rtHashMap row;
    rtHashMap alias;
    rtHashMap inst;
    rtHashMap dyntime;
} ContextMaps_t;

static ContextMaps_t gContextMaps = {0};
static int gContextRefCount = 0;

char const* ConvertPath(char const* path)
{
    static char buff1[RBUS_MAX_NAME_LENGTH];
    static char buff2[RBUS_MAX_NAME_LENGTH];
    char* buff;
    char const* p1;
    char const* p2;
    char* inst;
    buff1[0] = 0;
    buff2[0] = 0;
    buff = buff1;
    p1 = path;
    for(;;)
    {
        p2 = strchr(p1, ']');
        if(!p2)
            return p1;
        p2++;
        strncpy(buff, p1, p2-p1);
        buff[p2-p1] = 0;
        inst = rtHashMap_Get(gContextMaps.alias, buff);
        if(inst)
        {
            if(!*p2)
            {
                return inst;
            }
            else
            {
                strcpy(buff, inst);
                strcat(buff, p2);
                p1 = buff;
            }
        }
        else
        {
            if(!*p2)
            {
                return p1;
            }
            else
            {
                strcpy(buff, p1);
                strcat(buff, p2);
                p1 = buff;
            }
        }
        buff = buff == buff1 ? buff2 : buff1;
    }
}

void* GetParentContext(char const* path)
{
    void* ctx;
    char parent[RBUS_MAX_NAME_LENGTH];
    strcpy(parent, path);
    char* p = parent + strlen(parent)-1;
    for(;;)
    {
        while(p > parent && *p != '.')
            p--;
        if(p == parent)
            return NULL;
        if(*p == '.')
            *p = 0;
        ctx = rtHashMap_Get(gContextMaps.row, parent);
        if(ctx)
            return ctx;
    }
    return NULL;
}

void* GetRowContext(char const* path)
{
    return rtHashMap_Get(gContextMaps.row, path);
}

void SetRowContext(char const* tableName, uint32_t instNum, char const* alias, void* context)
{
    char path[RBUS_MAX_NAME_LENGTH];
    sprintf(path, "%s%u", tableName, instNum);
    rtHashMap_Set(gContextMaps.row, path, context);
    if(alias)
    {
        char aliasPath[RBUS_MAX_NAME_LENGTH];
        sprintf(aliasPath, "%s[%s]", tableName, alias);
        rtHashMap_Set(gContextMaps.alias, aliasPath, path);
        rtHashMap_Set(gContextMaps.inst, path, aliasPath);
    }
}

void RemoveRowContextByInstNum(char const* tableName, uint32_t instNum)
{
    char rowName[RBUS_MAX_NAME_LENGTH];
    sprintf(rowName, "%s%u", ConvertPath(tableName), instNum);
    RemoveRowContextByName(rowName);
}

void RemoveRowContextByName(char const* rowName)
{
    char const* rowName2 = ConvertPath(rowName);
    rtHashMap_Remove(gContextMaps.row, rowName2);
    char const* alias = rtHashMap_Get(gContextMaps.inst, rowName2);
    if(alias)
    {
      rtHashMap_Remove(gContextMaps.alias, alias);
      rtHashMap_Remove(gContextMaps.inst, rowName2);
    }
}


/*Hack to keep ccsp style dynamic tables somewhat efficient.
  Rbus doesn't have anything similar to the Update/Synchronize
  callbacks ccsp uses to support their dynamic tables.
  When a partial path query come in, get handlers for every
  property inside a table can be called.  The generated code
  for dynamic tables will call IsUpdated/Synchronize inside
  each getHandler.  However, we only want the Synchronize
  to happen once for the whole group of gets (or sets) that are
  called on properties inside the table.  By using a timer we 
  can prevent these unnecessary syncs from happening.
  This will search for the parent table the path is under
  and then get the last sync timestamp from dynTableTimeMap
  and compare that to the current time.
 */
bool IsTimeToSyncDynamicTable(char const* path)
{
    void* ptime = NULL;
    time_t now;
    char parent[RBUS_MAX_NAME_LENGTH];
    char* p;
    strcpy(parent, path);
    p = parent + strlen(parent)-1;
    for(;;)
    {
        while(p > parent && *p != '.')
            p--;
        if(p == parent)
            return false;
        if(*p == '.')
            *p = 0;
        while(p > parent && *p != '.')
            p--;
        if(p == parent)
            return false;
        if(*p == '.')
        {
            if(atoi(p+1) > 0 && *(p+1) != '0')
            {
                *p = 0;
                continue;
            }
            else
            {
                break;
            }
        }
    }
    ptime = rtHashMap_Get(gContextMaps.dyntime, parent);
    now = time(NULL);
    if(!ptime || now > (time_t)ptime + 5)
    {
        printf("%s %s YES\\n", __FUNCTION__, parent);
        rtHashMap_Set(gContextMaps.dyntime, parent, (void*)now);
        return true;
    }

    printf("%s %s NO\\n", __FUNCTION__, parent);
    return false;
}

char const* GetParamName(char const* path)
{
    char const* p = path + strlen(path);
    while(p > path && *(p-1) != '.')
        p--;
    return p;
}

void Context_Init()
{
    if(gContextRefCount == 0)
    {
        rtHashMap_Create(&gContextMaps.row);
        rtHashMap_CreateEx(&gContextMaps.alias, 0, NULL, NULL, NULL, NULL, rtHashMap_Copy_Func_String, rtHashMap_Destroy_Func_Free);
        rtHashMap_CreateEx(&gContextMaps.inst, 0, NULL, NULL, NULL, NULL, rtHashMap_Copy_Func_String, rtHashMap_Destroy_Func_Free);
        rtHashMap_Create(&gContextMaps.dyntime);
    }
    gContextRefCount++;
}

void Context_Release()
{
    gContextRefCount--;
    if(gContextRefCount == 0)
    {
        if(gContextMaps.row)
        {
            rtHashMap_Destroy(gContextMaps.row);
            gContextMaps.row = NULL;
        }
        if(gContextMaps.alias)
        {
            rtHashMap_Destroy(gContextMaps.alias);
            gContextMaps.alias = NULL;
        }
        if(gContextMaps.inst)
        {
            rtHashMap_Destroy(gContextMaps.inst);
            gContextMaps.inst = NULL;
        }
        if(gContextMaps.dyntime)
        {
            rtHashMap_Destroy(gContextMaps.dyntime);
            gContextMaps.dyntime = NULL;
        }  
    }
}

HandlerContext GetHandlerContext(char const* name)
{
    HandlerContext context;
    context.fullName = ConvertPath(name);
    context.name = GetParamName(context.fullName);
    context.userData = GetParentContext(context.fullName);
    return context;
}