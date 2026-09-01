/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include "Sample_hal.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define staticRecordCount 3

static HalData halData = { true, 24, 35600, "Try setting any StringParam to \"TESTROLLBACK\" to test rollback"};
static HalStaticRecord staticRecords[staticRecordCount] = {
    {true,  -20,    10,     "Static Record 1"},
    {false, -400,   200,    "Static Record 2"},
    {true,  -8000,  4000,   "Static Record 3"},
};
static HalWriteableRecord* writableRecords = NULL;
static int writableRecordCount = 0;
static HalDynamicRecord* dynamicRecords = NULL;
static int dynamicRecordCount = 0;
static uint32_t lastDynamicTime = 0;

void updateDynamicTable()
{
    int i;
    if((uint32_t)time(NULL) - lastDynamicTime < 10)
        return;
    if(dynamicRecords)
        free(dynamicRecords);
    dynamicRecordCount = 1 + rand() % 9;
    dynamicRecords = malloc(sizeof(HalDynamicRecord) * dynamicRecordCount);
    for(i = 0; i < dynamicRecordCount; ++i)
    {
        HalDynamicRecord* rec = dynamicRecords + i;
        rec->intVal = rand() % 10;
        rec->boolVal = rec->intVal % 2;
        rec->ulongVal = rand() % 1000;
        switch(rec->intVal)
        {
            case 0: strcpy(rec->stringVal, "zero"); break;
            case 1: strcpy(rec->stringVal, "one"); break;
            case 2: strcpy(rec->stringVal, "two"); break;
            case 3: strcpy(rec->stringVal, "three"); break;
            case 4: strcpy(rec->stringVal, "four"); break;
            case 5: strcpy(rec->stringVal, "five"); break;
            case 6: strcpy(rec->stringVal, "size"); break;
            case 7: strcpy(rec->stringVal, "seven"); break;
            case 8: strcpy(rec->stringVal, "eight"); break;
            case 9: strcpy(rec->stringVal, "nine"); break;
        }
    }
    lastDynamicTime = (uint32_t)time(NULL);
}

void Hal_Init()
{
    srand((unsigned)time(NULL));
}

void Hal_Unload()
{
}

void Hal_GetData(HalData* data)
{
    memcpy(data, &halData, sizeof(HalData));
}

void Hal_SetData(HalData* data)
{
    memcpy(&halData, data, sizeof(HalData));
}

int Hal_GetStaticRecordCount()
{
    return staticRecordCount;
}

void Hal_GetStaticRecord(uint32_t id, HalStaticRecord* data)
{
    if(id < staticRecordCount)
        memcpy(data, &staticRecords[id], sizeof(HalStaticRecord));
}

void Hal_SetStaticRecord(uint32_t id, HalStaticRecord* data)
{
    if(id < staticRecordCount)
        memcpy(&staticRecords[id], data, sizeof(HalStaticRecord)); 
}

int Hal_GetWritableRecordCount()
{
    return writableRecordCount;
}

void Hal_GetWritableRecord(uint32_t id, HalWriteableRecord* data)
{
    if(id < writableRecordCount)
        memcpy(data, writableRecords+id, sizeof(HalWriteableRecord)); 
}

void Hal_SetWritableRecord(uint32_t id, HalWriteableRecord* data)
{
    if(id < writableRecordCount)
        memcpy(writableRecords+id, data, sizeof(HalWriteableRecord)); 
}

uint32_t Hal_AddWritableRecord()
{
    writableRecordCount++;
    if(writableRecords)
        writableRecords = realloc(writableRecords, sizeof(HalWriteableRecord) * writableRecordCount);
    else
        writableRecords = malloc(sizeof(HalWriteableRecord) * writableRecordCount);
    return writableRecordCount;
}

void Hal_DelWritableRecord(uint32_t id)
{
    if(id < writableRecordCount)
    {
        uint32_t i;
        for(i = id; i < writableRecordCount; ++i)
        {
            memcpy(writableRecords+i+1, writableRecords+i, sizeof(HalWriteableRecord));
        }
    }
    writableRecordCount--;
}

bool Hal_IsUpdatedDynamicRecords()
{
    if((uint32_t)time(NULL) - lastDynamicTime > 10)
        return true;
    else
        return false;
}

int Hal_GetDynamicRecordCount()
{
    updateDynamicTable();
    return dynamicRecordCount;
}

void Hal_GetDynamicRecord(uint32_t id, HalDynamicRecord* data)
{
    updateDynamicTable();
    if(id < dynamicRecordCount)
        memcpy(data, dynamicRecords+id, sizeof(HalDynamicRecord));
}
