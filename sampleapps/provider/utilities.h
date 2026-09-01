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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALIAS_LENGTH 64

typedef enum TableRowIDType
{
    TableRowIDInvalid,
    TableRowIDNumber,
    TableRowIDAlias
} TableRowIDType;

typedef struct TableRowID
{
    TableRowIDType type;
    uint32_t instNum;
    char alias[MAX_ALIAS_LENGTH];
} TableRowID;

bool getTableRowID(char const* path, int index, TableRowID* id);
bool compareTableRowID(TableRowID* id, uint32_t instNum, char const* alias);

bool propertyNameEquals(char const* propertyFullName, const char* propertyLastName);

#ifdef __cplusplus
}
#endif

#endif
