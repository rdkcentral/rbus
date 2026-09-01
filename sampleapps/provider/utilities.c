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

#include <stdlib.h>
#include <string.h>
#include "utilities.h"

bool getTableRowID(char const* path, int index, TableRowID* id)
{
    char const* p;
    int i;

    p = path;
    i = 0;

    memset(id, 0, sizeof(TableRowID));

    while(i < index)
    {
        /*scan past any other alias we find along the way*/
        if(*p == '[')
        {
            p = strchr(p, ']');
            if(p == NULL)
                return false;
        }

        p = strchr(p, '.');
        if(p == NULL)
            return false;
        p++;
        if(!*p)
            return false;

        i++;
    }
    
    if(*p == '[')
    {
        int i = 0;

        p++;

        while(*p && *p != ']')
            id->alias[i++] = *p++;

        if(strlen(id->alias) == 0)
            return false;

        id->type = TableRowIDAlias;
    }
    else
    {
        char buff[MAX_ALIAS_LENGTH] = {0};
        int i = 0;
        while(*p && *p != '.')
            buff[i++] = *p++;

        id->instNum = atoi(buff);

        if(id->instNum < 1)
            return false;

        id->type = TableRowIDNumber;
    }

    return true;
}

bool compareTableRowID(TableRowID* rowID, uint32_t instNum, char const* alias)
{
    if( (rowID->type == TableRowIDNumber && instNum == rowID->instNum) ||
        (rowID->type == TableRowIDAlias && strcmp(alias, rowID->alias)==0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool propertyNameEquals(char const* path, const char* ending)
{
    int offset = (int)strlen(path) - (int)strlen(ending);

    if(offset < 0)
        return false;

    if(offset == 0)
        return strcmp(path, ending) == 0;
        
    if(path[offset-1] != '.')
        return false;

    return strcmp(&path[offset], ending) == 0;
}


