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
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>
#include "../common/test_macros.h"
#include <rtMemory.h>

int getDurationTables()
{
    return 1;
}

typedef struct TableRowTest
{
    char table[RBUS_MAX_NAME_LENGTH];
    char* alias;
    uint32_t num;
    char* data[2];
    int removeOrder;
    bool exists;
    
} TableRowTest;

TableRowTest testData[] = {
    { "Device.TestProvider.Table1.", "t1_1", 1, {"t1_1", "a1.1"}, -1, false  },
    { "Device.TestProvider.Table1.", "t1_2", 2, {"t1_2", "a1.2"},  8, false  },
    { "Device.TestProvider.Table1.", "t1_3", 3, {"t1_3", "a1.3"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.",     NULL, 1, {"t1_1_1", "a1.1.1"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.", "t1_1_2", 2, {"t1_1_2", "a1.1.2"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.", "t1_1_3", 3, {"t1_1_3", "a1.1.3"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.", "t1_2_1", 1, {"t1_2_1", "a1.2.1"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.",     NULL, 2, {"t1_2_2", "a1.2.2"},  7, false  },
    { "Device.TestProvider.Table1.2.Table2.", "t1_2_3", 3, {"t1_2_3", "a1.2.3"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.", "t1_3_1", 1, {"t1_3_1", "a1.3.1"},  4, false  },
    { "Device.TestProvider.Table1.3.Table2.", "t1_3_2", 2, {"t1_3_2", "a1.3.2"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.",     NULL, 3, {"t1_3_3", "a1.3.2"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.1.Table3.", "t1_1_1_1", 1, {"t1_1_1_1", "a1.1.1.1"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.1.Table3.", "t1_1_1_2", 2, {"t1_1_1_2", "a1.1.1.2"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.1.Table3.", "t1_1_1_3", 3, {"t1_1_1_3", "a1.1.1.3"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.2.Table3.",       NULL, 1, {"t1_1_2_1", "a1.1.2.1"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.2.Table3.", "t1_1_2_2", 2, {"t1_1_2_2", "a1.1.2.2"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.2.Table3.", "t1_1_2_3", 3, {"t1_1_2_3", "a1.1.2.3"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.3.Table3.", "t1_1_3_1", 1, {"t1_1_3_1", "a1.1.3.1"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.3.Table3.",       NULL, 2, {"t1_1_3_2", "a1.1.3.2"}, -1, false  },
    { "Device.TestProvider.Table1.1.Table2.3.Table3.", "t1_1_3_3", 3, {"t1_1_3_3", "a1.1.3.3"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.1.Table3.", "t1_2_1_1", 1, {"t1_2_1_1", "a1.2.1.1"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.1.Table3.", "t1_2_1_2", 2, {"t1_2_1_2", "a1.2.1.2"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.1.Table3.",       NULL, 3, {"t1_2_1_3", "a1.2.1.3"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.2.Table3.",       NULL, 1, {"t1_2_2_1", "a1.2.2.1"},  6, false  },
    { "Device.TestProvider.Table1.2.Table2.2.Table3.",       NULL, 2, {"t1_2_2_2", "a1.2.2.2"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.2.Table3.", "t1_2_2_3", 3, {"t1_2_2_3", "a1.2.2.3"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.3.Table3.",       NULL, 1, {"t1_2_3_1", "a1.2.3.1"}, -1, false  },
    { "Device.TestProvider.Table1.2.Table2.3.Table3.", "t1_2_3_2", 2, {"t1_2_3_2", "a1.2.3.2"},  5, false  },
    { "Device.TestProvider.Table1.2.Table2.3.Table3.",       NULL, 3, {"t1_2_3_3", "a1.2.3.3"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.1.Table3.", "t1_3_1_1", 1, {"t1_3_1_1", "a1.3.1.1"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.1.Table3.",       NULL, 2, {"t1_3_1_2", "a1.3.1.2"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.1.Table3.",       NULL, 3, {"t1_3_1_3", "a1.3.1.3"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.2.Table3.",       NULL, 1, {"t1_3_2_1", "a1.3.2.1"},  3, false  },
    { "Device.TestProvider.Table1.3.Table2.2.Table3.",       NULL, 2, {"t1_3_2_2", "a1.3.2.2"},  2, false  },
    { "Device.TestProvider.Table1.3.Table2.2.Table3.",       NULL, 3, {"t1_3_2_3", "a1.3.2.3"},  1, false  },
    { "Device.TestProvider.Table1.3.Table2.3.Table3.", "t1_3_3_1", 1, {"t1_3_3_1", "a1.3.3.1"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.3.Table3.", "t1_3_3_2", 2, {"t1_3_3_2", "a1.3.3.2"}, -1, false  },
    { "Device.TestProvider.Table1.3.Table2.3.Table3.", "t1_3_3_3", 3, {"t1_3_3_3", "a1.3.3.3"},  0, false  }
};

TableRowTest* tests = NULL;

int numRows = sizeof(testData)/sizeof(TableRowTest);

void resetData()
{
    if(tests)
        free(tests);

    tests = rt_malloc(sizeof(testData));

    memcpy(tests, testData, sizeof(testData));
}


int testAddRow(rbusHandle_t handle, TableRowTest* test, bool firstTime)
{
    rbusError_t err = 0;
    uint32_t instNum = 0;
    bool success = true;

    if(!test->exists || test->alias /*test that adding multiple alias fails*/)
    {   
        err = rbusTable_addRow(handle, test->table, test->alias, &instNum);
        if(!test->exists)
        {
            if(firstTime)
            {
                /*on the first addRow, the instNum should match the expected test->num value*/
                success = (err == RBUS_ERROR_SUCCESS && instNum == test->num);
            }
            else
            {
                /*on the second addRow, the instNum will be one not already used, 
                 thus > 3 since our test data adds 3 to each table the first time*/
                success = (err == RBUS_ERROR_SUCCESS && (instNum == test->num || instNum > 3));
                test->num = instNum;
            }
        }
        else
            success = (err != RBUS_ERROR_SUCCESS);
    }

    printf("\n# %s _test_Tables_addRow table=%s alias=%s num=%d exist=%d err=%d retnum=%d\n\n", 
        success ? "PASS" : "FAIL", test->table, test->alias, test->num, test->exists, err, instNum);
    TALLY(success);

    test->exists = true;

    return success ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
}

int testRemoveRow(rbusHandle_t handle, TableRowTest* test, bool byIndex)
{
    rbusError_t err;
    char rowName[RBUS_MAX_NAME_LENGTH];
    bool success;
    int  rc;

    if(byIndex || test->alias == NULL){
        rc = snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d", test->table, test->num);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at rowName - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }
    else{
        rc = snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s[%s]", test->table, test->alias);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at rowName - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }

    err = rbusTable_removeRow(handle, rowName);

    if(test->exists)
    {
        success = (err == RBUS_ERROR_SUCCESS);
    }
    else
    {   /*if row didn't exist then we should expect an error*/
        success = (err != RBUS_ERROR_SUCCESS);
    }

    printf("\n# %s _test_Tables_removeRow rowName=%s table=%s alias=%s num=%d exist=%d err=%d\n\n", 
        success ? "PASS" : "FAIL", rowName, test->table, test->alias, test->num, test->exists, err);
    TALLY(success);

    test->exists = false;

    return success ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
}

int testSet(rbusHandle_t handle, TableRowTest* test, bool byIndex, int iData)
{
    rbusError_t err;
    char prop[RBUS_MAX_NAME_LENGTH];
    bool success;
    int  rc;

    if(byIndex || test->alias == NULL){
        rc = snprintf(prop, RBUS_MAX_NAME_LENGTH, "%s%d.data", test->table, test->num);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at prop - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }
    else{
        rc = snprintf(prop, RBUS_MAX_NAME_LENGTH, "%s[%s].data", test->table, test->alias);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at prop - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }

    err = rbus_setStr(handle, prop, test->data[iData]);

    if(test->exists)
    {
        success = err == RBUS_ERROR_SUCCESS;
    }
    else
    {
        success = err != RBUS_ERROR_SUCCESS;
    }

    printf("\n# %s _test_Tables_setStr prop=%s data=%s table=%s alias=%s num=%d exist=%d err=%d\n\n", 
        success ? "PASS" : "FAILS", prop, test->data[iData], test->table, test->alias, test->num, test->exists, err);
    TALLY(success);

    return success ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
}

int testGet(rbusHandle_t handle, TableRowTest* test, bool byIndex, int iData)
{
    rbusError_t err;
    char prop[RBUS_MAX_NAME_LENGTH];
    char* value = NULL;
    bool success;
    int  rc;

    if(byIndex || test->alias == NULL){
        rc = snprintf(prop, RBUS_MAX_NAME_LENGTH, "%s%d.data", test->table, test->num);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at prop - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }

    else{
        rc = snprintf(prop, RBUS_MAX_NAME_LENGTH, "%s[%s].data", test->table, test->alias);
        if(rc >= RBUS_MAX_NAME_LENGTH)
            printf("Format Truncation error at prop - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
    }

    err = rbus_getStr(handle, prop, &value);

    if(test->exists)
    {
        success = (err == RBUS_ERROR_SUCCESS && strcmp(test->data[iData], value)==0);
    }
    else
    {
        success = err != RBUS_ERROR_SUCCESS;
    }

    printf("\n# %s _test_Tables_getStr prop=%s data=%s table=%s alias=%s num=%d exist=%d err=%d retdata=%s\n\n", 
        success ? "PASS" : "FAILS", prop, test->data[iData], test->table, test->alias, test->num, test->exists, err, value ? value : "");
    TALLY(success);

    if(err == RBUS_ERROR_SUCCESS && value)
         free(value);

    return success ? RBUS_ERROR_SUCCESS : RBUS_ERROR_BUS_ERROR;
}

int testAddMissingRows(rbusHandle_t handle, bool firstTime)
{
    int rows = numRows;
    int i, j, rc;

    printf("_test_Tables_%s numRows=%d\n", __FUNCTION__, rows);
    for(i = 0; i < rows; ++i)
    {
        uint32_t previousNum = tests[i].num;

        if(testAddRow(handle, &tests[i], firstTime) != RBUS_ERROR_SUCCESS)
            return RBUS_ERROR_BUS_ERROR;

        /*update children to point to new number*/
        if(!firstTime && previousNum != tests[i].num)
        {
            char name[RBUS_MAX_NAME_LENGTH];

            rc = snprintf(name, RBUS_MAX_NAME_LENGTH, "%s%d.", tests[i].table, previousNum);
            if(rc >= RBUS_MAX_NAME_LENGTH)
                printf("Format Truncation error at name - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);

            for(j = i+1; j < rows; ++j)
            {
                if( strncmp(name, tests[j].table, strlen(name))==0)
                {
                    char newName[RBUS_MAX_NAME_LENGTH];
                    rc = snprintf(newName, RBUS_MAX_NAME_LENGTH, "%s%d.%s", tests[i].table, tests[i].num, tests[j].table+strlen(name));
                    if(rc >= RBUS_MAX_NAME_LENGTH)
                        printf("Format Truncation error at name - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);
                    printf("updating name from: %s to: %s\n", tests[j].table, newName);
                    strncpy(tests[j].table, newName, RBUS_MAX_NAME_LENGTH);
                }    
            }
        }
    }
    return RBUS_ERROR_SUCCESS;
}

int testSetsAndGets(rbusHandle_t handle)
{
    int rows = numRows;
    int i;

    printf("_test_Tables_%s\n", __FUNCTION__);
    for(i = 0; i < rows; ++i)
    {
        if(testSet(handle, &tests[i], true, 0) != RBUS_ERROR_SUCCESS)/*set data by instNum*/
            return RBUS_ERROR_BUS_ERROR;
        if(testGet(handle, &tests[i], true, 0) != RBUS_ERROR_SUCCESS)/*get data by instNum*/
            return RBUS_ERROR_BUS_ERROR;
        if(testGet(handle, &tests[i], false, 0) != RBUS_ERROR_SUCCESS)/*get data by alias*/
            return RBUS_ERROR_BUS_ERROR;
        if(testSet(handle, &tests[i], false, 1) != RBUS_ERROR_SUCCESS)/*set data by alias*/
            return RBUS_ERROR_BUS_ERROR;
        if(testGet(handle, &tests[i], true, 1) != RBUS_ERROR_SUCCESS)/*get data by instNum*/
            return RBUS_ERROR_BUS_ERROR;
        if(testGet(handle, &tests[i], false, 1) != RBUS_ERROR_SUCCESS)/*get data by alias*/
            return RBUS_ERROR_BUS_ERROR;
    }
    return RBUS_ERROR_SUCCESS;
}

int testGetRowNames(rbusHandle_t handle)
{
    int i;

    printf("_test_Tables_%s\n", __FUNCTION__);
    for(i = 0; i < numRows; i+=3)
    {
        int j;
        rbusRowName_t* rows = NULL;
        rbusRowName_t* row = NULL;
        if(rbusTable_getRowNames(handle, tests[i].table, &rows) != RBUS_ERROR_SUCCESS)
            return RBUS_ERROR_BUS_ERROR;
        row = rows;
        j = 0;
        while(row)
        {
            char path[RBUS_MAX_NAME_LENGTH+10];
            bool pass;
            printf("testGetRowNames %d: %s %u %s\n", i+j, row->name, row->instNum, row->alias);
            snprintf(path, RBUS_MAX_NAME_LENGTH+10, "%s%u.", tests[i+j].table, tests[i+j].num);
            pass = strcmp(path, row->name) == 0;
            if(!pass)
                printf("FAIL testGetRowNames name doesn't match expected=%s actual=%s\n", path, row->name);
            TALLY(pass);
            pass = tests[i+j].num == row->instNum;
            if(!pass)
                printf("FAIL testGetRowNames instNum doesn't match expected=%u actual=%u\n", tests[i+j].num, row->instNum);
            TALLY(pass);
            pass = (tests[i+j].alias && row->alias && strcmp(tests[i+j].alias, row->alias) == 0) || (!tests[i+j].alias && !row->alias);
            if(!pass)
                printf("FAIL testGetRowNames alias doesn't match expected=%s actual=%s\n", tests[i+j].alias, row->alias);
            TALLY(pass);
            row = row->next;
            j++;
        }
        rbusTable_freeRowNames(handle, rows);
    }
    return RBUS_ERROR_SUCCESS;
}

int testGetNames(rbusHandle_t handle)
{
    int i;

    printf("_test_Tables_%s\n", __FUNCTION__);
    for(i = 0; i < numRows; i+=3)
    {
        rbusElementInfo_t* elems = NULL;
        rbusElementInfo_t* elem;
        if(rbusElementInfo_get(handle, tests[i].table, -1, &elems) != RBUS_ERROR_SUCCESS)
            return RBUS_ERROR_BUS_ERROR;
        elem = elems;
        int j = 0;
        while(elem)
        {
            char path[RBUS_MAX_NAME_LENGTH+10];
            bool pass;
            snprintf(path, RBUS_MAX_NAME_LENGTH+10, "%s%u.", tests[i+j].table, tests[i+j].num);
            pass = strcmp(path, elem->name) == 0;
            printf("%s testGetNames %d: expected=%s actual=%s\n", pass ? "PASS" : "FAIL", i+j, path, elem->name);
            TALLY(pass);
            elem = elem->next;
            j++;
        }
        rbusElementInfo_free(handle, elems);
    }
    {
        rbusElementInfo_t* elems = NULL;
        rbusElementInfo_t* elem;        
        bool pass;
        int numNames = 0;
        if(rbusElementInfo_get(handle, "Device.TestProvider.Table1.1.", -1, &elems) != RBUS_ERROR_SUCCESS)
            return RBUS_ERROR_BUS_ERROR;
        elem = elems;
        while(elem)
        {
            if(elem->type != RBUS_ELEMENT_TYPE_METHOD)
                numNames++;
            elem = elem->next;
            
        }
        printf("%s testGetNames Device.TestProvider.Table1.1.: numNames actual=%u expected=%u\n", numNames == 2 ? "PASS" : "FAIL", numNames, 2);
        TALLY(numNames == 2);
        if(numNames > 0)
        {
            pass = strcmp("Device.TestProvider.Table1.1.Table2.", elems->name) == 0;
            printf("%s testGetNames Device.TestProvider.Table1.1.: expected=%s actual=%s\n", pass ? "PASS" : "FAIL", "Device.TestProvider.Table1.1.Table2.", elems->name);
        }
        if(numNames > 1)
        {
            pass = strcmp("Device.TestProvider.Table1.1.data", elems->next->name) == 0;
            printf("%s testGetNames Device.TestProvider.Table1.1.: expected=%s actual=%s\n", pass ? "PASS" : "FAIL", "Device.TestProvider.Table1.1.data", elems->next->name);
        }
        rbusElementInfo_free(handle, elems);
    }
    return RBUS_ERROR_SUCCESS;
}

int testRemoveOneRow(rbusHandle_t handle, TableRowTest* test, bool byIndex)
{
    int rows = numRows;
    int i, rc;
    char name[RBUS_MAX_NAME_LENGTH];

    printf("_test_Tables_%s\n", __FUNCTION__);
    if(testRemoveRow(handle, test, byIndex) != RBUS_ERROR_SUCCESS)
        return RBUS_ERROR_BUS_ERROR;

    rc = snprintf(name, RBUS_MAX_NAME_LENGTH, "%s%d.", test->table, test->num);
    if(rc >= RBUS_MAX_NAME_LENGTH)
        printf("Format Truncation error at name - %d %s %s:%d", rc,  __FILE__, __FUNCTION__, __LINE__);

    /*we expect all children of this row to be recursively removed 
      so mark all children of this node non-existing*/
    for(i = 0; i < rows; ++i)
    {
        if( test != &tests[i] && strncmp(name, tests[i].table, strlen(name))==0)
            tests[i].exists = false;
    }

    return RBUS_ERROR_SUCCESS;
}

int testRemoveSomeRows(rbusHandle_t handle, bool byIndex)
{
    int rows = numRows;
    int i;
    int removeOrder;

    removeOrder = 0;

    printf("_test_Tables_%s\n", __FUNCTION__);
    for(i = 0; i < rows; ++i)
    {
        if(tests[i].removeOrder == removeOrder)
        {
            if(testRemoveOneRow(handle, &tests[i], byIndex) != RBUS_ERROR_SUCCESS)
                return RBUS_ERROR_BUS_ERROR;

            removeOrder++;
            i = -1; /*start over until we exhause removeOrders*/
        }
    }
    return RBUS_ERROR_SUCCESS;
}

int testRemoveAllRows(rbusHandle_t handle, bool byIndex)
{
    int rows = numRows;
    int i;

    printf("_test_Tables_%s\n", __FUNCTION__);
    for(i = 0; i < rows; ++i)
    {
        if(testRemoveOneRow(handle, &tests[i], byIndex) != RBUS_ERROR_SUCCESS)
            return RBUS_ERROR_BUS_ERROR;
    }
    return RBUS_ERROR_SUCCESS;
}

void testTables(rbusHandle_t handle, int* countPass, int* countFail)
{
    resetData();

    if(testAddMissingRows(handle, true) != RBUS_ERROR_SUCCESS
    || testSetsAndGets(handle) != RBUS_ERROR_SUCCESS
    || testGetRowNames(handle) != RBUS_ERROR_SUCCESS
    || testGetNames(handle) != RBUS_ERROR_SUCCESS
    || testRemoveSomeRows(handle, true) != RBUS_ERROR_SUCCESS
    || testSetsAndGets(handle) != RBUS_ERROR_SUCCESS
    || testAddMissingRows(handle, false) != RBUS_ERROR_SUCCESS
    || testRemoveSomeRows(handle, false) != RBUS_ERROR_SUCCESS
    || testSetsAndGets(handle) != RBUS_ERROR_SUCCESS
    || testRemoveAllRows(handle, true) != RBUS_ERROR_SUCCESS
    || testSetsAndGets(handle) != RBUS_ERROR_SUCCESS)
        goto exit1;

exit1:
    if(tests)
        free(tests);
    *countPass = gCountPass;
    *countFail = gCountFail;
    PRINT_TEST_RESULTS("_test_Tables");
}
