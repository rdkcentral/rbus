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

#ifndef BUS_CONTEXT_HELPER_H
#define BUS_CONTEXT_HELPER_H

#include <rbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* void Context_Init()
 *  @brief      Initialize the context helper api.  This maintain a reference count and can 
                be called safely repeatedly; however, Context_Release should be called once
                for every call to Context_Init.
 */
void Context_Init();

/* void Context_Release()
 *  @brief      Release the context helpers.  This should be called once for every Context_Init call.
 */
void Context_Release();

/* char const* ConvertPath(char const* path)
 *  @brief      Convert a path containing row aliases to a path containing instance numbers only.
                All methods below which take a path, expect the path to contain only instance numbers
                for row identiers.  Currently this is not a thread-safe function because it uses
                a static buffer to hold the coverted path in.  
 *  @param      path            The row path
 *  @return     context         The context for the row which was previously set by SetRowContext
 */
char const* ConvertPath(char const* path);

/* void* GetParentContext(char const* path)
 *  @brief      Get the row context(if any) for a parameter inside that row.
 *  @param      path            A parameter path for some element inside a row
 *  @return     context         The context for the row in which the parameter exists
 */
void* GetParentContext(char const* path);

/* void* GetRowContext(char const* path)
 *  @brief      Get the context of a row.  
 *  @param      path            The row path
 *  @return     context         The context for the row which was previously set by SetRowContext
 */
void* GetRowContext(char const* path);

/* void SetRowContext(char const* tableName, uint32_t instNum, char const* alias, void* context)
 *  @brief      Set the context for a newly created row along with its instance number and any alias it might have
 *  @param      tableName       The path to the table the row was created in
 *  @param      instNum         The instance number of the new row
 *  @param      alias           The alias for the new row (if any). Null if no alias exists.
 *  @param      context         The context for the row which was returned from the AddEntry handler
 */
void SetRowContext(char const* tableName, uint32_t instNum, char const* alias, void* context);

/* void RemoveRowContextByInstNum(char const* tableName, uint32_t instNum)
 *  @brief      Remove the context of a row previously set with SetRowContext
 *  @param      tableName       The path to the table of the row being removed
 *  @param      instNum         The instance number of the row being removed
 */
void RemoveRowContextByInstNum(char const* tableName, uint32_t instNum);

/* void RemoveRowContextByName(char const* tableName, uint32_t instNum)
 *  @brief      Remove the context of a row previously set with SetRowContext
 *  @param      rowName         The path to the row being removed (which includes its instance number)
 */
void RemoveRowContextByName(char const* rowName);

/** @fn         bool IsTimeToSyncDynamicTable(char const* path)
 *  @brief      Informs the caller if its ok to now call IsUpdated/Syncrhonize handlers for dynamic tables
 *              This is used to prevent dynamic tables from get synchronized multiple times
 *              during a partial path or multi set operation.
 *  @param      path            A dynamic table path
 *  @return     bool            True if its ok to sync or false if we should wait longer
 */
bool IsTimeToSyncDynamicTable(char const* path);

/** @fn         char const* GetParamName(char const* path)
 *  @brief      Get the relative name, or leaf name, from the end of the path
 *  @param      path            A parameter path
 *  @return     name            The name.  This pointer points into the path argument passed into the function
 */
char const* GetParamName(char const* path);


typedef struct 
{
    char const* fullName;   /*full name of element with any table alias converted to instance number*/
    char const* name;       /*leaf name of element*/
    void* userData;         /*any context data set for the this element's containing object (e.g. row)*/
} HandlerContext;

/** @fn         HandlerContext GetHandlerContext(char const* name)
 *  @brief      Get a handler context for a given element name
 *  @param      name            The element's full name
 *  @return     HandlerContext
 */
HandlerContext GetHandlerContext(char const* name);

#define GetPropertyContext(P) GetHandlerContext(rbusProperty_GetName(P))
#define GetTableContext(T) GetHandlerContext(T)

#ifdef __cplusplus
}
#endif

#endif